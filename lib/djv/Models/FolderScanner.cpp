// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/FolderScanner.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <climits>
#include <cstring>
#include <cwctype>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <winternl.h>
#endif

namespace djv
{
    namespace models
    {
        namespace
        {
#if defined(_WIN32)
            class WinHandle
            {
            public:
                WinHandle() = default;
                explicit WinHandle(HANDLE value) : value(value) {}
                ~WinHandle() { reset(); }
                WinHandle(WinHandle&& other) noexcept : value(other.value)
                { other.value = INVALID_HANDLE_VALUE; }
                WinHandle& operator=(WinHandle&& other) noexcept
                {
                    if (this != &other)
                    {
                        reset();
                        value = other.value;
                        other.value = INVALID_HANDLE_VALUE;
                    }
                    return *this;
                }
                WinHandle(const WinHandle&) = delete;
                WinHandle& operator=(const WinHandle&) = delete;
                explicit operator bool() const
                { return value && INVALID_HANDLE_VALUE != value; }
                HANDLE get() const { return value; }
                void reset(HANDLE next = INVALID_HANDLE_VALUE)
                {
                    if (*this)
                    {
                        CloseHandle(value);
                    }
                    value = next;
                }
            private:
                HANDLE value = INVALID_HANDLE_VALUE;
            };

            struct DirectoryIdentity
            {
                ULONGLONG volume = 0;
                FILE_ID_128 id = {};

                bool operator==(const DirectoryIdentity& other) const
                {
                    return volume == other.volume &&
                        0 == std::memcmp(id.Identifier, other.id.Identifier, sizeof(id.Identifier));
                }
            };

            struct WinPendingDirectory
            {
                std::filesystem::path path;
                size_t depth = 0;
                WinHandle handle;
                DirectoryIdentity identity;
            };

            using NtCreateFileFn = NTSTATUS (NTAPI *)(
                PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
                PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);
            using RtlNtStatusToDosErrorFn = ULONG (WINAPI *)(NTSTATUS);

            std::error_code winError(DWORD value = GetLastError())
            {
                return std::error_code(static_cast<int>(value), std::system_category());
            }

            bool getIdentity(HANDLE handle, DirectoryIdentity& out)
            {
                FILE_ID_INFO info = {};
                if (!GetFileInformationByHandleEx(
                    handle, FileIdInfo, &info, sizeof(info)))
                {
                    return false;
                }
                out.volume = info.VolumeSerialNumber;
                out.id = info.FileId;
                return true;
            }

            std::wstring finalPath(HANDLE handle)
            {
                const DWORD size = GetFinalPathNameByHandleW(
                    handle, nullptr, 0, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
                if (!size)
                {
                    return {};
                }
                std::wstring out(size, L'\0');
                const DWORD written = GetFinalPathNameByHandleW(
                    handle, out.data(), size, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
                if (!written || written >= size)
                {
                    return {};
                }
                out.resize(written);
                std::transform(out.begin(), out.end(), out.begin(), towlower);
                while (out.size() > 4 && (L'\\' == out.back() || L'/' == out.back()))
                {
                    out.pop_back();
                }
                return out;
            }

            bool isWithinRoot(const std::wstring& root, const std::wstring& candidate)
            {
                return candidate == root ||
                    (candidate.size() > root.size() &&
                     0 == candidate.compare(0, root.size(), root) &&
                     (L'\\' == candidate[root.size()] || L'/' == candidate[root.size()]));
            }

            WinHandle openRootDirectory(
                const std::filesystem::path& path,
                bool openReparsePoint,
                std::error_code& error)
            {
                const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS |
                    (openReparsePoint ? FILE_FLAG_OPEN_REPARSE_POINT : 0);
                HANDLE handle = CreateFileW(
                    path.c_str(),
                    FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr,
                    OPEN_EXISTING,
                    flags,
                    nullptr);
                if (INVALID_HANDLE_VALUE == handle)
                {
                    error = winError();
                    return {};
                }
                error.clear();
                return WinHandle(handle);
            }

            WinHandle openRelativeDirectory(
                HANDLE parent,
                const std::wstring& name,
                bool openReparsePoint,
                std::error_code& error)
            {
                if (name.size() > USHRT_MAX / sizeof(wchar_t))
                {
                    error = std::make_error_code(std::errc::filename_too_long);
                    return {};
                }
                static const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
                static const auto ntCreateFile = reinterpret_cast<NtCreateFileFn>(
                    GetProcAddress(ntdll, "NtCreateFile"));
                static const auto rtlStatusToError = reinterpret_cast<RtlNtStatusToDosErrorFn>(
                    GetProcAddress(ntdll, "RtlNtStatusToDosError"));
                if (!ntCreateFile || !rtlStatusToError)
                {
                    error = std::make_error_code(std::errc::function_not_supported);
                    return {};
                }
                UNICODE_STRING string = {};
                string.Buffer = const_cast<PWSTR>(name.data());
                string.Length = static_cast<USHORT>(name.size() * sizeof(wchar_t));
                string.MaximumLength = string.Length;
                OBJECT_ATTRIBUTES attributes;
                InitializeObjectAttributes(
                    &attributes, &string, OBJ_CASE_INSENSITIVE, parent, nullptr);
                IO_STATUS_BLOCK statusBlock = {};
                HANDLE raw = INVALID_HANDLE_VALUE;
                ULONG createOptions = FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT;
                if (openReparsePoint)
                {
                    createOptions |= FILE_OPEN_REPARSE_POINT;
                }
                const NTSTATUS status = ntCreateFile(
                    &raw,
                    FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                    &attributes,
                    &statusBlock,
                    nullptr,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN,
                    createOptions,
                    nullptr,
                    0);
                if (status < 0)
                {
                    error = winError(rtlStatusToError(status));
                    return {};
                }
                error.clear();
                return WinHandle(raw);
            }

            bool directoryAttributes(HANDLE handle, FILE_ATTRIBUTE_TAG_INFO& out)
            {
                return !!GetFileInformationByHandleEx(
                    handle, FileAttributeTagInfo, &out, sizeof(out));
            }
#endif

            struct PendingDirectory
            {
                std::filesystem::path path;
                size_t depth = 0;
            };

            std::string pathString(const std::filesystem::path& path)
            {
                return path.generic_u8string();
            }

            std::string toLowerASCII(const std::string& value)
            {
                std::string out = value;
                std::transform(
                    out.begin(),
                    out.end(),
                    out.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return out;
            }

            std::string normalizeExtension(const std::string& value)
            {
                std::string out = value;
                if (!out.empty() && '.' == out.front())
                {
                    out.erase(out.begin());
                }
                return toLowerASCII(out);
            }

            std::unordered_set<std::string> normalizeExtensions(
                const std::vector<std::string>& values)
            {
                std::unordered_set<std::string> out;
                for (const auto& value : values)
                {
                    const std::string extension = normalizeExtension(value);
                    if (!extension.empty())
                    {
                        out.insert(extension);
                    }
                }
                return out;
            }

            FolderScanWarningCode warningCode(const std::error_code& error)
            {
                return error == std::errc::permission_denied ?
                    FolderScanWarningCode::PermissionDenied :
                    FolderScanWarningCode::IOError;
            }

            void addWarning(
                FolderScanResult& result,
                FolderScanWarningCode code,
                const std::filesystem::path& path,
                const std::error_code& error,
                const std::string& message,
                size_t maxWarnings)
            {
                if (result.warnings.size() < maxWarnings)
                {
                    result.warnings.push_back({ code, path, error, message });
                }
                else
                {
                    ++result.suppressedWarnings;
                }
            }

            bool isLink(
                const std::filesystem::path& path,
                const std::filesystem::file_status& status)
            {
                if (std::filesystem::is_symlink(status))
                {
                    return true;
                }
#if defined(_WIN32)
                if (!std::filesystem::is_directory(status))
                {
                    return false;
                }
                const DWORD attributes = GetFileAttributesW(path.c_str());
                return INVALID_FILE_ATTRIBUTES != attributes &&
                    0 != (attributes & FILE_ATTRIBUTE_REPARSE_POINT);
#else
                return false;
#endif
            }

            std::string canonicalKey(const std::filesystem::path& path)
            {
                std::error_code error;
                std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
                if (error)
                {
                    error.clear();
                    canonical = std::filesystem::absolute(path, error);
                }
                if (error)
                {
                    canonical = path.lexically_normal();
                }
                std::string out = pathString(canonical);
#if defined(_WIN32)
                out = toLowerASCII(out);
#endif
                return out;
            }

            bool lessPath(
                const std::filesystem::path& a,
                const std::filesystem::path& b)
            {
                return pathString(a) < pathString(b);
            }

            bool lessPath(const ftk::Path& a, const ftk::Path& b)
            {
                return a.get() < b.get();
            }

            std::vector<ftk::Path> collapseSequences(
                const std::vector<ftk::Path>& paths,
                const std::unordered_set<std::string>& sequenceExtensions)
            {
                std::vector<ftk::Path> out;
                std::unordered_map<std::string, size_t> groups;
                for (const auto& path : paths)
                {
                    const std::string extension = normalizeExtension(path.getExt());
                    if (path.hasNum() && sequenceExtensions.count(extension))
                    {
                        const std::string key =
                            path.getDir() + '\0' +
                            path.getBase() + '\0' +
                            extension + '\0' +
                            std::to_string(path.getPad());
                        const auto i = groups.find(key);
                        if (i != groups.end() && out[i->second].addSeq(path))
                        {
                            continue;
                        }
                        groups[key] = out.size();
                    }
                    out.push_back(path);
                }
                std::sort(
                    out.begin(),
                    out.end(),
                    [](const ftk::Path& a, const ftk::Path& b)
                    {
                        return lessPath(a, b);
                    });
                return out;
            }
        }

        struct FolderScanCancellationToken::State
        {
            std::atomic_bool cancelled{ false };
#if defined(_WIN32)
            std::mutex mutex;
            HANDLE ioThread = nullptr;
#endif
        };

        FolderScanCancellationToken::FolderScanCancellationToken(
            const std::shared_ptr<State>& state) :
            _state(state)
        {}

        bool FolderScanCancellationToken::isCancellationRequested() const
        {
            return _state && _state->cancelled.load(std::memory_order_acquire);
        }

        void FolderScanCancellationToken::bindToCurrentThread() const
        {
#if defined(_WIN32)
            if (!_state)
            {
                return;
            }
            HANDLE duplicate = nullptr;
            if (!DuplicateHandle(
                GetCurrentProcess(),
                GetCurrentThread(),
                GetCurrentProcess(),
                &duplicate,
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS))
            {
                return;
            }
            std::lock_guard<std::mutex> lock(_state->mutex);
            if (_state->ioThread)
            {
                CloseHandle(_state->ioThread);
            }
            _state->ioThread = duplicate;
            if (_state->cancelled.load(std::memory_order_acquire))
            {
                CancelSynchronousIo(_state->ioThread);
            }
#endif
        }

        void FolderScanCancellationToken::unbindFromCurrentThread() const
        {
#if defined(_WIN32)
            if (!_state)
            {
                return;
            }
            std::lock_guard<std::mutex> lock(_state->mutex);
            if (_state->ioThread)
            {
                CloseHandle(_state->ioThread);
                _state->ioThread = nullptr;
            }
#endif
        }

        FolderScanCancellationSource::FolderScanCancellationSource() :
            _state(std::make_shared<FolderScanCancellationToken::State>())
        {}

        FolderScanCancellationToken FolderScanCancellationSource::getToken() const
        {
            return FolderScanCancellationToken(_state);
        }

        void FolderScanCancellationSource::cancel() const
        {
            _state->cancelled.store(true, std::memory_order_release);
#if defined(_WIN32)
            std::lock_guard<std::mutex> lock(_state->mutex);
            if (_state->ioThread)
            {
                // Directory enumeration on remote filesystems is synchronous;
                // cancel the exact worker thread instead of abandoning it.
                CancelSynchronousIo(_state->ioThread);
            }
#endif
        }

#if defined(_WIN32)
        namespace
        {
            class CancellationBinding
            {
            public:
                explicit CancellationBinding(const FolderScanCancellationToken& value) :
                    token(value)
                {
                    token.bindToCurrentThread();
                }
                ~CancellationBinding()
                {
                    token.unbindFromCurrentThread();
                }
            private:
                const FolderScanCancellationToken& token;
            };

            FolderScanResult cancelledResult(FolderScanResult out)
            {
                out.status = FolderScanStatus::Cancelled;
                out.paths.clear();
                return out;
            }

            FolderScanResult scanFolderWindows(
                const std::filesystem::path& root,
                const CompiledFileFilter& filter,
                const FolderScanOptions& options,
                const FolderScanCancellationToken& cancellation)
            {
                FolderScanResult out;
                CancellationBinding cancellationBinding(cancellation);
                if (cancellation.isCancellationRequested())
                {
                    return cancelledResult(std::move(out));
                }

                std::error_code error;
                WinHandle rootHandle = openRootDirectory(root, true, error);
                if (!rootHandle)
                {
                    if (cancellation.isCancellationRequested() ||
                        ERROR_OPERATION_ABORTED == error.value())
                    {
                        return cancelledResult(std::move(out));
                    }
                    out.status = FolderScanStatus::InvalidRoot;
                    addWarning(out, warningCode(error), root, error,
                        "Cannot open scan root", options.maxWarnings);
                    return out;
                }
                FILE_ATTRIBUTE_TAG_INFO rootAttributes = {};
                if (!directoryAttributes(rootHandle.get(), rootAttributes) ||
                    0 == (rootAttributes.FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    error = winError();
                    if (!error)
                    {
                        error = std::make_error_code(std::errc::not_a_directory);
                    }
                    out.status = FolderScanStatus::InvalidRoot;
                    addWarning(out, warningCode(error), root, error,
                        "Scan root is not a directory", options.maxWarnings);
                    return out;
                }
                const bool rootIsReparse =
                    0 != (rootAttributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
                if (rootIsReparse && !options.followDirectoryLinks)
                {
                    out.status = FolderScanStatus::InvalidRoot;
                    addWarning(out, FolderScanWarningCode::LinkSkipped, root, {},
                        "Scan root is a link and link traversal is disabled",
                        options.maxWarnings);
                    return out;
                }
                if (rootIsReparse)
                {
                    rootHandle = openRootDirectory(root, false, error);
                    if (!rootHandle)
                    {
                        out.status = FolderScanStatus::InvalidRoot;
                        addWarning(out, warningCode(error), root, error,
                            "Cannot resolve scan root", options.maxWarnings);
                        return out;
                    }
                }

                DirectoryIdentity rootIdentity;
                if (!getIdentity(rootHandle.get(), rootIdentity))
                {
                    error = winError();
                    out.status = FolderScanStatus::InvalidRoot;
                    addWarning(out, warningCode(error), root, error,
                        "Cannot identify scan root", options.maxWarnings);
                    return out;
                }
                const std::wstring rootFinalPath = finalPath(rootHandle.get());
                if (rootFinalPath.empty())
                {
                    error = winError();
                    out.status = FolderScanStatus::InvalidRoot;
                    addWarning(out, warningCode(error), root, error,
                        "Cannot resolve final scan root", options.maxWarnings);
                    return out;
                }

                const auto fileExtensions = normalizeExtensions(options.fileExtensions);
                const auto sequenceExtensions = normalizeExtensions(options.sequenceExtensions);
                ftk::PathOptions pathOptions;
                pathOptions.seqMaxDigits = options.sequenceMaxDigits;

                std::vector<WinPendingDirectory> pending;
                pending.push_back({ root, 0, std::move(rootHandle), rootIdentity });
                std::vector<DirectoryIdentity> visitedDirectories;
                bool stop = false;
                while (!pending.empty() && !stop)
                {
                    if (cancellation.isCancellationRequested())
                    {
                        return cancelledResult(std::move(out));
                    }
                    WinPendingDirectory current = std::move(pending.back());
                    pending.pop_back();
                    if (std::find(
                        visitedDirectories.begin(),
                        visitedDirectories.end(),
                        current.identity) != visitedDirectories.end())
                    {
                        continue;
                    }
                    visitedDirectories.push_back(current.identity);
                    if (out.visitedDirectories >= options.maxDirectories)
                    {
                        out.status = FolderScanStatus::TraversalLimitReached;
                        addWarning(out, FolderScanWarningCode::TraversalLimitReached,
                            current.path, {}, "Folder scan directory limit reached",
                            options.maxWarnings);
                        break;
                    }
                    ++out.visitedDirectories;

                    struct Entry
                    {
                        std::wstring name;
                        DWORD attributes = 0;
                    };
                    std::vector<Entry> entries;
                    std::vector<unsigned char> buffer(64 * 1024);
                    while (true)
                    {
                        if (cancellation.isCancellationRequested())
                        {
                            return cancelledResult(std::move(out));
                        }
                        if (!GetFileInformationByHandleEx(
                            current.handle.get(),
                            FileIdBothDirectoryInfo,
                            buffer.data(),
                            static_cast<DWORD>(buffer.size())))
                        {
                            const DWORD value = GetLastError();
                            if (ERROR_NO_MORE_FILES == value)
                            {
                                break;
                            }
                            if (ERROR_OPERATION_ABORTED == value ||
                                cancellation.isCancellationRequested())
                            {
                                return cancelledResult(std::move(out));
                            }
                            error = winError(value);
                            addWarning(out, warningCode(error), current.path, error,
                                "Directory enumeration stopped early", options.maxWarnings);
                            break;
                        }
                        auto* info = reinterpret_cast<FILE_ID_BOTH_DIR_INFO*>(buffer.data());
                        while (info)
                        {
                            const std::wstring name(
                                info->FileName,
                                info->FileNameLength / sizeof(wchar_t));
                            if (L"." != name && L".." != name)
                            {
                                if (out.visitedEntries >= options.maxEntries)
                                {
                                    out.status = FolderScanStatus::TraversalLimitReached;
                                    addWarning(out, FolderScanWarningCode::TraversalLimitReached,
                                        current.path, {},
                                        "Folder scan global entry limit reached",
                                        options.maxWarnings);
                                    stop = true;
                                    break;
                                }
                                if (entries.size() >= options.maxDirectoryEntries)
                                {
                                    out.status = FolderScanStatus::CandidateLimitReached;
                                    addWarning(out, FolderScanWarningCode::CandidateLimitReached,
                                        current.path, {}, "Directory entry limit reached",
                                        options.maxWarnings);
                                    stop = true;
                                    break;
                                }
                                entries.push_back({ name, info->FileAttributes });
                                ++out.visitedEntries;
                            }
                            if (!info->NextEntryOffset || stop)
                            {
                                break;
                            }
                            info = reinterpret_cast<FILE_ID_BOTH_DIR_INFO*>(
                                reinterpret_cast<unsigned char*>(info) + info->NextEntryOffset);
                        }
                        if (stop)
                        {
                            break;
                        }
                    }
                    if (stop)
                    {
                        break;
                    }
                    std::sort(entries.begin(), entries.end(),
                        [](const Entry& a, const Entry& b) { return a.name < b.name; });

                    std::vector<WinPendingDirectory> childDirectories;
                    for (const auto& entry : entries)
                    {
                        if (cancellation.isCancellationRequested())
                        {
                            return cancelledResult(std::move(out));
                        }
                        const auto displayPath = current.path / entry.name;
                        const bool directory =
                            0 != (entry.attributes & FILE_ATTRIBUTE_DIRECTORY);
                        const bool reparse =
                            0 != (entry.attributes & FILE_ATTRIBUTE_REPARSE_POINT);
                        if (directory)
                        {
                            if (!options.recursive ||
                                filter.excludesDirectory(ftk::Path(pathString(displayPath))))
                            {
                                continue;
                            }
                            if (current.depth >= options.maxDepth)
                            {
                                addWarning(out, FolderScanWarningCode::DepthLimitReached,
                                    displayPath, {}, "Directory depth limit reached",
                                    options.maxWarnings);
                                continue;
                            }
                            WinHandle child = openRelativeDirectory(
                                current.handle.get(), entry.name, true, error);
                            if (!child)
                            {
                                if (cancellation.isCancellationRequested() ||
                                    ERROR_OPERATION_ABORTED == error.value())
                                {
                                    return cancelledResult(std::move(out));
                                }
                                addWarning(out, warningCode(error), displayPath, error,
                                    "Cannot open directory entry", options.maxWarnings);
                                continue;
                            }
                            FILE_ATTRIBUTE_TAG_INFO actualAttributes = {};
                            if (!directoryAttributes(child.get(), actualAttributes))
                            {
                                error = winError();
                                addWarning(out, warningCode(error), displayPath, error,
                                    "Cannot inspect opened directory", options.maxWarnings);
                                continue;
                            }
                            const bool actualDirectory = 0 !=
                                (actualAttributes.FileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                            const bool actualReparse = 0 !=
                                (actualAttributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
                            if (!actualDirectory || reparse != actualReparse)
                            {
                                addWarning(out, FolderScanWarningCode::LinkSkipped,
                                    displayPath, {},
                                    "Directory identity changed during traversal",
                                    options.maxWarnings);
                                continue;
                            }
                            if (actualReparse)
                            {
                                if (!options.followDirectoryLinks)
                                {
                                    addWarning(out, FolderScanWarningCode::LinkSkipped,
                                        displayPath, {}, "Link traversal is disabled",
                                        options.maxWarnings);
                                    continue;
                                }
                                child = openRelativeDirectory(
                                    current.handle.get(), entry.name, false, error);
                                if (!child)
                                {
                                    addWarning(out, warningCode(error), displayPath, error,
                                        "Cannot resolve directory link", options.maxWarnings);
                                    continue;
                                }
                                const std::wstring childFinalPath = finalPath(child.get());
                                if (childFinalPath.empty() ||
                                    !isWithinRoot(rootFinalPath, childFinalPath))
                                {
                                    addWarning(out, FolderScanWarningCode::LinkSkipped,
                                        displayPath, {},
                                        "Directory link escapes the scan root",
                                        options.maxWarnings);
                                    continue;
                                }
                            }
                            DirectoryIdentity identity;
                            if (!getIdentity(child.get(), identity))
                            {
                                error = winError();
                                addWarning(out, warningCode(error), displayPath, error,
                                    "Cannot identify opened directory", options.maxWarnings);
                                continue;
                            }
                            childDirectories.push_back(
                                { displayPath, current.depth + 1, std::move(child), identity });
                        }
                        else
                        {
                            if (reparse && !options.followDirectoryLinks)
                            {
                                addWarning(out, FolderScanWarningCode::LinkSkipped,
                                    displayPath, {}, "Link traversal is disabled",
                                    options.maxWarnings);
                                continue;
                            }
                            if (out.visitedFiles >= options.maxCandidates)
                            {
                                out.status = FolderScanStatus::CandidateLimitReached;
                                addWarning(out, FolderScanWarningCode::CandidateLimitReached,
                                    displayPath, {}, "Folder scan candidate limit reached",
                                    options.maxWarnings);
                                stop = true;
                                break;
                            }
                            ++out.visitedFiles;
                            const std::string value = pathString(displayPath);
                            if (value.size() > CompiledFileFilter::maxCandidateLength)
                            {
                                addWarning(out, FolderScanWarningCode::CandidateTooLong,
                                    displayPath, {}, "Candidate path is too long",
                                    options.maxWarnings);
                                continue;
                            }
                            const ftk::Path path(value, pathOptions);
                            const std::string extension = normalizeExtension(path.getExt());
                            if ((!fileExtensions.empty() && !fileExtensions.count(extension)) ||
                                !filter.matches(path))
                            {
                                continue;
                            }
                            out.paths.push_back(path);
                        }
                    }
                    for (auto i = childDirectories.rbegin();
                        i != childDirectories.rend(); ++i)
                    {
                        pending.push_back(std::move(*i));
                    }
                }

                std::sort(out.paths.begin(), out.paths.end(),
                    [](const ftk::Path& a, const ftk::Path& b) { return lessPath(a, b); });
                if (FolderScanStatus::CandidateLimitReached == out.status ||
                    FolderScanStatus::TraversalLimitReached == out.status)
                {
                    out.paths.clear();
                }
                if (options.collapseSequences && !sequenceExtensions.empty())
                {
                    out.paths = collapseSequences(out.paths, sequenceExtensions);
                }
                if (FolderScanStatus::Completed == out.status &&
                    options.maxResults > 0 && out.paths.size() > options.maxResults)
                {
                    out.status = FolderScanStatus::ResultLimitReached;
                    addWarning(out, FolderScanWarningCode::ResultLimitReached,
                        root, {}, "Folder scan result limit reached; use a narrower filter",
                        options.maxWarnings);
                    out.paths.clear();
                }
                return out;
            }
        }
#endif

        FolderScanResult scanFolder(
            const std::filesystem::path& root,
            const CompiledFileFilter& filter,
            const FolderScanOptions& options,
            const FolderScanCancellationToken& cancellation)
        {
#if defined(_WIN32)
            return scanFolderWindows(root, filter, options, cancellation);
#else
            FolderScanResult out;
            if (cancellation.isCancellationRequested())
            {
                out.status = FolderScanStatus::Cancelled;
                return out;
            }

            std::error_code error;
            const auto rootLinkStatus = std::filesystem::symlink_status(root, error);
            if (error)
            {
                out.status = FolderScanStatus::InvalidRoot;
                addWarning(out, warningCode(error), root, error, "Cannot inspect scan root", options.maxWarnings);
                return out;
            }
            if (isLink(root, rootLinkStatus) && !options.followDirectoryLinks)
            {
                out.status = FolderScanStatus::InvalidRoot;
                addWarning(
                    out,
                    FolderScanWarningCode::LinkSkipped,
                    root,
                    std::error_code(),
                    "Scan root is a link and link traversal is disabled",
                    options.maxWarnings);
                return out;
            }
            const auto rootStatus = std::filesystem::status(root, error);
            if (error || !std::filesystem::is_directory(rootStatus))
            {
                if (!error)
                {
                    error = std::make_error_code(std::errc::not_a_directory);
                }
                out.status = FolderScanStatus::InvalidRoot;
                addWarning(out, warningCode(error), root, error, "Scan root is not a directory", options.maxWarnings);
                return out;
            }

            const auto fileExtensions = normalizeExtensions(options.fileExtensions);
            const auto sequenceExtensions = normalizeExtensions(options.sequenceExtensions);
            ftk::PathOptions pathOptions;
            pathOptions.seqMaxDigits = options.sequenceMaxDigits;

            std::vector<PendingDirectory> pending = { { root, 0 } };
            std::unordered_set<std::string> visitedDirectories;
            bool stop = false;
            while (!pending.empty() && !stop)
            {
                if (cancellation.isCancellationRequested())
                {
                    out.status = FolderScanStatus::Cancelled;
                    out.paths.clear();
                    return out;
                }

                const PendingDirectory current = pending.back();
                pending.pop_back();
                if (!visitedDirectories.insert(canonicalKey(current.path)).second)
                {
                    continue;
                }
                if (out.visitedDirectories >= options.maxDirectories)
                {
                    out.status = FolderScanStatus::TraversalLimitReached;
                    addWarning(
                        out,
                        FolderScanWarningCode::TraversalLimitReached,
                        current.path,
                        std::error_code(),
                        "Folder scan directory limit reached",
                        options.maxWarnings);
                    stop = true;
                    break;
                }
                ++out.visitedDirectories;

                std::vector<std::filesystem::path> entries;
                std::filesystem::directory_iterator iterator(
                    current.path,
                    std::filesystem::directory_options::none,
                    error);
                if (error)
                {
                    addWarning(
                        out,
                        warningCode(error),
                        current.path,
                        error,
                        "Cannot enumerate directory",
                        options.maxWarnings);
                    error.clear();
                    continue;
                }
                const std::filesystem::directory_iterator end;
                while (iterator != end)
                {
                    if (out.visitedEntries >= options.maxEntries)
                    {
                        out.status = FolderScanStatus::TraversalLimitReached;
                        addWarning(
                            out,
                            FolderScanWarningCode::TraversalLimitReached,
                            current.path,
                            std::error_code(),
                            "Folder scan global entry limit reached",
                            options.maxWarnings);
                        stop = true;
                        break;
                    }
                    if (entries.size() >= options.maxDirectoryEntries)
                    {
                        out.status = FolderScanStatus::CandidateLimitReached;
                        addWarning(
                            out,
                            FolderScanWarningCode::CandidateLimitReached,
                            current.path,
                            std::error_code(),
                            "Directory entry limit reached",
                            options.maxWarnings);
                        stop = true;
                        break;
                    }
                    entries.push_back(iterator->path());
                    ++out.visitedEntries;
                    iterator.increment(error);
                    if (error)
                    {
                        addWarning(
                            out,
                            warningCode(error),
                            current.path,
                            error,
                            "Directory enumeration stopped early",
                            options.maxWarnings);
                        error.clear();
                        break;
                    }
                }
                if (stop)
                {
                    break;
                }
                std::sort(
                    entries.begin(),
                    entries.end(),
                    [](const std::filesystem::path& a, const std::filesystem::path& b)
                    {
                        return lessPath(a, b);
                    });

                std::vector<PendingDirectory> childDirectories;
                for (const auto& entry : entries)
                {
                    if (cancellation.isCancellationRequested())
                    {
                        out.status = FolderScanStatus::Cancelled;
                        out.paths.clear();
                        return out;
                    }

                    const auto linkStatus = std::filesystem::symlink_status(entry, error);
                    if (error)
                    {
                        addWarning(out, warningCode(error), entry, error, "Cannot inspect entry", options.maxWarnings);
                        error.clear();
                        continue;
                    }
                    const bool link = isLink(entry, linkStatus);
                    if (link && !options.followDirectoryLinks)
                    {
                        addWarning(
                            out,
                            FolderScanWarningCode::LinkSkipped,
                            entry,
                            std::error_code(),
                            "Link traversal is disabled",
                            options.maxWarnings);
                        continue;
                    }

                    const auto status = link ? std::filesystem::status(entry, error) : linkStatus;
                    if (error)
                    {
                        addWarning(out, warningCode(error), entry, error, "Cannot resolve entry", options.maxWarnings);
                        error.clear();
                        continue;
                    }
                    if (std::filesystem::is_directory(status))
                    {
                        if (!options.recursive || filter.excludesDirectory(ftk::Path(pathString(entry))))
                        {
                            continue;
                        }
                        if (current.depth >= options.maxDepth)
                        {
                            addWarning(
                                out,
                                FolderScanWarningCode::DepthLimitReached,
                                entry,
                                std::error_code(),
                                "Directory depth limit reached",
                                options.maxWarnings);
                            continue;
                        }
                        childDirectories.push_back({ entry, current.depth + 1 });
                    }
                    else if (std::filesystem::is_regular_file(status))
                    {
                        if (out.visitedFiles >= options.maxCandidates)
                        {
                            out.status = FolderScanStatus::CandidateLimitReached;
                            addWarning(
                                out,
                                FolderScanWarningCode::CandidateLimitReached,
                                entry,
                                std::error_code(),
                                "Folder scan candidate limit reached",
                                options.maxWarnings);
                            stop = true;
                            break;
                        }
                        ++out.visitedFiles;
                        const std::string value = pathString(entry);
                        if (value.size() > CompiledFileFilter::maxCandidateLength)
                        {
                            addWarning(
                                out,
                                FolderScanWarningCode::CandidateTooLong,
                                entry,
                                std::error_code(),
                                "Candidate path is too long",
                                options.maxWarnings);
                            continue;
                        }

                        const ftk::Path path(value, pathOptions);
                        const std::string extension = normalizeExtension(path.getExt());
                        if ((!fileExtensions.empty() && !fileExtensions.count(extension)) ||
                            !filter.matches(path))
                        {
                            continue;
                        }
                        out.paths.push_back(path);
                    }
                }

                for (auto i = childDirectories.rbegin(); i != childDirectories.rend(); ++i)
                {
                    pending.push_back(*i);
                }
            }

            std::sort(
                out.paths.begin(),
                out.paths.end(),
                [](const ftk::Path& a, const ftk::Path& b)
                {
                    return lessPath(a, b);
                });
            if (FolderScanStatus::CandidateLimitReached == out.status ||
                FolderScanStatus::TraversalLimitReached == out.status)
            {
                // A bounded scan is an atomic result; never expose a partial
                // file set that a caller could mistake for a complete folder.
                out.paths.clear();
            }
            if (options.collapseSequences && !sequenceExtensions.empty())
            {
                out.paths = collapseSequences(out.paths, sequenceExtensions);
            }
            if (FolderScanStatus::Completed == out.status &&
                options.maxResults > 0 && out.paths.size() > options.maxResults)
            {
                out.status = FolderScanStatus::ResultLimitReached;
                addWarning(
                    out,
                    FolderScanWarningCode::ResultLimitReached,
                    root,
                    std::error_code(),
                    "Folder scan result limit reached; use a narrower filter",
                    options.maxWarnings);
                out.paths.clear();
            }
            return out;
#endif
        }
    }
}
