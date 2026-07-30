// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/WindowsOpenBroker.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

#if defined(_WIN32)
#    if !defined(NOMINMAX)
#        define NOMINMAX
#    endif
#    include <windows.h>
#    include <aclapi.h>
#    include <sddl.h>

#    pragma comment(lib, "Advapi32.lib")
#endif // _WIN32

namespace djv
{
    namespace app
    {
        namespace
        {
            constexpr uint8_t protocolMagic[] = { 'D', 'J', 'V', 'O' };
            constexpr uint16_t requestMessageType = 1;
            constexpr uint16_t responseMessageType = 2;
            constexpr size_t protocolHeaderBytes = 20;
            constexpr uint8_t responseReceipt = 0xA5;

            enum class ResponseCode : uint32_t
            {
                Accepted = 0,
                InvalidRequest = 1,
                Unauthorized = 2,
                CallbackRejected = 3,
                ServerStopping = 4,
                InternalError = 5,
                ProtocolError = 6,
                PayloadTooLarge = 7
            };

            void appendU16(std::vector<uint8_t>& out, uint16_t value)
            {
                out.push_back(static_cast<uint8_t>(value & 0xFFU));
                out.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
            }

            void appendU32(std::vector<uint8_t>& out, uint32_t value)
            {
                for (unsigned int i = 0; i < 4; ++i)
                {
                    out.push_back(static_cast<uint8_t>((value >> (i * 8U)) & 0xFFU));
                }
            }

            void appendU64(std::vector<uint8_t>& out, uint64_t value)
            {
                for (unsigned int i = 0; i < 8; ++i)
                {
                    out.push_back(static_cast<uint8_t>((value >> (i * 8U)) & 0xFFU));
                }
            }

            bool readU16(
                const std::vector<uint8_t>& data,
                size_t& offset,
                uint16_t& value)
            {
                if (offset > data.size() || data.size() - offset < 2)
                {
                    return false;
                }
                value =
                    static_cast<uint16_t>(data[offset]) |
                    static_cast<uint16_t>(data[offset + 1]) << 8U;
                offset += 2;
                return true;
            }

            bool readU32(
                const std::vector<uint8_t>& data,
                size_t& offset,
                uint32_t& value)
            {
                if (offset > data.size() || data.size() - offset < 4)
                {
                    return false;
                }
                value = 0;
                for (unsigned int i = 0; i < 4; ++i)
                {
                    value |= static_cast<uint32_t>(data[offset + i]) << (i * 8U);
                }
                offset += 4;
                return true;
            }

            bool readU64(
                const std::vector<uint8_t>& data,
                size_t& offset,
                uint64_t& value)
            {
                if (offset > data.size() || data.size() - offset < 8)
                {
                    return false;
                }
                value = 0;
                for (unsigned int i = 0; i < 8; ++i)
                {
                    value |= static_cast<uint64_t>(data[offset + i]) << (i * 8U);
                }
                offset += 8;
                return true;
            }

            bool validateOptions(
                const WindowsOpenBrokerOptions& options,
                std::string& error)
            {
                if (options.applicationId.empty() || options.applicationId.size() > 256)
                {
                    error = "The application identifier must contain 1-256 UTF-8 bytes";
                    return false;
                }
                if (std::string::npos != options.applicationId.find('\0'))
                {
                    error = "The application identifier contains an embedded NUL";
                    return false;
                }
                const auto maxTimeout = std::chrono::hours(1);
                if (options.connectTimeout.count() <= 0 ||
                    options.ioTimeout.count() <= 0 ||
                    options.startTimeout.count() <= 0 ||
                    options.connectTimeout > maxTimeout ||
                    options.ioTimeout > maxTimeout ||
                    options.startTimeout > maxTimeout)
                {
                    error = "Broker timeouts must be between 1 millisecond and 1 hour";
                    return false;
                }
                return true;
            }

#if defined(_WIN32)
            struct Handle
            {
                Handle() = default;
                explicit Handle(HANDLE value) : value(value) {}
                ~Handle()
                {
                    reset();
                }

                Handle(const Handle&) = delete;
                Handle& operator=(const Handle&) = delete;

                Handle(Handle&& other) noexcept : value(other.release()) {}
                Handle& operator=(Handle&& other) noexcept
                {
                    if (this != &other)
                    {
                        reset(other.release());
                    }
                    return *this;
                }

                explicit operator bool() const noexcept
                {
                    return value && INVALID_HANDLE_VALUE != value;
                }

                HANDLE get() const noexcept
                {
                    return value;
                }

                HANDLE release() noexcept
                {
                    const HANDLE out = value;
                    value = nullptr;
                    return out;
                }

                void reset(HANDLE newValue = nullptr) noexcept
                {
                    if (value && INVALID_HANDLE_VALUE != value)
                    {
                        ::CloseHandle(value);
                    }
                    value = newValue;
                }

                HANDLE value = nullptr;
            };

            struct LocalMemory
            {
                ~LocalMemory()
                {
                    if (value)
                    {
                        ::LocalFree(value);
                    }
                }

                LocalMemory(const LocalMemory&) = delete;
                LocalMemory& operator=(const LocalMemory&) = delete;
                LocalMemory() = default;

                HLOCAL value = nullptr;
            };

            bool utf8ToWide(
                const std::string& value,
                std::wstring& out,
                std::string& error)
            {
                if (std::string::npos != value.find('\0'))
                {
                    error = "UTF-8 text contains an embedded NUL";
                    return false;
                }
                if (value.empty())
                {
                    out.clear();
                    return true;
                }
                if (value.size() > static_cast<size_t>((std::numeric_limits<int>::max)()))
                {
                    error = "UTF-8 text is too large";
                    return false;
                }
                const int size = ::MultiByteToWideChar(
                    CP_UTF8,
                    MB_ERR_INVALID_CHARS,
                    value.data(),
                    static_cast<int>(value.size()),
                    nullptr,
                    0);
                if (size <= 0)
                {
                    error = "Text is not valid UTF-8";
                    return false;
                }
                std::wstring tmp(static_cast<size_t>(size), L'\0');
                if (::MultiByteToWideChar(
                        CP_UTF8,
                        MB_ERR_INVALID_CHARS,
                        value.data(),
                        static_cast<int>(value.size()),
                        tmp.data(),
                        size) != size)
                {
                    error = "UTF-8 conversion failed";
                    return false;
                }
                out = std::move(tmp);
                return true;
            }

            bool wideToUtf8(
                const std::wstring& value,
                std::string& out,
                std::string& error)
            {
                if (value.empty())
                {
                    out.clear();
                    return true;
                }
                if (value.size() > static_cast<size_t>((std::numeric_limits<int>::max)()))
                {
                    error = "UTF-16 text is too large";
                    return false;
                }
                const int size = ::WideCharToMultiByte(
                    CP_UTF8,
                    WC_ERR_INVALID_CHARS,
                    value.data(),
                    static_cast<int>(value.size()),
                    nullptr,
                    0,
                    nullptr,
                    nullptr);
                if (size <= 0)
                {
                    error = "Text contains invalid UTF-16";
                    return false;
                }
                std::string tmp(static_cast<size_t>(size), '\0');
                if (::WideCharToMultiByte(
                        CP_UTF8,
                        WC_ERR_INVALID_CHARS,
                        value.data(),
                        static_cast<int>(value.size()),
                        tmp.data(),
                        size,
                        nullptr,
                        nullptr) != size)
                {
                    error = "UTF-16 conversion failed";
                    return false;
                }
                out = std::move(tmp);
                return true;
            }

            std::string windowsError(const char* context, DWORD code)
            {
                LPWSTR text = nullptr;
                const DWORD count = ::FormatMessageW(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr,
                    code,
                    0,
                    reinterpret_cast<LPWSTR>(&text),
                    0,
                    nullptr);
                std::wstring wide;
                if (count && text)
                {
                    wide.assign(text, text + count);
                    ::LocalFree(text);
                    while (!wide.empty() &&
                           (wide.back() == L'\r' ||
                            wide.back() == L'\n' ||
                            wide.back() == L' '))
                    {
                        wide.pop_back();
                    }
                }
                std::string message;
                std::string conversionError;
                if (!wide.empty())
                {
                    wideToUtf8(wide, message, conversionError);
                }
                std::ostringstream stream;
                stream << context << " failed with Windows error " << code;
                if (!message.empty())
                {
                    stream << ": " << message;
                }
                return stream.str();
            }

            bool getTokenUserSid(
                HANDLE token,
                std::vector<uint8_t>& sid,
                std::string& error)
            {
                DWORD size = 0;
                ::GetTokenInformation(token, TokenUser, nullptr, 0, &size);
                if (!size || ERROR_INSUFFICIENT_BUFFER != ::GetLastError())
                {
                    error = windowsError("GetTokenInformation(size)", ::GetLastError());
                    return false;
                }
                std::vector<uint8_t> buffer(size);
                if (!::GetTokenInformation(
                        token,
                        TokenUser,
                        buffer.data(),
                        size,
                        &size))
                {
                    error = windowsError("GetTokenInformation", ::GetLastError());
                    return false;
                }
                const auto tokenUser = reinterpret_cast<const TOKEN_USER*>(buffer.data());
                if (!::IsValidSid(tokenUser->User.Sid))
                {
                    error = "The access token contains an invalid user SID";
                    return false;
                }
                const DWORD sidBytes = ::GetLengthSid(tokenUser->User.Sid);
                sid.resize(sidBytes);
                if (!::CopySid(sidBytes, sid.data(), tokenUser->User.Sid))
                {
                    error = windowsError("CopySid", ::GetLastError());
                    return false;
                }
                return true;
            }

            bool getTokenLogonSid(
                HANDLE token,
                std::vector<uint8_t>& sid,
                std::string& error)
            {
                DWORD size = 0;
                ::GetTokenInformation(token, TokenGroups, nullptr, 0, &size);
                if (!size || ERROR_INSUFFICIENT_BUFFER != ::GetLastError())
                {
                    error = windowsError(
                        "GetTokenInformation(groups size)", ::GetLastError());
                    return false;
                }
                std::vector<uint8_t> buffer(size);
                if (!::GetTokenInformation(
                        token,
                        TokenGroups,
                        buffer.data(),
                        size,
                        &size))
                {
                    error = windowsError(
                        "GetTokenInformation(groups)", ::GetLastError());
                    return false;
                }
                const auto groups =
                    reinterpret_cast<const TOKEN_GROUPS*>(buffer.data());
                for (DWORD i = 0; i < groups->GroupCount; ++i)
                {
                    const auto& group = groups->Groups[i];
                    if (SE_GROUP_LOGON_ID ==
                        (group.Attributes & SE_GROUP_LOGON_ID))
                    {
                        if (!::IsValidSid(group.Sid))
                        {
                            error = "The access token contains an invalid logon SID";
                            return false;
                        }
                        const DWORD sidBytes = ::GetLengthSid(group.Sid);
                        sid.resize(sidBytes);
                        if (!::CopySid(sidBytes, sid.data(), group.Sid))
                        {
                            error = windowsError("CopySid(logon)", ::GetLastError());
                            return false;
                        }
                        return true;
                    }
                }
                error = "The access token does not contain a logon SID";
                return false;
            }

            bool getProcessUserSid(
                HANDLE process,
                std::vector<uint8_t>& sid,
                std::string& error)
            {
                HANDLE rawToken = nullptr;
                if (!::OpenProcessToken(process, TOKEN_QUERY, &rawToken))
                {
                    error = windowsError("OpenProcessToken", ::GetLastError());
                    return false;
                }
                Handle token(rawToken);
                return getTokenUserSid(token.get(), sid, error);
            }

            bool getProcessLogonSid(
                HANDLE process,
                std::vector<uint8_t>& sid,
                std::string& error)
            {
                HANDLE rawToken = nullptr;
                if (!::OpenProcessToken(process, TOKEN_QUERY, &rawToken))
                {
                    error = windowsError(
                        "OpenProcessToken(logon)", ::GetLastError());
                    return false;
                }
                Handle token(rawToken);
                return getTokenLogonSid(token.get(), sid, error);
            }

            bool sidToString(
                PSID sid,
                std::wstring& out,
                std::string& error)
            {
                LPWSTR raw = nullptr;
                if (!::ConvertSidToStringSidW(sid, &raw))
                {
                    error = windowsError("ConvertSidToStringSidW", ::GetLastError());
                    return false;
                }
                LocalMemory memory;
                memory.value = raw;
                out = raw;
                return true;
            }

            uint64_t stableIdentityHash(
                const std::string& applicationId,
                const std::wstring& sid,
                uint32_t sessionId)
            {
                uint64_t value = 14695981039346656037ULL;
                const auto add = [&value](uint8_t byte)
                {
                    value ^= byte;
                    value *= 1099511628211ULL;
                };
                for (const unsigned char byte : applicationId)
                {
                    add(byte);
                }
                add(0);
                for (const wchar_t character : sid)
                {
                    add(static_cast<uint8_t>(character & 0xFF));
                    add(static_cast<uint8_t>((character >> 8) & 0xFF));
                }
                add(0);
                for (unsigned int i = 0; i < 4; ++i)
                {
                    add(static_cast<uint8_t>((sessionId >> (i * 8U)) & 0xFFU));
                }
                return value;
            }

            bool makeSecurityAttributes(
                const std::wstring& sid,
                SECURITY_ATTRIBUTES& attributes,
                LocalMemory& storage,
                std::string& error)
            {
                const std::wstring sddl = L"D:P(A;;GA;;;" + sid + L")";
                PSECURITY_DESCRIPTOR descriptor = nullptr;
                if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(
                        sddl.c_str(),
                        SDDL_REVISION_1,
                        &descriptor,
                        nullptr))
                {
                    error = windowsError(
                        "ConvertStringSecurityDescriptorToSecurityDescriptorW",
                        ::GetLastError());
                    return false;
                }
                storage.value = descriptor;
                attributes.nLength = sizeof(attributes);
                attributes.lpSecurityDescriptor = descriptor;
                attributes.bInheritHandle = FALSE;
                return true;
            }

#if defined(DJV_WINDOWS_OPEN_BROKER_TESTING)
            std::atomic<WindowsOpenBrokerRevertToSelfTestHook>
                revertToSelfTestHook(nullptr);
            std::atomic<WindowsOpenBrokerFailStopTestHook>
                failStopTestHook(nullptr);
#endif // DJV_WINDOWS_OPEN_BROKER_TESTING

            bool brokerRevertToSelf()
            {
#if defined(DJV_WINDOWS_OPEN_BROKER_TESTING)
                if (const auto hook = revertToSelfTestHook.load())
                {
                    return hook();
                }
#endif // DJV_WINDOWS_OPEN_BROKER_TESTING
                return 0 != ::RevertToSelf();
            }

            [[noreturn]] void securityFailStop(const std::string& message)
            {
#if defined(DJV_WINDOWS_OPEN_BROKER_TESTING)
                if (const auto hook = failStopTestHook.load())
                {
                    // Test hooks must throw. If one returns, the production
                    // fail-stop path remains authoritative.
                    hook(message);
                }
#else // DJV_WINDOWS_OPEN_BROKER_TESTING
                (void)message;
#endif // DJV_WINDOWS_OPEN_BROKER_TESTING
                ::TerminateProcess(::GetCurrentProcess(), ERROR_ACCESS_DENIED);
                std::abort();
            }

            bool isForbiddenDevicePath(const std::filesystem::path& path)
            {
                std::wstring value = path.native();
                std::transform(
                    value.begin(),
                    value.end(),
                    value.begin(),
                    [](wchar_t character)
                    {
                        return static_cast<wchar_t>(::towlower(character));
                    });
                return
                    0 == value.rfind(L"\\\\.\\", 0) ||
                    0 == value.rfind(L"\\\\?\\globalroot\\", 0) ||
                    0 == value.rfind(L"\\\\?\\pipe\\", 0) ||
                    0 == value.rfind(L"\\??\\", 0);
            }

            struct ProtocolHeader
            {
                uint16_t version = 0;
                uint16_t type = 0;
                uint32_t payloadBytes = 0;
                uint64_t requestId = 0;
            };

            std::vector<uint8_t> encodeHeader(const ProtocolHeader& header)
            {
                std::vector<uint8_t> out;
                out.reserve(protocolHeaderBytes);
                out.insert(
                    out.end(),
                    std::begin(protocolMagic),
                    std::end(protocolMagic));
                appendU16(out, header.version);
                appendU16(out, header.type);
                appendU32(out, header.payloadBytes);
                appendU64(out, header.requestId);
                return out;
            }

            bool decodeHeader(
                const std::vector<uint8_t>& data,
                ProtocolHeader& header,
                std::string& error)
            {
                if (data.size() != protocolHeaderBytes)
                {
                    error = "Protocol header has the wrong size";
                    return false;
                }
                if (0 != std::memcmp(data.data(), protocolMagic, sizeof(protocolMagic)))
                {
                    error = "Protocol magic does not match";
                    return false;
                }
                size_t offset = sizeof(protocolMagic);
                if (!readU16(data, offset, header.version) ||
                    !readU16(data, offset, header.type) ||
                    !readU32(data, offset, header.payloadBytes) ||
                    !readU64(data, offset, header.requestId) ||
                    offset != data.size())
                {
                    error = "Protocol header is truncated";
                    return false;
                }
                return true;
            }

            bool encodeRequestPayload(
                const std::vector<std::string>& paths,
                const std::string& cwd,
                std::vector<uint8_t>& out,
                std::string& error)
            {
                if (paths.empty() || paths.size() > WindowsOpenBroker::maxPathCount)
                {
                    error = "An open request must contain 1-128 paths";
                    return false;
                }
                if (cwd.size() > WindowsOpenBroker::maxPathBytes)
                {
                    error = "The working directory exceeds the protocol limit";
                    return false;
                }
                out.clear();
                out.reserve(8 + cwd.size() + paths.size() * 8);
                appendU32(out, static_cast<uint32_t>(cwd.size()));
                appendU32(out, static_cast<uint32_t>(paths.size()));
                out.insert(out.end(), cwd.begin(), cwd.end());
                for (const auto& path : paths)
                {
                    if (path.empty() || path.size() > WindowsOpenBroker::maxPathBytes)
                    {
                        error = "A path is empty or exceeds the protocol limit";
                        return false;
                    }
                    if (out.size() > WindowsOpenBroker::maxPayloadBytes - 4 - path.size())
                    {
                        error = "The open request exceeds the protocol payload limit";
                        return false;
                    }
                    appendU32(out, static_cast<uint32_t>(path.size()));
                    out.insert(out.end(), path.begin(), path.end());
                }
                if (out.size() > WindowsOpenBroker::maxPayloadBytes)
                {
                    error = "The open request exceeds the protocol payload limit";
                    return false;
                }
                return true;
            }

            bool decodeRequestPayload(
                const std::vector<uint8_t>& payload,
                std::vector<std::string>& paths,
                std::string& cwd,
                std::string& error)
            {
                size_t offset = 0;
                uint32_t cwdBytes = 0;
                uint32_t pathCount = 0;
                if (!readU32(payload, offset, cwdBytes) ||
                    !readU32(payload, offset, pathCount))
                {
                    error = "The request payload is truncated";
                    return false;
                }
                if (cwdBytes > WindowsOpenBroker::maxPathBytes ||
                    pathCount == 0 ||
                    pathCount > WindowsOpenBroker::maxPathCount)
                {
                    error = "The request declares an invalid path count or working directory";
                    return false;
                }
                if (offset > payload.size() || payload.size() - offset < cwdBytes)
                {
                    error = "The working directory is truncated";
                    return false;
                }
                cwd.assign(
                    reinterpret_cast<const char*>(payload.data() + offset),
                    cwdBytes);
                offset += cwdBytes;

                std::vector<std::string> decoded;
                decoded.reserve(pathCount);
                for (uint32_t i = 0; i < pathCount; ++i)
                {
                    uint32_t pathBytes = 0;
                    if (!readU32(payload, offset, pathBytes) ||
                        pathBytes == 0 ||
                        pathBytes > WindowsOpenBroker::maxPathBytes ||
                        offset > payload.size() ||
                        payload.size() - offset < pathBytes)
                    {
                        error = "A request path is empty, oversized, or truncated";
                        return false;
                    }
                    decoded.emplace_back(
                        reinterpret_cast<const char*>(payload.data() + offset),
                        pathBytes);
                    offset += pathBytes;
                }
                if (offset != payload.size())
                {
                    error = "The request payload contains trailing bytes";
                    return false;
                }
                paths = std::move(decoded);
                return true;
            }

            std::vector<uint8_t> encodeResponsePayload(
                ResponseCode code,
                std::string message)
            {
                std::wstring validation;
                std::string validationError;
                if (!utf8ToWide(message, validation, validationError))
                {
                    message = "The server produced a non-UTF-8 response";
                }
                if (message.size() > WindowsOpenBroker::maxResponseMessageBytes)
                {
                    message = "The server response exceeded the message limit";
                }
                std::vector<uint8_t> out;
                out.reserve(8 + message.size());
                appendU32(out, static_cast<uint32_t>(code));
                appendU32(out, static_cast<uint32_t>(message.size()));
                out.insert(out.end(), message.begin(), message.end());
                return out;
            }

            bool decodeResponsePayload(
                const std::vector<uint8_t>& payload,
                ResponseCode& code,
                std::string& message,
                std::string& error)
            {
                size_t offset = 0;
                uint32_t rawCode = 0;
                uint32_t messageBytes = 0;
                if (!readU32(payload, offset, rawCode) ||
                    !readU32(payload, offset, messageBytes) ||
                    messageBytes > WindowsOpenBroker::maxResponseMessageBytes ||
                    offset > payload.size() ||
                    payload.size() - offset != messageBytes)
                {
                    error = "The response payload is malformed";
                    return false;
                }
                if (rawCode > static_cast<uint32_t>(ResponseCode::PayloadTooLarge))
                {
                    error = "The response contains an unknown status code";
                    return false;
                }
                message.assign(
                    reinterpret_cast<const char*>(payload.data() + offset),
                    messageBytes);
                std::wstring validation;
                if (!utf8ToWide(message, validation, error))
                {
                    error = "The response message is not valid UTF-8";
                    return false;
                }
                code = static_cast<ResponseCode>(rawCode);
                return true;
            }

            enum class IOStatus
            {
                Complete,
                Stopped,
                Timeout,
                Disconnected,
                Error
            };

            DWORD remainingMilliseconds(
                const std::chrono::steady_clock::time_point& deadline)
            {
                const auto now = std::chrono::steady_clock::now();
                if (now >= deadline)
                {
                    return 0;
                }
                const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - now);
                const auto value = (std::max)(int64_t(1), remaining.count());
                return static_cast<DWORD>((std::min)(
                    value,
                    static_cast<int64_t>((std::numeric_limits<DWORD>::max)() - 1)));
            }

            IOStatus transferOnce(
                HANDLE pipe,
                bool write,
                uint8_t* data,
                DWORD requested,
                DWORD& transferred,
                HANDLE stopEvent,
                const std::chrono::steady_clock::time_point& deadline,
                std::string& error)
            {
                Handle event(::CreateEventW(nullptr, TRUE, FALSE, nullptr));
                if (!event)
                {
                    error = windowsError("CreateEventW", ::GetLastError());
                    return IOStatus::Error;
                }
                OVERLAPPED overlapped = {};
                overlapped.hEvent = event.get();
                transferred = 0;
                const BOOL immediate = write ?
                    ::WriteFile(pipe, data, requested, &transferred, &overlapped) :
                    ::ReadFile(pipe, data, requested, &transferred, &overlapped);
                if (immediate)
                {
                    return transferred ? IOStatus::Complete : IOStatus::Disconnected;
                }

                const DWORD operationError = ::GetLastError();
                if (ERROR_BROKEN_PIPE == operationError ||
                    ERROR_PIPE_NOT_CONNECTED == operationError ||
                    ERROR_NO_DATA == operationError)
                {
                    return IOStatus::Disconnected;
                }
                if (ERROR_IO_PENDING != operationError)
                {
                    error = windowsError(write ? "WriteFile" : "ReadFile", operationError);
                    return IOStatus::Error;
                }

                const DWORD timeout = remainingMilliseconds(deadline);
                if (!timeout)
                {
                    ::CancelIoEx(pipe, &overlapped);
                    if (::GetOverlappedResult(
                            pipe, &overlapped, &transferred, TRUE) &&
                        transferred)
                    {
                        // Completion won the cancellation race. Reporting a
                        // timeout here could make a caller retry a delivered
                        // request.
                        return IOStatus::Complete;
                    }
                    return IOStatus::Timeout;
                }
                HANDLE waitHandles[2] = { event.get(), nullptr };
                DWORD handleCount = 1;
                if (stopEvent)
                {
                    waitHandles[0] = stopEvent;
                    waitHandles[1] = event.get();
                    handleCount = 2;
                }
                const DWORD waitResult = ::WaitForMultipleObjects(
                    handleCount,
                    waitHandles,
                    FALSE,
                    timeout);
                if ((stopEvent && WAIT_OBJECT_0 == waitResult) ||
                    WAIT_TIMEOUT == waitResult)
                {
                    const bool stopped = stopEvent && WAIT_OBJECT_0 == waitResult;
                    ::CancelIoEx(pipe, &overlapped);
                    const BOOL completed = ::GetOverlappedResult(
                        pipe, &overlapped, &transferred, TRUE);
                    if (!stopped && completed && transferred)
                    {
                        return IOStatus::Complete;
                    }
                    return stopped ? IOStatus::Stopped : IOStatus::Timeout;
                }
                const DWORD eventIndex = stopEvent ? WAIT_OBJECT_0 + 1 : WAIT_OBJECT_0;
                if (eventIndex != waitResult)
                {
                    ::CancelIoEx(pipe, &overlapped);
                    ::GetOverlappedResult(pipe, &overlapped, &transferred, TRUE);
                    error = windowsError("WaitForMultipleObjects", ::GetLastError());
                    return IOStatus::Error;
                }
                if (!::GetOverlappedResult(pipe, &overlapped, &transferred, FALSE))
                {
                    const DWORD resultError = ::GetLastError();
                    if (ERROR_OPERATION_ABORTED == resultError && stopEvent &&
                        WAIT_OBJECT_0 == ::WaitForSingleObject(stopEvent, 0))
                    {
                        return IOStatus::Stopped;
                    }
                    if (ERROR_BROKEN_PIPE == resultError ||
                        ERROR_PIPE_NOT_CONNECTED == resultError ||
                        ERROR_NO_DATA == resultError)
                    {
                        return IOStatus::Disconnected;
                    }
                    error = windowsError("GetOverlappedResult", resultError);
                    return IOStatus::Error;
                }
                return transferred ? IOStatus::Complete : IOStatus::Disconnected;
            }

            IOStatus transferExact(
                HANDLE pipe,
                bool write,
                uint8_t* data,
                size_t size,
                HANDLE stopEvent,
                std::chrono::milliseconds timeout,
                std::string& error)
            {
                const auto deadline = std::chrono::steady_clock::now() + timeout;
                size_t offset = 0;
                while (offset < size)
                {
                    if (std::chrono::steady_clock::now() >= deadline)
                    {
                        return IOStatus::Timeout;
                    }
                    const DWORD requested = static_cast<DWORD>((std::min)(
                        size - offset,
                        static_cast<size_t>(64U * 1024U)));
                    DWORD transferred = 0;
                    const IOStatus status = transferOnce(
                        pipe,
                        write,
                        data + offset,
                        requested,
                        transferred,
                        stopEvent,
                        deadline,
                        error);
                    if (IOStatus::Complete != status)
                    {
                        return status;
                    }
                    offset += transferred;
                }
                return IOStatus::Complete;
            }

            IOStatus writeBytes(
                HANDLE pipe,
                const std::vector<uint8_t>& data,
                HANDLE stopEvent,
                std::chrono::milliseconds timeout,
                std::string& error)
            {
                if (data.empty())
                {
                    return IOStatus::Complete;
                }
                return transferExact(
                    pipe,
                    true,
                    const_cast<uint8_t*>(data.data()),
                    data.size(),
                    stopEvent,
                    timeout,
                    error);
            }

            IOStatus readBytes(
                HANDLE pipe,
                std::vector<uint8_t>& data,
                HANDLE stopEvent,
                std::chrono::milliseconds timeout,
                std::string& error)
            {
                if (data.empty())
                {
                    return IOStatus::Complete;
                }
                return transferExact(
                    pipe,
                    false,
                    data.data(),
                    data.size(),
                    stopEvent,
                    timeout,
                    error);
            }

            bool writeResponse(
                HANDLE pipe,
                uint64_t requestId,
                ResponseCode code,
                const std::string& message,
                HANDLE stopEvent,
                std::chrono::milliseconds timeout,
                std::string& error)
            {
                const auto payload = encodeResponsePayload(code, message);
                const auto header = encodeHeader({
                    WindowsOpenBroker::protocolVersion,
                    responseMessageType,
                    static_cast<uint32_t>(payload.size()),
                    requestId });
                if (IOStatus::Complete != writeBytes(
                        pipe, header, stopEvent, timeout, error))
                {
                    return false;
                }
                if (IOStatus::Complete != writeBytes(
                        pipe, payload, stopEvent, timeout, error))
                {
                    return false;
                }

                // DisconnectNamedPipe discards unread server output. The
                // bounded receipt proves that the client consumed the complete
                // response before this pipe instance is disconnected/reused.
                std::vector<uint8_t> receipt(1);
                if (IOStatus::Complete != readBytes(
                        pipe, receipt, stopEvent, timeout, error))
                {
                    return false;
                }
                if (responseReceipt != receipt.front())
                {
                    error = "The client response receipt is invalid";
                    return false;
                }
                return true;
            }

            bool getCurrentSessionId(uint32_t& sessionId, std::string& error)
            {
                DWORD value = 0;
                if (!::ProcessIdToSessionId(::GetCurrentProcessId(), &value))
                {
                    error = windowsError("ProcessIdToSessionId", ::GetLastError());
                    return false;
                }
                sessionId = value;
                return true;
            }

            bool verifyClientIdentity(
                HANDLE pipe,
                uint32_t expectedSessionId,
                std::string& error)
            {
                using GetClientSessionId = BOOL(WINAPI*)(HANDLE, PULONG);
                const HMODULE kernel = ::GetModuleHandleW(L"kernel32.dll");
                if (!kernel)
                {
                    error = windowsError("GetModuleHandleW(kernel32)", ::GetLastError());
                    return false;
                }
                const auto getClientSessionId = reinterpret_cast<GetClientSessionId>(
                    ::GetProcAddress(
                        kernel,
                        "GetNamedPipeClientSessionId"));
                if (!getClientSessionId)
                {
                    error = "GetNamedPipeClientSessionId is unavailable";
                    return false;
                }
                ULONG clientSessionId = 0;
                if (!getClientSessionId(pipe, &clientSessionId) ||
                    clientSessionId != expectedSessionId)
                {
                    error = "The named-pipe client belongs to a different logon session";
                    return false;
                }

                if (!::ImpersonateNamedPipeClient(pipe))
                {
                    error = windowsError("ImpersonateNamedPipeClient", ::GetLastError());
                    return false;
                }
                HANDLE rawClientToken = nullptr;
                const BOOL opened = ::OpenThreadToken(
                    ::GetCurrentThread(),
                    TOKEN_QUERY,
                    TRUE,
                    &rawClientToken);
                const DWORD tokenError = opened ? ERROR_SUCCESS : ::GetLastError();
                const bool reverted = brokerRevertToSelf();
                const DWORD revertError = reverted ? ERROR_SUCCESS : ::GetLastError();
                Handle clientToken(rawClientToken);
                if (!reverted)
                {
                    error = windowsError("RevertToSelf", revertError);
                    // Microsoft requires process shutdown here: continuing may
                    // execute later requests under the client's token.
                    securityFailStop(error);
                }
                if (!opened)
                {
                    error = windowsError("OpenThreadToken", tokenError);
                    return false;
                }
                std::vector<uint8_t> clientSid;
                std::vector<uint8_t> currentSid;
                std::vector<uint8_t> clientLogonSid;
                std::vector<uint8_t> currentLogonSid;
                if (!getTokenUserSid(clientToken.get(), clientSid, error) ||
                    !getProcessUserSid(::GetCurrentProcess(), currentSid, error) ||
                    !getTokenLogonSid(clientToken.get(), clientLogonSid, error) ||
                    !getProcessLogonSid(
                        ::GetCurrentProcess(), currentLogonSid, error))
                {
                    return false;
                }
                if (!::EqualSid(clientSid.data(), currentSid.data()))
                {
                    error = "The named-pipe client belongs to a different user";
                    return false;
                }
                if (!::EqualSid(clientLogonSid.data(), currentLogonSid.data()))
                {
                    error = "The named-pipe client belongs to a different logon";
                    return false;
                }
                return true;
            }

            bool verifyServerIdentity(
                HANDLE pipe,
                uint32_t expectedSessionId,
                std::string& error)
            {
                using GetServerProcessId = BOOL(WINAPI*)(HANDLE, PULONG);
                using GetServerSessionId = BOOL(WINAPI*)(HANDLE, PULONG);
                const HMODULE kernel = ::GetModuleHandleW(L"kernel32.dll");
                if (!kernel)
                {
                    error = windowsError("GetModuleHandleW(kernel32)", ::GetLastError());
                    return false;
                }
                const auto getServerProcessId = reinterpret_cast<GetServerProcessId>(
                    ::GetProcAddress(kernel, "GetNamedPipeServerProcessId"));
                const auto getServerSessionId = reinterpret_cast<GetServerSessionId>(
                    ::GetProcAddress(kernel, "GetNamedPipeServerSessionId"));
                if (!getServerProcessId || !getServerSessionId)
                {
                    error = "Named-pipe server identity APIs are unavailable";
                    return false;
                }
                ULONG serverPid = 0;
                ULONG serverSessionId = 0;
                if (!getServerProcessId(pipe, &serverPid) || !serverPid ||
                    !getServerSessionId(pipe, &serverSessionId) ||
                    serverSessionId != expectedSessionId)
                {
                    error = "The named-pipe server has an unexpected process or session identity";
                    return false;
                }
                Handle process(::OpenProcess(
                    PROCESS_QUERY_LIMITED_INFORMATION,
                    FALSE,
                    serverPid));
                if (!process)
                {
                    error = windowsError("OpenProcess(server)", ::GetLastError());
                    return false;
                }
                std::vector<uint8_t> serverSid;
                std::vector<uint8_t> currentSid;
                std::vector<uint8_t> serverLogonSid;
                std::vector<uint8_t> currentLogonSid;
                if (!getProcessUserSid(process.get(), serverSid, error) ||
                    !getProcessUserSid(::GetCurrentProcess(), currentSid, error) ||
                    !getProcessLogonSid(process.get(), serverLogonSid, error) ||
                    !getProcessLogonSid(
                        ::GetCurrentProcess(), currentLogonSid, error))
                {
                    return false;
                }
                if (!::EqualSid(serverSid.data(), currentSid.data()))
                {
                    error = "The named-pipe server belongs to a different user";
                    return false;
                }
                if (!::EqualSid(serverLogonSid.data(), currentLogonSid.data()))
                {
                    error = "The named-pipe server belongs to a different logon";
                    return false;
                }
                return true;
            }

            enum class ConnectStatus
            {
                Connected,
                Stopped,
                Error
            };

            ConnectStatus connectServerPipe(
                HANDLE pipe,
                HANDLE stopEvent,
                std::string& error)
            {
                Handle event(::CreateEventW(nullptr, TRUE, FALSE, nullptr));
                if (!event)
                {
                    error = windowsError("CreateEventW(connect)", ::GetLastError());
                    return ConnectStatus::Error;
                }
                OVERLAPPED overlapped = {};
                overlapped.hEvent = event.get();
                if (::ConnectNamedPipe(pipe, &overlapped))
                {
                    return ConnectStatus::Connected;
                }
                const DWORD connectError = ::GetLastError();
                if (ERROR_PIPE_CONNECTED == connectError)
                {
                    return ConnectStatus::Connected;
                }
                if (ERROR_IO_PENDING != connectError)
                {
                    error = windowsError("ConnectNamedPipe", connectError);
                    return ConnectStatus::Error;
                }
                HANDLE handles[2] = { stopEvent, event.get() };
                const DWORD waitResult = ::WaitForMultipleObjects(2, handles, FALSE, INFINITE);
                if (WAIT_OBJECT_0 == waitResult)
                {
                    ::CancelIoEx(pipe, &overlapped);
                    DWORD ignored = 0;
                    ::GetOverlappedResult(pipe, &overlapped, &ignored, TRUE);
                    return ConnectStatus::Stopped;
                }
                if (WAIT_OBJECT_0 + 1 != waitResult)
                {
                    ::CancelIoEx(pipe, &overlapped);
                    DWORD ignored = 0;
                    ::GetOverlappedResult(pipe, &overlapped, &ignored, TRUE);
                    error = windowsError("WaitForMultipleObjects(connect)", ::GetLastError());
                    return ConnectStatus::Error;
                }
                DWORD ignored = 0;
                if (!::GetOverlappedResult(pipe, &overlapped, &ignored, FALSE))
                {
                    const DWORD resultError = ::GetLastError();
                    if (ERROR_OPERATION_ABORTED == resultError &&
                        WAIT_OBJECT_0 == ::WaitForSingleObject(stopEvent, 0))
                    {
                        return ConnectStatus::Stopped;
                    }
                    error = windowsError("GetOverlappedResult(connect)", resultError);
                    return ConnectStatus::Error;
                }
                return ConnectStatus::Connected;
            }
#endif // _WIN32
        }

        class WindowsOpenBroker::Private
        {
        public:
            explicit Private(const WindowsOpenBrokerOptions& value) :
                options(value)
            {}

            void setLastError(const std::string& value)
            {
                std::lock_guard<std::mutex> lock(errorMutex);
                lastError = value;
            }

            std::string getLastError() const
            {
                std::lock_guard<std::mutex> lock(errorMutex);
                return lastError;
            }

#if defined(_WIN32)
            void handleClient(HANDLE pipe)
            {
                uint64_t requestId = 0;
                std::string error;
                if (!verifyClientIdentity(pipe, identity.sessionId, error))
                {
                    setLastError(error);
                    writeResponse(
                        pipe,
                        requestId,
                        ResponseCode::Unauthorized,
                        "The client identity was rejected",
                        stopEvent.get(),
                        options.ioTimeout,
                        error);
                    return;
                }

                std::vector<uint8_t> rawHeader(protocolHeaderBytes);
                const IOStatus headerStatus = readBytes(
                    pipe,
                    rawHeader,
                    stopEvent.get(),
                    options.ioTimeout,
                    error);
                if (IOStatus::Complete != headerStatus)
                {
                    if (IOStatus::Stopped != headerStatus &&
                        IOStatus::Disconnected != headerStatus)
                    {
                        setLastError(error.empty() ? "Request header read timed out" : error);
                    }
                    return;
                }

                ProtocolHeader header;
                if (!decodeHeader(rawHeader, header, error))
                {
                    setLastError(error);
                    writeResponse(
                        pipe,
                        requestId,
                        ResponseCode::ProtocolError,
                        error,
                        stopEvent.get(),
                        options.ioTimeout,
                        error);
                    return;
                }
                requestId = header.requestId;
                if (WindowsOpenBroker::protocolVersion != header.version ||
                    requestMessageType != header.type)
                {
                    error = "Unsupported protocol version or message type";
                    setLastError(error);
                    writeResponse(
                        pipe,
                        requestId,
                        ResponseCode::ProtocolError,
                        error,
                        stopEvent.get(),
                        options.ioTimeout,
                        error);
                    return;
                }
                if (header.payloadBytes > WindowsOpenBroker::maxPayloadBytes)
                {
                    error = "Request payload exceeds the protocol limit";
                    setLastError(error);
                    writeResponse(
                        pipe,
                        requestId,
                        ResponseCode::PayloadTooLarge,
                        error,
                        stopEvent.get(),
                        options.ioTimeout,
                        error);
                    return;
                }

                std::vector<uint8_t> payload(header.payloadBytes);
                const IOStatus payloadStatus = readBytes(
                    pipe,
                    payload,
                    stopEvent.get(),
                    options.ioTimeout,
                    error);
                if (IOStatus::Complete != payloadStatus)
                {
                    if (IOStatus::Stopped != payloadStatus &&
                        IOStatus::Disconnected != payloadStatus)
                    {
                        setLastError(error.empty() ? "Request payload read timed out" : error);
                    }
                    return;
                }

                std::vector<std::string> rawPaths;
                std::string cwd;
                if (!decodeRequestPayload(payload, rawPaths, cwd, error))
                {
                    setLastError(error);
                    writeResponse(
                        pipe,
                        requestId,
                        ResponseCode::InvalidRequest,
                        error,
                        stopEvent.get(),
                        options.ioTimeout,
                        error);
                    return;
                }
                std::vector<std::string> paths;
                if (!WindowsOpenBroker::normalizePaths(
                        rawPaths, cwd, paths, error))
                {
                    setLastError(error);
                    writeResponse(
                        pipe,
                        requestId,
                        ResponseCode::InvalidRequest,
                        error,
                        stopEvent.get(),
                        options.ioTimeout,
                        error);
                    return;
                }
                if (stopping.load())
                {
                    writeResponse(
                        pipe,
                        requestId,
                        ResponseCode::ServerStopping,
                        "The server is stopping",
                        stopEvent.get(),
                        options.ioTimeout,
                        error);
                    return;
                }

                std::string callbackMessage;
                bool accepted = false;
                try
                {
                    accepted = callback(paths, callbackMessage);
                }
                catch (const std::exception& exception)
                {
                    setLastError(std::string("Open callback exception: ") + exception.what());
                    writeResponse(
                        pipe,
                        requestId,
                        ResponseCode::InternalError,
                        "The server callback failed",
                        stopEvent.get(),
                        options.ioTimeout,
                        error);
                    return;
                }
                catch (...)
                {
                    setLastError("Open callback threw an unknown exception");
                    writeResponse(
                        pipe,
                        requestId,
                        ResponseCode::InternalError,
                        "The server callback failed",
                        stopEvent.get(),
                        options.ioTimeout,
                        error);
                    return;
                }
                const ResponseCode responseCode = accepted ?
                    ResponseCode::Accepted :
                    ResponseCode::CallbackRejected;
                if (!accepted && callbackMessage.empty())
                {
                    callbackMessage = "The open request was rejected";
                }
                if (!writeResponse(
                        pipe,
                        requestId,
                        responseCode,
                        callbackMessage,
                        stopEvent.get(),
                        options.ioTimeout,
                        error) &&
                    !stopping.load())
                {
                    setLastError(error.empty() ? "Writing the response failed" : error);
                }
            }

            void run()
            {
                SECURITY_ATTRIBUTES attributes = {};
                LocalMemory descriptor;
                std::string error;
                if (!makeSecurityAttributes(
                        identity.logonSid,
                        attributes,
                        descriptor,
                        error))
                {
                    setLastError(error);
                    serverStarted = false;
                    ::SetEvent(readyEvent.get());
                    return;
                }
                constexpr DWORD rejectRemoteClients = 0x00000008UL;
                Handle pipe(::CreateNamedPipeW(
                    identity.pipeName.c_str(),
                    PIPE_ACCESS_DUPLEX |
                        FILE_FLAG_OVERLAPPED |
                        FILE_FLAG_FIRST_PIPE_INSTANCE,
                    PIPE_TYPE_BYTE |
                        PIPE_READMODE_BYTE |
                        PIPE_WAIT |
                        rejectRemoteClients,
                    1,
                    64U * 1024U,
                    64U * 1024U,
                    0,
                    &attributes));
                if (!pipe || INVALID_HANDLE_VALUE == pipe.get())
                {
                    setLastError(windowsError("CreateNamedPipeW", ::GetLastError()));
                    serverStarted = false;
                    ::SetEvent(readyEvent.get());
                    return;
                }
                {
                    std::lock_guard<std::mutex> lock(pipeMutex);
                    activePipe = pipe.get();
                }
                serverStarted = true;
                running = true;
                ::SetEvent(readyEvent.get());

                while (!stopping.load())
                {
                    error.clear();
                    const ConnectStatus status = connectServerPipe(
                        pipe.get(), stopEvent.get(), error);
                    if (ConnectStatus::Stopped == status)
                    {
                        break;
                    }
                    if (ConnectStatus::Error == status)
                    {
                        if (!stopping.load())
                        {
                            setLastError(error);
                        }
                        break;
                    }
                    if (!stopping.load())
                    {
                        handleClient(pipe.get());
                    }
                    if (!::DisconnectNamedPipe(pipe.get()))
                    {
                        const DWORD disconnectError = ::GetLastError();
                        if (ERROR_PIPE_NOT_CONNECTED != disconnectError && !stopping.load())
                        {
                            setLastError(windowsError(
                                "DisconnectNamedPipe", disconnectError));
                        }
                    }
                }
                {
                    std::lock_guard<std::mutex> lock(pipeMutex);
                    activePipe = nullptr;
                }
                running = false;
            }

            Handle stopEvent;
            Handle readyEvent;
            Handle exitEvent;
            Handle primaryMutex;
            std::thread worker;
            std::mutex lifecycleMutex;
            std::condition_variable lifecycleCV;
            std::thread::id workerThreadId;
            bool workerDetached = false;
            bool joinInProgress = false;
            bool cleanupComplete = true;
            std::mutex pipeMutex;
            HANDLE activePipe = nullptr;
            std::atomic<bool> running = false;
            std::atomic<bool> stopping = false;
            std::atomic<bool> serverStarted = false;
#else // _WIN32
            bool running = false;
#endif // _WIN32

            WindowsOpenBrokerOptions options;
            WindowsOpenBrokerIdentity identity;
            Callback callback;
            mutable std::mutex errorMutex;
            std::string lastError;
        };

        WindowsOpenBroker::WindowsOpenBroker(
            const WindowsOpenBrokerOptions& options) :
            _p(std::make_shared<Private>(options))
        {
            std::string error;
            resolveIdentity(options, _p->identity, error);
            if (!error.empty())
            {
                _p->setLastError(error);
            }
        }

        WindowsOpenBroker::~WindowsOpenBroker()
        {
            stop();
        }

        WindowsOpenBrokerStartResult WindowsOpenBroker::start(Callback callback)
        {
            WindowsOpenBrokerStartResult out;
            std::string error;
            if (!validateOptions(_p->options, error) || !callback)
            {
                out.status = WindowsOpenBrokerStartStatus::InvalidOptions;
                out.error = error.empty() ? "The broker callback is empty" : error;
                return out;
            }
#if defined(_WIN32)
            std::unique_lock<std::mutex> lifecycleLock(_p->lifecycleMutex);
            if (_p->workerDetached)
            {
                if (!_p->exitEvent ||
                    WAIT_OBJECT_0 != ::WaitForSingleObject(_p->exitEvent.get(), 0))
                {
                    out.status = WindowsOpenBrokerStartStatus::AlreadyRunning;
                    out.error = "The detached broker worker is still stopping";
                    return out;
                }
                lifecycleLock.unlock();
                stop();
                lifecycleLock.lock();
            }
            if (_p->worker.joinable() || _p->running.load() ||
                _p->joinInProgress || !_p->cleanupComplete)
            {
                out.status = WindowsOpenBrokerStartStatus::AlreadyRunning;
                out.error = "This broker object is already running";
                return out;
            }
            if (!resolveIdentity(_p->options, _p->identity, error))
            {
                out.status = WindowsOpenBrokerStartStatus::SecurityError;
                out.error = error;
                _p->setLastError(error);
                return out;
            }
            SECURITY_ATTRIBUTES attributes = {};
            LocalMemory descriptor;
            if (!makeSecurityAttributes(
                    _p->identity.logonSid,
                    attributes,
                    descriptor,
                    error))
            {
                out.status = WindowsOpenBrokerStartStatus::SecurityError;
                out.error = error;
                _p->setLastError(error);
                return out;
            }
            _p->stopEvent.reset(::CreateEventW(nullptr, TRUE, FALSE, nullptr));
            _p->readyEvent.reset(::CreateEventW(nullptr, TRUE, FALSE, nullptr));
            _p->exitEvent.reset(::CreateEventW(nullptr, TRUE, FALSE, nullptr));
            if (!_p->stopEvent || !_p->readyEvent || !_p->exitEvent)
            {
                out.status = WindowsOpenBrokerStartStatus::IOError;
                out.error = windowsError("CreateEventW(start)", ::GetLastError());
                _p->setLastError(out.error);
                _p->stopEvent.reset();
                _p->readyEvent.reset();
                _p->exitEvent.reset();
                return out;
            }
            const HANDLE rawMutex = ::CreateMutexW(
                &attributes,
                FALSE,
                _p->identity.mutexName.c_str());
            if (!rawMutex)
            {
                const DWORD mutexError = ::GetLastError();
                out.status = ERROR_ACCESS_DENIED == mutexError ?
                    WindowsOpenBrokerStartStatus::SecurityError :
                    WindowsOpenBrokerStartStatus::IOError;
                out.error = windowsError("CreateMutexW", mutexError);
                _p->setLastError(out.error);
                _p->stopEvent.reset();
                _p->readyEvent.reset();
                _p->exitEvent.reset();
                return out;
            }
            _p->primaryMutex.reset(rawMutex);
            if (ERROR_ALREADY_EXISTS == ::GetLastError())
            {
                _p->primaryMutex.reset();
                _p->stopEvent.reset();
                _p->readyEvent.reset();
                _p->exitEvent.reset();
                out.status = WindowsOpenBrokerStartStatus::AlreadyRunning;
                out.error = "Another broker already owns the per-logon primary slot";
                return out;
            }
            _p->callback = std::move(callback);
            _p->stopping = false;
            _p->serverStarted = false;
            _p->workerDetached = false;
            _p->joinInProgress = false;
            _p->cleanupComplete = false;
            try
            {
                const auto state = _p;
                _p->worker = std::thread(
                    [state]
                    {
                        try
                        {
                            state->run();
                        }
                        catch (const std::exception& exception)
                        {
                            state->setLastError(
                                std::string("Broker worker exception: ") +
                                exception.what());
                        }
                        catch (...)
                        {
                            state->setLastError(
                                "Broker worker threw an unknown exception");
                        }
                        {
                            std::lock_guard<std::mutex> lock(state->pipeMutex);
                            state->activePipe = nullptr;
                        }
                        state->running = false;
                        state->serverStarted = false;
                        if (state->readyEvent)
                        {
                            ::SetEvent(state->readyEvent.get());
                        }
                        if (state->exitEvent)
                        {
                            ::SetEvent(state->exitEvent.get());
                        }
                    });
                _p->workerThreadId = _p->worker.get_id();
            }
            catch (const std::exception& exception)
            {
                out.status = WindowsOpenBrokerStartStatus::IOError;
                out.error = std::string("Starting the broker thread failed: ") +
                    exception.what();
                _p->setLastError(out.error);
                _p->primaryMutex.reset();
                _p->stopEvent.reset();
                _p->readyEvent.reset();
                _p->exitEvent.reset();
                _p->workerThreadId = std::thread::id();
                _p->cleanupComplete = true;
                _p->lifecycleCV.notify_all();
                return out;
            }
            const DWORD waitResult = ::WaitForSingleObject(
                _p->readyEvent.get(),
                static_cast<DWORD>(_p->options.startTimeout.count()));
            if (WAIT_OBJECT_0 != waitResult || !_p->serverStarted.load())
            {
                out.status = WindowsOpenBrokerStartStatus::IOError;
                out.error = WAIT_TIMEOUT == waitResult ?
                    "The broker server did not become ready before the timeout" :
                    _p->getLastError();
                if (out.error.empty())
                {
                    out.error = windowsError("WaitForSingleObject(start)", ::GetLastError());
                }
                lifecycleLock.unlock();
                stop();
                return out;
            }
            out.status = WindowsOpenBrokerStartStatus::Started;
            return out;
#else // _WIN32
            (void)callback;
            out.status = WindowsOpenBrokerStartStatus::Unsupported;
            out.error = "The single-instance open broker is only available on Windows";
            return out;
#endif // _WIN32
        }

        void WindowsOpenBroker::stop() noexcept
        {
#if defined(_WIN32)
            const auto state = _p;
            std::thread workerToJoin;
            HANDLE detachedExitEvent = nullptr;
            bool ownsCleanup = false;
            bool returnAfterCancel = false;

            state->stopping = true;
            {
                std::unique_lock<std::mutex> lock(state->lifecycleMutex);
                if (state->cleanupComplete)
                {
                    return;
                }
                if (state->stopEvent)
                {
                    ::SetEvent(state->stopEvent.get());
                }

                const bool calledByWorker =
                    state->workerThreadId == std::this_thread::get_id();
                if (state->joinInProgress)
                {
                    // An external caller already owns the join. Waiting here
                    // from the callback would deadlock that owner, which is
                    // joining this thread, so the callback simply returns.
                    if (calledByWorker)
                    {
                        returnAfterCancel = true;
                    }
                    else
                    {
                        state->lifecycleCV.wait(
                            lock,
                            [state] { return state->cleanupComplete; });
                        return;
                    }
                }
                else if (state->worker.joinable())
                {
                    if (calledByWorker)
                    {
                        // The thread owns a shared state reference. Detaching
                        // makes callback-triggered destruction safe: state and
                        // handles live until run() exits.
                        try
                        {
                            state->worker.detach();
                            state->workerDetached = true;
                        }
                        catch (...)
                        {
                            // detach() can only fail here if the guarded
                            // std::thread invariant was violated. Leaving a
                            // joinable worker to reach its destructor would
                            // terminate later at a less diagnosable point.
                            std::terminate();
                        }
                        returnAfterCancel = true;
                    }
                    else
                    {
                        workerToJoin = std::move(state->worker);
                        state->joinInProgress = true;
                        ownsCleanup = true;
                    }
                }
                else if (state->workerDetached)
                {
                    if (calledByWorker)
                    {
                        return;
                    }
                    detachedExitEvent = state->exitEvent.get();
                    state->joinInProgress = true;
                    ownsCleanup = true;
                }
                else
                {
                    // A partially initialized lifecycle has no worker. This
                    // caller still owns the one cleanup of its handles.
                    state->joinInProgress = true;
                    ownsCleanup = true;
                }
            }

            {
                std::lock_guard<std::mutex> lock(state->pipeMutex);
                if (state->activePipe)
                {
                    ::CancelIoEx(state->activePipe, nullptr);
                }
            }
            if (returnAfterCancel)
            {
                return;
            }

            if (workerToJoin.joinable())
            {
                try
                {
                    workerToJoin.join();
                }
                catch (const std::exception& exception)
                {
                    state->setLastError(
                        std::string("Joining the broker worker failed: ") +
                        exception.what());
                    try
                    {
                        if (workerToJoin.joinable())
                        {
                            workerToJoin.detach();
                        }
                    }
                    catch (...)
                    {}
                    if (state->exitEvent)
                    {
                        ::WaitForSingleObject(state->exitEvent.get(), INFINITE);
                    }
                }
                catch (...)
                {
                    state->setLastError(
                        "Joining the broker worker failed with an unknown exception");
                    try
                    {
                        if (workerToJoin.joinable())
                        {
                            workerToJoin.detach();
                        }
                    }
                    catch (...)
                    {}
                    if (state->exitEvent)
                    {
                        ::WaitForSingleObject(state->exitEvent.get(), INFINITE);
                    }
                }
            }
            else if (detachedExitEvent)
            {
                ::WaitForSingleObject(detachedExitEvent, INFINITE);
            }

            if (ownsCleanup)
            {
                Callback callbackToDestroy;
                std::lock_guard<std::mutex> lock(state->lifecycleMutex);
                state->workerDetached = false;
                state->joinInProgress = false;
                state->cleanupComplete = true;
                state->workerThreadId = std::thread::id();
                state->running = false;
                state->primaryMutex.reset();
                state->readyEvent.reset();
                state->stopEvent.reset();
                state->exitEvent.reset();
                callbackToDestroy = std::move(state->callback);
                state->lifecycleCV.notify_all();
            }
#else // _WIN32
            _p->running = false;
#endif // _WIN32
        }

        bool WindowsOpenBroker::isRunning() const noexcept
        {
#if defined(_WIN32)
            return _p->running.load();
#else // _WIN32
            return _p->running;
#endif // _WIN32
        }

        std::string WindowsOpenBroker::getLastError() const
        {
            return _p->getLastError();
        }

        const WindowsOpenBrokerIdentity& WindowsOpenBroker::getIdentity() const noexcept
        {
            return _p->identity;
        }

#if defined(DJV_WINDOWS_OPEN_BROKER_TESTING)
        void setWindowsOpenBrokerSecurityTestHooks(
            WindowsOpenBrokerRevertToSelfTestHook revertHook,
            WindowsOpenBrokerFailStopTestHook failStopHook) noexcept
        {
#if defined(_WIN32)
            revertToSelfTestHook.store(revertHook);
            failStopTestHook.store(failStopHook);
#else // _WIN32
            (void)revertHook;
            (void)failStopHook;
#endif // _WIN32
        }
#endif // DJV_WINDOWS_OPEN_BROKER_TESTING

        bool WindowsOpenBroker::resolveIdentity(
            const WindowsOpenBrokerOptions& options,
            WindowsOpenBrokerIdentity& out,
            std::string& error)
        {
            if (!validateOptions(options, error))
            {
                return false;
            }
#if defined(_WIN32)
            std::wstring applicationId;
            if (!utf8ToWide(options.applicationId, applicationId, error))
            {
                return false;
            }
            std::vector<uint8_t> sid;
            if (!getProcessUserSid(::GetCurrentProcess(), sid, error))
            {
                return false;
            }
            std::wstring sidString;
            if (!sidToString(sid.data(), sidString, error))
            {
                return false;
            }
            std::vector<uint8_t> logonSid;
            if (!getProcessLogonSid(::GetCurrentProcess(), logonSid, error))
            {
                return false;
            }
            std::wstring logonSidString;
            if (!sidToString(logonSid.data(), logonSidString, error))
            {
                return false;
            }
            uint32_t sessionId = 0;
            if (!getCurrentSessionId(sessionId, error))
            {
                return false;
            }
            const uint64_t hash = stableIdentityHash(
                options.applicationId, logonSidString, sessionId);
            std::wostringstream hashText;
            hashText << std::hex << std::setw(16) << std::setfill(L'0') << hash;
            out.pipeName = L"\\\\.\\pipe\\DJV.OpenBroker." + hashText.str();
            out.mutexName = L"Local\\DJV.OpenBroker." + hashText.str();
            out.userSid = std::move(sidString);
            out.logonSid = std::move(logonSidString);
            out.sessionId = sessionId;
            out.rejectsRemoteClients = true;
            return true;
#else // _WIN32
            (void)out;
            error = "The single-instance open broker is only available on Windows";
            return false;
#endif // _WIN32
        }

        bool WindowsOpenBroker::normalizePaths(
            const std::vector<std::string>& paths,
            const std::string& workingDirectoryUtf8,
            std::vector<std::string>& out,
            std::string& error)
        {
            out.clear();
            if (paths.empty() || paths.size() > maxPathCount)
            {
                error = "An open request must contain 1-128 paths";
                return false;
            }
            if (workingDirectoryUtf8.size() > maxPathBytes)
            {
                error = "The working directory exceeds the path limit";
                return false;
            }
#if defined(_WIN32)
            std::filesystem::path cwd;
            if (!workingDirectoryUtf8.empty())
            {
                std::wstring wideCwd;
                if (!utf8ToWide(workingDirectoryUtf8, wideCwd, error))
                {
                    return false;
                }
                cwd = std::filesystem::path(wideCwd).lexically_normal();
                if (!cwd.is_absolute() || isForbiddenDevicePath(cwd))
                {
                    error = "The working directory is not a safe absolute filesystem path";
                    return false;
                }
            }
            std::vector<std::string> normalized;
            normalized.reserve(paths.size());
            size_t totalBytes = 0;
            for (const auto& pathUtf8 : paths)
            {
                if (pathUtf8.empty() || pathUtf8.size() > maxPathBytes)
                {
                    error = "A path is empty or exceeds the path limit";
                    return false;
                }
                std::wstring widePath;
                if (!utf8ToWide(pathUtf8, widePath, error))
                {
                    return false;
                }
                std::filesystem::path path(widePath);
                if (isForbiddenDevicePath(path))
                {
                    error = "Device namespace paths are not accepted";
                    return false;
                }
                if (!path.is_absolute())
                {
                    if (path.has_root_name() || path.has_root_directory())
                    {
                        error = "Drive-relative and root-relative paths are ambiguous";
                        return false;
                    }
                    if (cwd.empty())
                    {
                        error = "A relative path requires an absolute working directory";
                        return false;
                    }
                    path = cwd / path;
                }
                path = path.lexically_normal();
                if (!path.is_absolute() || isForbiddenDevicePath(path))
                {
                    error = "A normalized path is not a safe absolute filesystem path";
                    return false;
                }
                std::string value;
                if (!wideToUtf8(path.native(), value, error) ||
                    value.empty() ||
                    value.size() > maxPathBytes)
                {
                    if (error.empty())
                    {
                        error = "A normalized path exceeds the path limit";
                    }
                    return false;
                }
                if (totalBytes > maxPayloadBytes - value.size())
                {
                    error = "The normalized paths exceed the payload limit";
                    return false;
                }
                totalBytes += value.size();
                normalized.push_back(std::move(value));
            }
            out = std::move(normalized);
            return true;
#else // _WIN32
            (void)workingDirectoryUtf8;
            error = "Windows path normalization is unavailable on this platform";
            return false;
#endif // _WIN32
        }

        WindowsOpenForwardResult WindowsOpenBroker::forward(
            const std::vector<std::string>& paths,
            const std::string& workingDirectoryUtf8,
            const WindowsOpenBrokerOptions& options)
        {
            WindowsOpenForwardResult out;
            std::string error;
            if (!validateOptions(options, error))
            {
                out.status = WindowsOpenForwardStatus::InvalidArgument;
                out.error = error;
                return out;
            }
#if defined(_WIN32)
            std::vector<std::string> normalized;
            if (!normalizePaths(
                    paths, workingDirectoryUtf8, normalized, error))
            {
                out.status = WindowsOpenForwardStatus::InvalidArgument;
                out.error = error;
                return out;
            }
            std::vector<uint8_t> payload;
            if (!encodeRequestPayload(
                    paths, workingDirectoryUtf8, payload, error))
            {
                out.status = WindowsOpenForwardStatus::InvalidArgument;
                out.error = error;
                return out;
            }
            WindowsOpenBrokerIdentity identity;
            if (!resolveIdentity(options, identity, error))
            {
                out.status = WindowsOpenForwardStatus::SecurityError;
                out.error = error;
                return out;
            }

            const auto connectDeadline =
                std::chrono::steady_clock::now() + options.connectTimeout;
            Handle pipe;
            bool primaryMarkerObserved = false;
            while (std::chrono::steady_clock::now() < connectDeadline)
            {
                const HANDLE rawPipe = ::CreateFileW(
                    identity.pipeName.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    nullptr,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED |
                        SECURITY_SQOS_PRESENT |
                        SECURITY_IDENTIFICATION,
                    nullptr);
                if (INVALID_HANDLE_VALUE != rawPipe)
                {
                    pipe.reset(rawPipe);
                    break;
                }
                const DWORD openError = ::GetLastError();
                if (ERROR_FILE_NOT_FOUND == openError)
                {
                    Handle primaryMarker(::OpenMutexW(
                        SYNCHRONIZE,
                        FALSE,
                        identity.mutexName.c_str()));
                    if (!primaryMarker)
                    {
                        const DWORD markerError = ::GetLastError();
                        if (ERROR_FILE_NOT_FOUND == markerError)
                        {
                            out.status = WindowsOpenForwardStatus::NoServer;
                            out.error = "No per-user broker server is running";
                            return out;
                        }
                        out.status = ERROR_ACCESS_DENIED == markerError ?
                            WindowsOpenForwardStatus::SecurityError :
                            WindowsOpenForwardStatus::IOError;
                        out.error = windowsError("OpenMutexW(primary marker)", markerError);
                        return out;
                    }
                    primaryMarkerObserved = true;
                    ::Sleep((std::min)(DWORD(10), remainingMilliseconds(connectDeadline)));
                    continue;
                }
                if (ERROR_ACCESS_DENIED == openError)
                {
                    out.status = WindowsOpenForwardStatus::SecurityError;
                    out.error = windowsError("CreateFileW(pipe)", openError);
                    return out;
                }
                if (ERROR_PIPE_BUSY != openError)
                {
                    out.status = WindowsOpenForwardStatus::IOError;
                    out.error = windowsError("CreateFileW(pipe)", openError);
                    return out;
                }
                primaryMarkerObserved = true;
                const DWORD remaining = remainingMilliseconds(connectDeadline);
                if (!remaining || !::WaitNamedPipeW(identity.pipeName.c_str(), remaining))
                {
                    const DWORD waitError = ::GetLastError();
                    if (ERROR_SEM_TIMEOUT != waitError &&
                        ERROR_FILE_NOT_FOUND != waitError)
                    {
                        out.status = WindowsOpenForwardStatus::IOError;
                        out.error = windowsError("WaitNamedPipeW", waitError);
                        return out;
                    }
                }
            }
            if (!pipe)
            {
                out.status = primaryMarkerObserved ?
                    WindowsOpenForwardStatus::Timeout :
                    WindowsOpenForwardStatus::NoServer;
                out.error = primaryMarkerObserved ?
                    "Timed out waiting for the broker server" :
                    "No per-user broker server is running";
                return out;
            }
            if (!verifyServerIdentity(pipe.get(), identity.sessionId, error))
            {
                out.status = WindowsOpenForwardStatus::SecurityError;
                out.error = error;
                return out;
            }

            static std::atomic<uint64_t> sequence(0);
            const uint64_t requestId =
                (::GetTickCount64() << 16U) ^
                static_cast<uint64_t>(::GetCurrentProcessId()) ^
                ++sequence;
            const auto header = encodeHeader({
                protocolVersion,
                requestMessageType,
                static_cast<uint32_t>(payload.size()),
                requestId });
            IOStatus status = writeBytes(
                pipe.get(), header, nullptr, options.ioTimeout, error);
            if (IOStatus::Complete == status)
            {
                status = writeBytes(
                    pipe.get(), payload, nullptr, options.ioTimeout, error);
            }
            if (IOStatus::Complete != status)
            {
                out.status = IOStatus::Timeout == status ?
                    WindowsOpenForwardStatus::Timeout :
                    WindowsOpenForwardStatus::IOError;
                out.error = error.empty() ? "Writing the open request failed" : error;
                return out;
            }

            std::vector<uint8_t> rawHeader(protocolHeaderBytes);
            status = readBytes(
                pipe.get(), rawHeader, nullptr, options.ioTimeout, error);
            if (IOStatus::Complete != status)
            {
                out.status = WindowsOpenForwardStatus::DeliveryUnknown;
                out.error = error.empty() ? "Reading the broker ACK failed" : error;
                return out;
            }
            ProtocolHeader responseHeader;
            if (!decodeHeader(rawHeader, responseHeader, error) ||
                protocolVersion != responseHeader.version ||
                responseMessageType != responseHeader.type ||
                requestId != responseHeader.requestId ||
                responseHeader.payloadBytes > maxResponseMessageBytes + 8)
            {
                out.status = WindowsOpenForwardStatus::DeliveryUnknown;
                out.error = error.empty() ? "The broker ACK header is invalid" : error;
                return out;
            }
            std::vector<uint8_t> responsePayload(responseHeader.payloadBytes);
            status = readBytes(
                pipe.get(), responsePayload, nullptr, options.ioTimeout, error);
            if (IOStatus::Complete != status)
            {
                out.status = WindowsOpenForwardStatus::DeliveryUnknown;
                out.error = error.empty() ? "Reading the broker ACK payload failed" : error;
                return out;
            }
            ResponseCode responseCode = ResponseCode::InternalError;
            std::string responseMessage;
            if (!decodeResponsePayload(
                    responsePayload, responseCode, responseMessage, error))
            {
                // Release the server's bounded receipt wait even for malformed
                // responses. The delivery state remains unknown.
                const std::vector<uint8_t> receipt(1, responseReceipt);
                std::string receiptError;
                writeBytes(
                    pipe.get(), receipt, nullptr, options.ioTimeout, receiptError);
                out.status = WindowsOpenForwardStatus::DeliveryUnknown;
                out.error = error;
                return out;
            }

            // Receipt is server-side flow control, not part of delivery proof:
            // the complete, correlated response above is authoritative.
            const std::vector<uint8_t> receipt(1, responseReceipt);
            std::string receiptError;
            writeBytes(pipe.get(), receipt, nullptr, options.ioTimeout, receiptError);
            if (ResponseCode::Accepted == responseCode)
            {
                out.status = WindowsOpenForwardStatus::Forwarded;
                return out;
            }
            out.error = responseMessage.empty() ?
                "The broker rejected the open request" :
                responseMessage;
            switch (responseCode)
            {
            case ResponseCode::CallbackRejected:
            case ResponseCode::ServerStopping:
                out.status = WindowsOpenForwardStatus::Rejected;
                break;
            case ResponseCode::Unauthorized:
                out.status = WindowsOpenForwardStatus::SecurityError;
                break;
            case ResponseCode::InvalidRequest:
            case ResponseCode::ProtocolError:
            case ResponseCode::PayloadTooLarge:
                out.status = WindowsOpenForwardStatus::ProtocolError;
                break;
            case ResponseCode::InternalError:
                out.status = WindowsOpenForwardStatus::DeliveryUnknown;
                break;
            case ResponseCode::Accepted:
                break;
            }
            return out;
#else // _WIN32
            (void)paths;
            (void)workingDirectoryUtf8;
            out.status = WindowsOpenForwardStatus::Unsupported;
            out.error = "The single-instance open broker is only available on Windows";
            return out;
#endif // _WIN32
        }
    }
}
