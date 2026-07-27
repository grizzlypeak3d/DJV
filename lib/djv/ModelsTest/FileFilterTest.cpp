// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/FileFilterTest.h>

#include <djv/Models/FileFilter.h>
#include <djv/Models/FolderScanner.h>

#include <chrono>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <winioctl.h>
#endif
namespace djv
{
    namespace models_tests
    {
        namespace
        {
            void require(bool value, const std::string& message)
            {
                if (!value)
                {
                    throw std::runtime_error(message);
                }
            }

            std::shared_ptr<const models::CompiledFileFilter> compile(
                const std::string& expression)
            {
                const auto result = models::compileFileFilter(expression);
                require(static_cast<bool>(result), "Filter failed to compile: " + expression);
                return result.filter;
            }

            void touch(const std::filesystem::path& path)
            {
                std::filesystem::create_directories(path.parent_path());
                std::ofstream stream(path, std::ios::binary);
                stream << "x";
            }

            class TmpDir
            {
            public:
                TmpDir()
                {
                    const auto id = std::chrono::steady_clock::now().time_since_epoch().count();
                    _path = std::filesystem::temp_directory_path() /
                        ("djv-file-filter-test-" + std::to_string(id));
                    std::filesystem::create_directories(_path);
                }

                ~TmpDir()
                {
                    std::error_code error;
                    std::filesystem::remove_all(_path, error);
                }

                const std::filesystem::path& getPath() const { return _path; }

            private:
                std::filesystem::path _path;
            };

#if defined(_WIN32)
            bool createDirectoryJunction(
                const std::filesystem::path& link,
                const std::filesystem::path& target)
            {
                struct MountPointReparseData
                {
                    DWORD tag;
                    WORD dataLength;
                    WORD reserved;
                    WORD substituteOffset;
                    WORD substituteLength;
                    WORD printOffset;
                    WORD printLength;
                    wchar_t pathBuffer[1];
                };
                if (!CreateDirectoryW(link.c_str(), nullptr))
                {
                    return false;
                }
                const HANDLE handle = CreateFileW(
                    link.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                    FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
                    nullptr);
                if (INVALID_HANDLE_VALUE == handle)
                {
                    RemoveDirectoryW(link.c_str());
                    return false;
                }
                const std::wstring printName =
                    std::filesystem::absolute(target).lexically_normal().wstring();
                const std::wstring substituteName = L"\\??\\" + printName;
                const size_t substituteBytes = substituteName.size() * sizeof(wchar_t);
                const size_t printBytes = printName.size() * sizeof(wchar_t);
                std::vector<unsigned char> buffer(MAXIMUM_REPARSE_DATA_BUFFER_SIZE, 0);
                auto* data = reinterpret_cast<MountPointReparseData*>(buffer.data());
                data->tag = IO_REPARSE_TAG_MOUNT_POINT;
                data->substituteLength = static_cast<WORD>(substituteBytes);
                data->printOffset = static_cast<WORD>(substituteBytes + sizeof(wchar_t));
                data->printLength = static_cast<WORD>(printBytes);
                std::memcpy(data->pathBuffer, substituteName.c_str(), substituteBytes);
                std::memcpy(
                    reinterpret_cast<unsigned char*>(data->pathBuffer) +
                        substituteBytes + sizeof(wchar_t),
                    printName.c_str(), printBytes);
                const size_t inputLength = offsetof(MountPointReparseData, pathBuffer) +
                    substituteBytes + sizeof(wchar_t) + printBytes + sizeof(wchar_t);
                data->dataLength = static_cast<WORD>(inputLength - 8);
                DWORD returned = 0;
                const BOOL ok = DeviceIoControl(
                    handle, FSCTL_SET_REPARSE_POINT, data,
                    static_cast<DWORD>(inputLength), nullptr, 0, &returned, nullptr);
                CloseHandle(handle);
                if (!ok)
                {
                    RemoveDirectoryW(link.c_str());
                }
                return !!ok;
            }

            void concurrentReparseReplacement()
            {
                TmpDir scan;
                TmpDir outside;
                const auto victim = scan.getPath() / "victim";
                const auto held = scan.getPath() / "held";
                touch(victim / "inside.exr");
                touch(outside.getPath() / "outside.exr");
                if (!createDirectoryJunction(scan.getPath() / "probe", outside.getPath()))
                {
                    // Junction creation may be denied by machine policy. The
                    // static reparse test remains covered elsewhere.
                    return;
                }
                RemoveDirectoryW((scan.getPath() / "probe").c_str());

                std::atomic_bool stop(false);
                std::thread replacer([&]
                {
                    while (!stop.load(std::memory_order_acquire))
                    {
                        std::error_code error;
                        std::filesystem::rename(victim, held, error);
                        if (!error)
                        {
                            if (createDirectoryJunction(victim, outside.getPath()))
                            {
                                std::this_thread::yield();
                                RemoveDirectoryW(victim.c_str());
                            }
                            error.clear();
                            std::filesystem::rename(held, victim, error);
                        }
                        std::this_thread::yield();
                    }
                });

                const auto filter = compile("");
                for (size_t i = 0; i < 100; ++i)
                {
                    const auto result = models::scanFolder(scan.getPath(), *filter);
                    for (const auto& path : result.paths)
                    {
                        require(
                            std::string::npos == path.get().find("outside.exr"),
                            "Concurrent junction replacement escaped the scan root");
                    }
                }
                stop.store(true, std::memory_order_release);
                replacer.join();
                RemoveDirectoryW(victim.c_str());
                std::error_code error;
                if (std::filesystem::exists(held, error))
                {
                    std::filesystem::rename(held, victim, error);
                }
            }
#endif

            void grammar()
            {
                const auto filter = compile(
                    "ext:^(exr|mov)$ dir:(shot010|shot020) -name:(proxy|thumb)");
                require(filter->matches(ftk::Path("/show/shot010/beauty.0001.exr")),
                    "Expected EXR to match");
                require(!filter->matches(ftk::Path("/show/shot010/beauty.proxy.0001.exr")),
                    "Excluded proxy matched");
                require(!filter->matches(ftk::Path("/show/shot030/beauty.0001.exr")),
                    "Wrong directory matched");
                require(!filter->matches(ftk::Path("/show/shot010/beauty.wav")),
                    "Wrong extension matched");

                const auto aliases = compile("file:beauty extension:^exr$ folder:shot010");
                require(aliases->matches(ftk::Path("/show/shot010/beauty.0001.EXR")),
                    "Aliases or ASCII case-insensitive extension failed");

                const std::string unicodeName =
                    "r\xC3\xA9sum\xC3\xA9.0001.exr";
                require(compile("name:r\xC3\xA9sum\xC3\xA9")->matches(
                    ftk::Path("/show/" + unicodeName)),
                    "UTF-8 literal filter failed");
            }

            void invalidGrammar()
            {
                struct Case
                {
                    std::string expression;
                    models::FileFilterErrorCode code;
                };
                const std::vector<Case> cases =
                {
                    { "name:", models::FileFilterErrorCode::EmptyPattern },
                    { "-", models::FileFilterErrorCode::EmptyPattern },
                    { "mime:exr", models::FileFilterErrorCode::UnknownTarget },
                    { "name:[", models::FileFilterErrorCode::InvalidRegularExpression },
                    { "name:(a+)+$", models::FileFilterErrorCode::UnsafeRegularExpression },
                    { "name:a*a*a*b", models::FileFilterErrorCode::UnsafeRegularExpression },
                    { "name:(a)\\1", models::FileFilterErrorCode::UnsafeRegularExpression },
                    { std::string(models::CompiledFileFilter::maxExpressionLength + 1, 'a'),
                        models::FileFilterErrorCode::ExpressionTooLong },
                    { "name:" + std::string(models::CompiledFileFilter::maxPatternLength + 1, 'a'),
                        models::FileFilterErrorCode::PatternTooLong }
                };
                for (const auto& test : cases)
                {
                    const auto result = models::compileFileFilter(test.expression);
                    require(!result, "Invalid expression compiled: " + test.expression);
                    require(result.error.has_value(), "Missing compile error");
                    require(test.code == result.error->code, "Wrong compile error code");
                }

                std::string tooManyTerms;
                for (size_t i = 0; i < models::CompiledFileFilter::maxTermCount + 1; ++i)
                {
                    tooManyTerms += (i ? " a" : "a");
                }
                const auto result = models::compileFileFilter(tooManyTerms);
                require(!result, "Too many terms compiled");
                require(result.error &&
                    models::FileFilterErrorCode::TooManyTerms == result.error->code,
                    "Wrong too-many-terms error");
            }

            void scanner()
            {
                TmpDir tmp;
                const auto root = tmp.getPath();
                touch(root / "shot020" / "plate.0002.exr");
                touch(root / "shot010" / "plate.0002.exr");
                touch(root / "shot010" / "plate.0001.exr");
                touch(root / "shot010" / "plate.0003.exr");
                touch(root / "shot010" / "plate.mov");
                touch(root / "cache" / "plate.0001.exr");
                touch(root / "shot010" / "plate.0004.txt");

                models::FolderScanOptions options;
                options.fileExtensions = { ".exr", "mov" };
                options.collapseSequences = true;
                options.sequenceExtensions = { "EXR" };
                const auto filter = compile("-dir:^cache$");
                const auto result = models::scanFolder(root, *filter, options);
                require(models::FolderScanStatus::Completed == result.status,
                    "Folder scan did not complete");
                require(3 == result.paths.size(), "Unexpected scan result count");
                require(result.paths[0].get() < result.paths[1].get() &&
                    result.paths[1].get() < result.paths[2].get(),
                    "Folder scan result is not sorted");

                size_t sequenceCount = 0;
                for (const auto& path : result.paths)
                {
                    if (path.isSeq())
                    {
                        ++sequenceCount;
                        require(path.getFrames().has_value(), "Sequence has no frame range");
                        require(1 == path.getFrames()->min() && 3 == path.getFrames()->max(),
                            "Wrong sequence frame range");
                    }
                    require(std::string::npos == path.get().find("cache"),
                        "Excluded directory was scanned");
                    require(".txt" != path.getExt(), "Extension allowlist failed");
                }
                require(1 == sequenceCount, "EXR sequence was not collapsed exactly once");

                // A path expression that matches a directory itself is not
                // necessarily inherited by its children. It must never be used
                // for unsafe subtree pruning.
                const auto pathAnchorFilter = compile("-path:cache$");
                const auto pathAnchorResult = models::scanFolder(
                    root, *pathAnchorFilter, options);
                bool cacheChildFound = false;
                for (const auto& path : pathAnchorResult.paths)
                {
                    cacheChildFound |= std::string::npos != path.get().find("cache");
                }
                require(cacheChildFound,
                    "Path-only exclusion incorrectly pruned a directory subtree");

                const auto result2 = models::scanFolder(root, *filter, options);
                require(result.paths == result2.paths, "Repeated scan order differs");
            }

            void cancellationAndLimits()
            {
                TmpDir tmp;
                touch(tmp.getPath() / "a.exr");

                const auto filter = compile("");
                models::FolderScanCancellationSource cancellation;
                cancellation.cancel();
                const auto cancelled = models::scanFolder(
                    tmp.getPath(), *filter, {}, cancellation.getToken());
                require(models::FolderScanStatus::Cancelled == cancelled.status,
                    "Pre-cancelled scan did not cancel");
                require(cancelled.paths.empty(), "Cancelled scan returned paths");

                models::FolderScanOptions options;
                options.maxCandidates = 0;
                const auto limited = models::scanFolder(tmp.getPath(), *filter, options);
                require(models::FolderScanStatus::CandidateLimitReached == limited.status,
                    "Candidate limit was not enforced");
                require(limited.paths.empty(), "Candidate-limited scan exposed partial results");
                require(!limited.warnings.empty() &&
                    models::FolderScanWarningCode::CandidateLimitReached ==
                        limited.warnings.back().code,
                    "Candidate limit warning missing");

                std::filesystem::create_directories(tmp.getPath() / "empty-a");
                std::filesystem::create_directories(tmp.getPath() / "empty-b");
                options.maxCandidates = 100;
                options.maxDirectories = 2;
                const auto traversalLimited = models::scanFolder(
                    tmp.getPath(), *filter, options);
                require(models::FolderScanStatus::TraversalLimitReached ==
                    traversalLimited.status,
                    "Global directory traversal limit was not enforced");
                require(traversalLimited.paths.empty(),
                    "Traversal-limited scan exposed partial results");
                require(!traversalLimited.warnings.empty() &&
                    models::FolderScanWarningCode::TraversalLimitReached ==
                        traversalLimited.warnings.back().code,
                    "Traversal limit warning missing");

                options.maxDirectories = 100000;
                options.maxEntries = 2;
                const auto entryLimited = models::scanFolder(
                    tmp.getPath(), *filter, options);
                require(models::FolderScanStatus::TraversalLimitReached ==
                    entryLimited.status,
                    "Global entry traversal limit was not enforced");
                require(2 == entryLimited.visitedEntries,
                    "Global entry traversal accounting is incorrect");
                require(entryLimited.paths.empty(),
                    "Entry-limited scan exposed partial results");

                touch(tmp.getPath() / "b.exr");
                touch(tmp.getPath() / "c.exr");
                options.maxCandidates = 100;
                options.maxDirectories = 100000;
                options.maxEntries = 1000000;
                options.maxResults = 2;
                const auto resultLimited = models::scanFolder(
                    tmp.getPath(), *filter, options);
                require(models::FolderScanStatus::ResultLimitReached ==
                    resultLimited.status,
                    "Result limit was not enforced");
                require(resultLimited.paths.empty(),
                    "Result-limited scan exposed partial results");
            }

            void warningsAndInvalidRoot()
            {
                TmpDir tmp;
                touch(tmp.getPath() / "real" / "a.exr");
                const auto filter = compile("");

                const auto missing = models::scanFolder(
                    tmp.getPath() / "missing", *filter);
                require(models::FolderScanStatus::InvalidRoot == missing.status,
                    "Missing root was not rejected");
                require(!missing.warnings.empty(), "Missing root warning absent");

                std::error_code error;
                std::filesystem::create_directory_symlink(
                    tmp.getPath() / "real",
                    tmp.getPath() / "linked",
                    error);
                if (!error)
                {
                    const auto linked = models::scanFolder(tmp.getPath(), *filter);
                    bool foundLinkWarning = false;
                    for (const auto& warning : linked.warnings)
                    {
                        foundLinkWarning |=
                            models::FolderScanWarningCode::LinkSkipped == warning.code;
                    }
                    require(foundLinkWarning, "Skipped link warning absent");
                    require(1 == linked.paths.size(), "Directory link was followed by default");
                }

#if !defined(_WIN32)
                const auto deniedPath = tmp.getPath() / "denied";
                touch(deniedPath / "private.exr");
                std::filesystem::permissions(
                    deniedPath,
                    std::filesystem::perms::none,
                    std::filesystem::perm_options::replace,
                    error);
                if (!error)
                {
                    std::error_code probeError;
                    std::filesystem::directory_iterator probe(deniedPath, probeError);
                    (void)probe;
                    if (probeError == std::errc::permission_denied)
                    {
                        const auto denied = models::scanFolder(tmp.getPath(), *filter);
                        bool foundPermissionWarning = false;
                        for (const auto& warning : denied.warnings)
                        {
                            foundPermissionWarning |=
                                models::FolderScanWarningCode::PermissionDenied == warning.code;
                        }
                        require(foundPermissionWarning, "Permission warning absent");
                    }
                    std::filesystem::permissions(
                        deniedPath,
                        std::filesystem::perms::owner_all,
                        std::filesystem::perm_options::replace,
                        error);
                }
#endif
            }

            void benchmark()
            {
                const auto filter = compile("ext:^exr$ name:^plate\\.[0-9]{4}\\.exr$");
                const auto start = std::chrono::steady_clock::now();
                size_t matches = 0;
                for (size_t i = 0; i < 10000; ++i)
                {
                    const std::string number = std::to_string(10000 + i).substr(1);
                    matches += filter->matches(ftk::Path(
                        "/show/shot010/plate." + number + ".exr")) ? 1 : 0;
                }
                const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start);
                require(10000 == matches, "10k synthetic path benchmark mismatched");
                require(elapsed < std::chrono::seconds(5),
                    "10k synthetic path benchmark exceeded five seconds");
                std::cout << "10k-path filter benchmark: " << elapsed.count() << " ms\n";
            }
        }

        void FileFilterTest::run()
        {
            grammar();
            invalidGrammar();
            scanner();
            cancellationAndLimits();
            warningsAndInvalidRoot();
#if defined(_WIN32)
            concurrentReparseReplacement();
#endif
            benchmark();
        }
    }
}
#if defined(DJV_FILE_FILTER_TEST_STANDALONE)
int main()
{
    try
    {
        djv::models_tests::FileFilterTest::run();
        std::cout << "FileFilterTest: PASS\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FileFilterTest: FAIL: " << e.what() << '\n';
        return 1;
    }
}
#endif
