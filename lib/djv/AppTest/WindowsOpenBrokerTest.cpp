// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/AppTest/WindowsOpenBrokerTest.h>

#include <djv/App/WindowsOpenBroker.h>

#include <chrono>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#    if !defined(NOMINMAX)
#        define NOMINMAX
#    endif
#    include <windows.h>
#    include <aclapi.h>
#endif // _WIN32

namespace djv
{
    namespace app_tests
    {
        namespace
        {
            void require(bool value, const char* expression, int line)
            {
                if (!value)
                {
                    std::ostringstream stream;
                    stream << "Requirement failed at line " << line << ": " << expression;
                    throw std::runtime_error(stream.str());
                }
            }

#define DJV_BROKER_REQUIRE(EXPRESSION) \
            require(!!(EXPRESSION), #EXPRESSION, __LINE__)

#if defined(_WIN32)
            class TestHandle
            {
            public:
                TestHandle() = default;
                explicit TestHandle(HANDLE value) : _value(value) {}
                ~TestHandle()
                {
                    if (_value && INVALID_HANDLE_VALUE != _value)
                    {
                        ::CloseHandle(_value);
                    }
                }

                TestHandle(const TestHandle&) = delete;
                TestHandle& operator=(const TestHandle&) = delete;

                TestHandle(TestHandle&& other) noexcept :
                    _value(other.release())
                {}

                TestHandle& operator=(TestHandle&& other) noexcept
                {
                    if (this != &other)
                    {
                        if (_value && INVALID_HANDLE_VALUE != _value)
                        {
                            ::CloseHandle(_value);
                        }
                        _value = other.release();
                    }
                    return *this;
                }

                explicit operator bool() const noexcept
                {
                    return _value && INVALID_HANDLE_VALUE != _value;
                }

                HANDLE get() const noexcept
                {
                    return _value;
                }

                HANDLE release() noexcept
                {
                    const HANDLE out = _value;
                    _value = nullptr;
                    return out;
                }

            private:
                HANDLE _value = nullptr;
            };

            std::string wideToUtf8(const std::wstring& value)
            {
                if (value.empty())
                {
                    return std::string();
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
                DJV_BROKER_REQUIRE(size > 0);
                std::string out(static_cast<size_t>(size), '\0');
                DJV_BROKER_REQUIRE(size == ::WideCharToMultiByte(
                    CP_UTF8,
                    WC_ERR_INVALID_CHARS,
                    value.data(),
                    static_cast<int>(value.size()),
                    out.data(),
                    size,
                    nullptr,
                    nullptr));
                return out;
            }

            std::wstring getTempPath()
            {
                const DWORD size = ::GetTempPathW(0, nullptr);
                DJV_BROKER_REQUIRE(size > 0);
                std::wstring out(size, L'\0');
                const DWORD written = ::GetTempPathW(size, out.data());
                DJV_BROKER_REQUIRE(written > 0 && written < size);
                out.resize(written);
                return out;
            }

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

            uint16_t readU16(const std::vector<uint8_t>& data, size_t& offset)
            {
                DJV_BROKER_REQUIRE(offset <= data.size() && data.size() - offset >= 2);
                const uint16_t out =
                    static_cast<uint16_t>(data[offset]) |
                    static_cast<uint16_t>(data[offset + 1]) << 8U;
                offset += 2;
                return out;
            }

            uint32_t readU32(const std::vector<uint8_t>& data, size_t& offset)
            {
                DJV_BROKER_REQUIRE(offset <= data.size() && data.size() - offset >= 4);
                uint32_t out = 0;
                for (unsigned int i = 0; i < 4; ++i)
                {
                    out |= static_cast<uint32_t>(data[offset + i]) << (i * 8U);
                }
                offset += 4;
                return out;
            }

            uint64_t readU64(const std::vector<uint8_t>& data, size_t& offset)
            {
                DJV_BROKER_REQUIRE(offset <= data.size() && data.size() - offset >= 8);
                uint64_t out = 0;
                for (unsigned int i = 0; i < 8; ++i)
                {
                    out |= static_cast<uint64_t>(data[offset + i]) << (i * 8U);
                }
                offset += 8;
                return out;
            }

            std::vector<uint8_t> makeHeader(
                uint32_t payloadBytes,
                uint64_t requestId,
                uint16_t version = app::WindowsOpenBroker::protocolVersion,
                uint16_t type = 1)
            {
                std::vector<uint8_t> out = { 'D', 'J', 'V', 'O' };
                appendU16(out, version);
                appendU16(out, type);
                appendU32(out, payloadBytes);
                appendU64(out, requestId);
                DJV_BROKER_REQUIRE(20 == out.size());
                return out;
            }

            bool transfer(
                HANDLE pipe,
                bool write,
                uint8_t* data,
                size_t size,
                DWORD timeout = 3000)
            {
                size_t offset = 0;
                while (offset < size)
                {
                    TestHandle event(::CreateEventW(nullptr, TRUE, FALSE, nullptr));
                    DJV_BROKER_REQUIRE(event);
                    OVERLAPPED overlapped = {};
                    overlapped.hEvent = event.get();
                    const DWORD requested = static_cast<DWORD>((std::min)(
                        size - offset,
                        static_cast<size_t>(64U * 1024U)));
                    DWORD transferred = 0;
                    const BOOL immediate = write ?
                        ::WriteFile(
                            pipe,
                            data + offset,
                            requested,
                            &transferred,
                            &overlapped) :
                        ::ReadFile(
                            pipe,
                            data + offset,
                            requested,
                            &transferred,
                            &overlapped);
                    if (!immediate)
                    {
                        const DWORD operationError = ::GetLastError();
                        if (ERROR_IO_PENDING != operationError)
                        {
                            return false;
                        }
                        if (WAIT_OBJECT_0 != ::WaitForSingleObject(event.get(), timeout))
                        {
                            ::CancelIoEx(pipe, &overlapped);
                            ::GetOverlappedResult(pipe, &overlapped, &transferred, TRUE);
                            return false;
                        }
                        if (!::GetOverlappedResult(
                                pipe, &overlapped, &transferred, FALSE))
                        {
                            return false;
                        }
                    }
                    if (!transferred)
                    {
                        return false;
                    }
                    offset += transferred;
                }
                return true;
            }

            bool writeBytes(HANDLE pipe, const std::vector<uint8_t>& data)
            {
                return data.empty() || transfer(
                    pipe,
                    true,
                    const_cast<uint8_t*>(data.data()),
                    data.size());
            }

            bool readBytes(HANDLE pipe, std::vector<uint8_t>& data)
            {
                return data.empty() || transfer(
                    pipe, false, data.data(), data.size());
            }

            TestHandle connectRaw(const std::wstring& pipeName)
            {
                const auto deadline =
                    std::chrono::steady_clock::now() + std::chrono::seconds(3);
                while (std::chrono::steady_clock::now() < deadline)
                {
                    const HANDLE pipe = ::CreateFileW(
                        pipeName.c_str(),
                        GENERIC_READ | GENERIC_WRITE | READ_CONTROL,
                        0,
                        nullptr,
                        OPEN_EXISTING,
                        FILE_FLAG_OVERLAPPED |
                            SECURITY_SQOS_PRESENT |
                            SECURITY_IDENTIFICATION,
                        nullptr);
                    if (INVALID_HANDLE_VALUE != pipe)
                    {
                        return TestHandle(pipe);
                    }
                    const DWORD error = ::GetLastError();
                    if (ERROR_PIPE_BUSY == error)
                    {
                        ::WaitNamedPipeW(pipeName.c_str(), 100);
                    }
                    else if (ERROR_FILE_NOT_FOUND == error)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }
                    else
                    {
                        break;
                    }
                }
                return TestHandle();
            }

            uint32_t rawExchange(
                const std::wstring& pipeName,
                const std::vector<uint8_t>& request,
                uint64_t expectedRequestId,
                std::string& responseMessage)
            {
                TestHandle pipe = connectRaw(pipeName);
                DJV_BROKER_REQUIRE(pipe);
                DJV_BROKER_REQUIRE(writeBytes(pipe.get(), request));

                std::vector<uint8_t> header(20);
                DJV_BROKER_REQUIRE(readBytes(pipe.get(), header));
                DJV_BROKER_REQUIRE(0 == std::memcmp(
                    header.data(), "DJVO", 4));
                size_t offset = 4;
                DJV_BROKER_REQUIRE(
                    app::WindowsOpenBroker::protocolVersion == readU16(header, offset));
                DJV_BROKER_REQUIRE(2 == readU16(header, offset));
                const uint32_t payloadBytes = readU32(header, offset);
                DJV_BROKER_REQUIRE(expectedRequestId == readU64(header, offset));
                DJV_BROKER_REQUIRE(20 == offset);
                DJV_BROKER_REQUIRE(
                    payloadBytes <= app::WindowsOpenBroker::maxResponseMessageBytes + 8);

                std::vector<uint8_t> payload(payloadBytes);
                DJV_BROKER_REQUIRE(readBytes(pipe.get(), payload));
                const std::vector<uint8_t> receipt(1, 0xA5);
                DJV_BROKER_REQUIRE(writeBytes(pipe.get(), receipt));

                offset = 0;
                const uint32_t responseCode = readU32(payload, offset);
                const uint32_t messageBytes = readU32(payload, offset);
                DJV_BROKER_REQUIRE(offset <= payload.size());
                DJV_BROKER_REQUIRE(payload.size() - offset == messageBytes);
                responseMessage.assign(
                    reinterpret_cast<const char*>(payload.data() + offset),
                    messageBytes);
                return responseCode;
            }

            std::vector<uint8_t> currentUserSid()
            {
                HANDLE rawToken = nullptr;
                DJV_BROKER_REQUIRE(::OpenProcessToken(
                    ::GetCurrentProcess(), TOKEN_QUERY, &rawToken));
                TestHandle token(rawToken);
                DWORD size = 0;
                ::GetTokenInformation(token.get(), TokenUser, nullptr, 0, &size);
                DJV_BROKER_REQUIRE(size > 0 && ERROR_INSUFFICIENT_BUFFER == ::GetLastError());
                std::vector<uint8_t> buffer(size);
                DJV_BROKER_REQUIRE(::GetTokenInformation(
                    token.get(), TokenUser, buffer.data(), size, &size));
                const auto tokenUser = reinterpret_cast<const TOKEN_USER*>(buffer.data());
                const DWORD sidBytes = ::GetLengthSid(tokenUser->User.Sid);
                std::vector<uint8_t> out(sidBytes);
                DJV_BROKER_REQUIRE(::CopySid(
                    sidBytes, out.data(), tokenUser->User.Sid));
                return out;
            }

            std::vector<uint8_t> currentLogonSid()
            {
                HANDLE rawToken = nullptr;
                DJV_BROKER_REQUIRE(::OpenProcessToken(
                    ::GetCurrentProcess(), TOKEN_QUERY, &rawToken));
                TestHandle token(rawToken);
                DWORD size = 0;
                ::GetTokenInformation(token.get(), TokenGroups, nullptr, 0, &size);
                DJV_BROKER_REQUIRE(
                    size > 0 && ERROR_INSUFFICIENT_BUFFER == ::GetLastError());
                std::vector<uint8_t> buffer(size);
                DJV_BROKER_REQUIRE(::GetTokenInformation(
                    token.get(), TokenGroups, buffer.data(), size, &size));
                const auto groups =
                    reinterpret_cast<const TOKEN_GROUPS*>(buffer.data());
                for (DWORD i = 0; i < groups->GroupCount; ++i)
                {
                    const auto& group = groups->Groups[i];
                    if (SE_GROUP_LOGON_ID ==
                        (group.Attributes & SE_GROUP_LOGON_ID))
                    {
                        const DWORD sidBytes = ::GetLengthSid(group.Sid);
                        std::vector<uint8_t> out(sidBytes);
                        DJV_BROKER_REQUIRE(::CopySid(
                            sidBytes, out.data(), group.Sid));
                        return out;
                    }
                }
                DJV_BROKER_REQUIRE(false);
                return {};
            }

            void verifyLogonDacl(HANDLE handle)
            {
                PACL dacl = nullptr;
                PSECURITY_DESCRIPTOR descriptor = nullptr;
                const DWORD result = ::GetSecurityInfo(
                    handle,
                    SE_KERNEL_OBJECT,
                    DACL_SECURITY_INFORMATION,
                    nullptr,
                    nullptr,
                    &dacl,
                    nullptr,
                    &descriptor);
                DJV_BROKER_REQUIRE(ERROR_SUCCESS == result);
                DJV_BROKER_REQUIRE(descriptor);
                DJV_BROKER_REQUIRE(dacl);

                ACL_SIZE_INFORMATION info = {};
                DJV_BROKER_REQUIRE(::GetAclInformation(
                    dacl,
                    &info,
                    sizeof(info),
                    AclSizeInformation));
                DJV_BROKER_REQUIRE(1 == info.AceCount);
                void* rawAce = nullptr;
                DJV_BROKER_REQUIRE(::GetAce(dacl, 0, &rawAce));
                const auto header = reinterpret_cast<const ACE_HEADER*>(rawAce);
                DJV_BROKER_REQUIRE(ACCESS_ALLOWED_ACE_TYPE == header->AceType);
                const auto ace = reinterpret_cast<const ACCESS_ALLOWED_ACE*>(rawAce);
                const auto logonSid = currentLogonSid();
                const auto userSid = currentUserSid();
                DJV_BROKER_REQUIRE(::EqualSid(
                    const_cast<DWORD*>(&ace->SidStart),
                    const_cast<uint8_t*>(logonSid.data())));
                DJV_BROKER_REQUIRE(!::EqualSid(
                    const_cast<DWORD*>(&ace->SidStart),
                    const_cast<uint8_t*>(userSid.data())));
                DJV_BROKER_REQUIRE(0 != ace->Mask);

                SECURITY_DESCRIPTOR_CONTROL control = 0;
                DWORD revision = 0;
                DJV_BROKER_REQUIRE(::GetSecurityDescriptorControl(
                    descriptor, &control, &revision));
                DJV_BROKER_REQUIRE(0 != (control & SE_DACL_PROTECTED));
                ::LocalFree(descriptor);
            }

            void verifyMutexDacl(const app::WindowsOpenBrokerIdentity& identity)
            {
                TestHandle mutex(::OpenMutexW(
                    READ_CONTROL,
                    FALSE,
                    identity.mutexName.c_str()));
                DJV_BROKER_REQUIRE(mutex);
                verifyLogonDacl(mutex.get());
            }

            std::string uniqueApplicationId(const char* suffix)
            {
                std::ostringstream stream;
                stream << "DJV.WindowsOpenBrokerTest."
                       << ::GetCurrentProcessId() << "." << suffix;
                return stream.str();
            }

            void normalizationTest()
            {
                const std::filesystem::path cwd =
                    std::filesystem::path(getTempPath()) / L"DJV-\u00E9";
                const std::string cwdUtf8 = wideToUtf8(cwd.native());
                const std::vector<std::string> paths = {
                    std::string("plans\\s\xC3\xA9" "quence.mov"),
                    std::string("..\\audio\\son.wav") };
                std::vector<std::string> normalized;
                std::string error;
                DJV_BROKER_REQUIRE(app::WindowsOpenBroker::normalizePaths(
                    paths, cwdUtf8, normalized, error));
                DJV_BROKER_REQUIRE(2 == normalized.size());
                DJV_BROKER_REQUIRE(
                    wideToUtf8((cwd / L"plans\\s\u00E9quence.mov").lexically_normal().native()) ==
                    normalized[0]);
                DJV_BROKER_REQUIRE(
                    wideToUtf8((cwd / L"..\\audio\\son.wav").lexically_normal().native()) ==
                    normalized[1]);

                DJV_BROKER_REQUIRE(!app::WindowsOpenBroker::normalizePaths(
                    { "C:ambiguous.mov" }, cwdUtf8, normalized, error));
                DJV_BROKER_REQUIRE(!app::WindowsOpenBroker::normalizePaths(
                    { "\\root-relative.mov" }, cwdUtf8, normalized, error));
                DJV_BROKER_REQUIRE(!app::WindowsOpenBroker::normalizePaths(
                    { "\\\\.\\pipe\\not-a-media-file" }, cwdUtf8, normalized, error));
                DJV_BROKER_REQUIRE(!app::WindowsOpenBroker::normalizePaths(
                    { std::string("\xC3\x28", 2) }, cwdUtf8, normalized, error));
                DJV_BROKER_REQUIRE(!app::WindowsOpenBroker::normalizePaths(
                    std::vector<std::string>(
                        app::WindowsOpenBroker::maxPathCount + 1,
                        "C:\\file.mov"),
                    cwdUtf8,
                    normalized,
                    error));
                DJV_BROKER_REQUIRE(!app::WindowsOpenBroker::normalizePaths(
                    { std::string(app::WindowsOpenBroker::maxPathBytes + 1, 'x') },
                    cwdUtf8,
                    normalized,
                    error));
            }

            void identityAndRoundTripTest()
            {
                app::WindowsOpenBrokerOptions options;
                options.applicationId = uniqueApplicationId("roundtrip");
                options.connectTimeout = std::chrono::milliseconds(1000);
                options.ioTimeout = std::chrono::milliseconds(1000);
                options.startTimeout = std::chrono::milliseconds(1000);

                app::WindowsOpenBrokerIdentity firstIdentity;
                app::WindowsOpenBrokerIdentity secondIdentity;
                std::string error;
                DJV_BROKER_REQUIRE(app::WindowsOpenBroker::resolveIdentity(
                    options, firstIdentity, error));
                DJV_BROKER_REQUIRE(app::WindowsOpenBroker::resolveIdentity(
                    options, secondIdentity, error));
                DJV_BROKER_REQUIRE(firstIdentity.pipeName == secondIdentity.pipeName);
                DJV_BROKER_REQUIRE(firstIdentity.mutexName == secondIdentity.mutexName);
                DJV_BROKER_REQUIRE(firstIdentity.userSid == secondIdentity.userSid);
                DJV_BROKER_REQUIRE(firstIdentity.logonSid == secondIdentity.logonSid);
                DJV_BROKER_REQUIRE(!firstIdentity.logonSid.empty());
                DJV_BROKER_REQUIRE(firstIdentity.userSid != firstIdentity.logonSid);
                DJV_BROKER_REQUIRE(firstIdentity.rejectsRemoteClients);

                app::WindowsOpenBrokerOptions otherOptions = options;
                otherOptions.applicationId += ".other";
                app::WindowsOpenBrokerIdentity otherIdentity;
                DJV_BROKER_REQUIRE(app::WindowsOpenBroker::resolveIdentity(
                    otherOptions, otherIdentity, error));
                DJV_BROKER_REQUIRE(firstIdentity.pipeName != otherIdentity.pipeName);

                std::mutex captureMutex;
                std::vector<std::string> captured;
                app::WindowsOpenBroker broker(options);
                const auto startResult = broker.start(
                    [&captureMutex, &captured](
                        const std::vector<std::string>& paths,
                        std::string& message)
                    {
                        if (std::string::npos != paths.front().find("throw"))
                        {
                            throw std::runtime_error("deliberate callback failure");
                        }
                        if (std::string::npos != paths.front().find("slow"))
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(250));
                        }
                        if (std::string::npos != paths.front().find("reject"))
                        {
                            message = "deliberate rejection";
                            return false;
                        }
                        std::lock_guard<std::mutex> lock(captureMutex);
                        captured = paths;
                        return true;
                    });
                DJV_BROKER_REQUIRE(startResult);
                DJV_BROKER_REQUIRE(broker.isRunning());
                verifyMutexDacl(broker.getIdentity());
                {
                    TestHandle pipe = connectRaw(broker.getIdentity().pipeName);
                    DJV_BROKER_REQUIRE(pipe);
                    verifyLogonDacl(pipe.get());
                }

                app::WindowsOpenBroker duplicate(options);
                const auto duplicateResult = duplicate.start(
                    [](const std::vector<std::string>&, std::string&)
                    {
                        return true;
                    });
                DJV_BROKER_REQUIRE(
                    app::WindowsOpenBrokerStartStatus::AlreadyRunning ==
                    duplicateResult.status);

                const std::filesystem::path cwd =
                    std::filesystem::path(getTempPath()) / L"DJV-\u00E9";
                const std::string cwdUtf8 = wideToUtf8(cwd.native());
                const auto forwarded = app::WindowsOpenBroker::forward(
                    { std::string("clips\\s\xC3\xA9" "quence.mov") },
                    cwdUtf8,
                    options);
                DJV_BROKER_REQUIRE(forwarded);
                {
                    std::lock_guard<std::mutex> lock(captureMutex);
                    DJV_BROKER_REQUIRE(1 == captured.size());
                    DJV_BROKER_REQUIRE(
                        wideToUtf8((cwd / L"clips\\s\u00E9quence.mov").lexically_normal().native()) ==
                        captured.front());
                }

                const auto rejected = app::WindowsOpenBroker::forward(
                    { "reject.mov" }, cwdUtf8, options);
                DJV_BROKER_REQUIRE(
                    app::WindowsOpenForwardStatus::Rejected == rejected.status);
                DJV_BROKER_REQUIRE("deliberate rejection" == rejected.error);

                const auto threw = app::WindowsOpenBroker::forward(
                    { "throw.mov" }, cwdUtf8, options);
                DJV_BROKER_REQUIRE(
                    app::WindowsOpenForwardStatus::DeliveryUnknown == threw.status);
                DJV_BROKER_REQUIRE(std::string::npos !=
                    broker.getLastError().find("deliberate callback failure"));

                // The client deadline is independent from the server timeout.
                app::WindowsOpenBrokerOptions shortClientOptions = options;
                shortClientOptions.ioTimeout = std::chrono::milliseconds(50);
                const auto timedOut = app::WindowsOpenBroker::forward(
                    { "slow.mov" }, cwdUtf8, shortClientOptions);
                DJV_BROKER_REQUIRE(
                    app::WindowsOpenForwardStatus::DeliveryUnknown == timedOut.status);
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                {
                    std::lock_guard<std::mutex> lock(captureMutex);
                    DJV_BROKER_REQUIRE(1 == captured.size());
                    DJV_BROKER_REQUIRE(std::string::npos !=
                        captured.front().find("slow.mov"));
                }
                DJV_BROKER_REQUIRE(app::WindowsOpenBroker::forward(
                    { "after-timeout.mov" }, cwdUtf8, options));

                // Malformed magic receives an explicit protocol NACK.
                std::vector<uint8_t> invalidMagic = makeHeader(0, 1);
                invalidMagic[0] = 'X';
                std::string responseMessage;
                DJV_BROKER_REQUIRE(6 == rawExchange(
                    broker.getIdentity().pipeName,
                    invalidMagic,
                    0,
                    responseMessage));

                // Oversized declared payload is rejected before allocation/read.
                const auto oversizedPayload = makeHeader(
                    static_cast<uint32_t>(app::WindowsOpenBroker::maxPayloadBytes + 1),
                    2);
                DJV_BROKER_REQUIRE(7 == rawExchange(
                    broker.getIdentity().pipeName,
                    oversizedPayload,
                    2,
                    responseMessage));

                // An oversized path count is rejected from a bounded payload.
                std::vector<uint8_t> countPayload;
                appendU32(countPayload, 0);
                appendU32(
                    countPayload,
                    static_cast<uint32_t>(app::WindowsOpenBroker::maxPathCount + 1));
                auto oversizedCount = makeHeader(
                    static_cast<uint32_t>(countPayload.size()),
                    3);
                oversizedCount.insert(
                    oversizedCount.end(), countPayload.begin(), countPayload.end());
                DJV_BROKER_REQUIRE(1 == rawExchange(
                    broker.getIdentity().pipeName,
                    oversizedCount,
                    3,
                    responseMessage));

                // An oversized individual path is rejected before path storage.
                std::vector<uint8_t> pathPayload;
                appendU32(pathPayload, 0);
                appendU32(pathPayload, 1);
                appendU32(
                    pathPayload,
                    static_cast<uint32_t>(app::WindowsOpenBroker::maxPathBytes + 1));
                auto oversizedPath = makeHeader(
                    static_cast<uint32_t>(pathPayload.size()),
                    4);
                oversizedPath.insert(
                    oversizedPath.end(), pathPayload.begin(), pathPayload.end());
                DJV_BROKER_REQUIRE(1 == rawExchange(
                    broker.getIdentity().pipeName,
                    oversizedPath,
                    4,
                    responseMessage));

                // Invalid UTF-8 reaches the same explicit invalid-request path.
                std::vector<uint8_t> invalidUtf8Payload;
                appendU32(invalidUtf8Payload, 0);
                appendU32(invalidUtf8Payload, 1);
                appendU32(invalidUtf8Payload, 2);
                invalidUtf8Payload.push_back(0xC3);
                invalidUtf8Payload.push_back(0x28);
                auto invalidUtf8 = makeHeader(
                    static_cast<uint32_t>(invalidUtf8Payload.size()),
                    5);
                invalidUtf8.insert(
                    invalidUtf8.end(),
                    invalidUtf8Payload.begin(),
                    invalidUtf8Payload.end());
                DJV_BROKER_REQUIRE(1 == rawExchange(
                    broker.getIdentity().pipeName,
                    invalidUtf8,
                    5,
                    responseMessage));

                // A future protocol version receives a NACK, not a silent drop.
                const auto futureVersion = makeHeader(0, 6, 99);
                DJV_BROKER_REQUIRE(6 == rawExchange(
                    broker.getIdentity().pipeName,
                    futureVersion,
                    6,
                    responseMessage));

                broker.stop();
                broker.stop();
                DJV_BROKER_REQUIRE(!broker.isRunning());

                // A primary marker without a ready pipe is a bounded startup
                // race/health timeout, not a false "no server" result.
                TestHandle marker(::CreateMutexW(
                    nullptr,
                    FALSE,
                    broker.getIdentity().mutexName.c_str()));
                DJV_BROKER_REQUIRE(marker);
                app::WindowsOpenBrokerOptions markerOptions = options;
                markerOptions.connectTimeout = std::chrono::milliseconds(50);
                const auto markerTimeout = app::WindowsOpenBroker::forward(
                    { "marker-timeout.mov" }, cwdUtf8, markerOptions);
                DJV_BROKER_REQUIRE(
                    app::WindowsOpenForwardStatus::Timeout == markerTimeout.status);
                marker = TestHandle();
                const auto noServer = app::WindowsOpenBroker::forward(
                    { "after-stop.mov" }, cwdUtf8, options);
                DJV_BROKER_REQUIRE(
                    app::WindowsOpenForwardStatus::NoServer == noServer.status);
            }

            void boundedStopTest()
            {
                app::WindowsOpenBrokerOptions options;
                options.applicationId = uniqueApplicationId("stop");
                options.ioTimeout = std::chrono::seconds(10);
                options.startTimeout = std::chrono::seconds(1);
                app::WindowsOpenBroker broker(options);
                DJV_BROKER_REQUIRE(broker.start(
                    [](const std::vector<std::string>&, std::string&)
                    {
                        return true;
                    }));

                // Connect and deliberately send no bytes. stop() must cancel
                // the pending overlapped read instead of waiting for ioTimeout.
                TestHandle idleClient = connectRaw(broker.getIdentity().pipeName);
                DJV_BROKER_REQUIRE(idleClient);
                const auto start = std::chrono::steady_clock::now();
                std::thread stopper([&broker] { broker.stop(); });
                stopper.join();
                const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start);
                DJV_BROKER_REQUIRE(elapsed < std::chrono::seconds(2));
                DJV_BROKER_REQUIRE(!broker.isRunning());
            }

            void concurrentStopTest()
            {
                for (int iteration = 0; iteration < 12; ++iteration)
                {
                    app::WindowsOpenBrokerOptions options;
                    options.applicationId = uniqueApplicationId(
                        ("concurrent-stop-" + std::to_string(iteration)).c_str());
                    options.connectTimeout = std::chrono::seconds(1);
                    options.ioTimeout = std::chrono::seconds(2);
                    options.startTimeout = std::chrono::seconds(1);

                    app::WindowsOpenBroker broker(options);
                    std::atomic<bool> callbackEntered(false);
                    std::atomic<bool> releaseCallback(false);
                    std::atomic<bool> callbackStopReturned(false);
                    DJV_BROKER_REQUIRE(broker.start(
                        [&broker, &callbackEntered, &releaseCallback,
                         &callbackStopReturned](
                            const std::vector<std::string>&,
                            std::string&)
                        {
                            callbackEntered = true;
                            while (!releaseCallback.load())
                            {
                                std::this_thread::yield();
                            }
                            // An external caller owns the join by this point.
                            // This self-stop must return rather than wait for
                            // that caller, which is waiting for this callback.
                            broker.stop();
                            callbackStopReturned = true;
                            return true;
                        }));

                    app::WindowsOpenForwardResult forwardResult;
                    std::thread forwarder([&]
                        {
                            forwardResult = app::WindowsOpenBroker::forward(
                                { "concurrent-stop.mov" },
                                wideToUtf8(getTempPath()),
                                options);
                        });
                    const auto callbackDeadline =
                        std::chrono::steady_clock::now() + std::chrono::seconds(2);
                    while (!callbackEntered.load() &&
                           std::chrono::steady_clock::now() < callbackDeadline)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    DJV_BROKER_REQUIRE(callbackEntered.load());

                    std::atomic<bool> externalStopStarted(false);
                    std::atomic<bool> externalStopReturned(false);
                    std::thread stopper([&]
                        {
                            externalStopStarted = true;
                            broker.stop();
                            externalStopReturned = true;
                        });
                    while (!externalStopStarted.load())
                    {
                        std::this_thread::yield();
                    }
                    // Give the external caller time to claim join ownership
                    // while the callback is deliberately held at the gate.
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    releaseCallback = true;

                    stopper.join();
                    forwarder.join();
                    DJV_BROKER_REQUIRE(callbackStopReturned.load());
                    DJV_BROKER_REQUIRE(externalStopReturned.load());
                    DJV_BROKER_REQUIRE(!broker.isRunning());
                    DJV_BROKER_REQUIRE(
                        app::WindowsOpenForwardStatus::Forwarded ==
                            forwardResult.status ||
                        app::WindowsOpenForwardStatus::DeliveryUnknown ==
                            forwardResult.status ||
                        app::WindowsOpenForwardStatus::IOError ==
                            forwardResult.status);

                    // Idempotence remains true after the concurrent cleanup.
                    broker.stop();
                }
            }

            void callbackDestructionTest()
            {
                app::WindowsOpenBrokerOptions options;
                options.applicationId = uniqueApplicationId("callback-destroy");
                options.connectTimeout = std::chrono::seconds(1);
                options.ioTimeout = std::chrono::seconds(1);
                options.startTimeout = std::chrono::seconds(1);

                std::atomic<app::WindowsOpenBroker*> broker(
                    new app::WindowsOpenBroker(options));
                std::atomic<bool> destroyed(false);
                DJV_BROKER_REQUIRE(broker.load()->start(
                    [&broker, &destroyed](
                        const std::vector<std::string>&,
                        std::string&)
                    {
                        app::WindowsOpenBroker* value = broker.exchange(nullptr);
                        delete value;
                        destroyed = true;
                        return true;
                    }));

                const auto result = app::WindowsOpenBroker::forward(
                    { "destroy-from-callback.mov" },
                    wideToUtf8(getTempPath()),
                    options);
                const auto deadline =
                    std::chrono::steady_clock::now() + std::chrono::seconds(2);
                while (!destroyed.load() &&
                       std::chrono::steady_clock::now() < deadline)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
                DJV_BROKER_REQUIRE(destroyed.load());
                DJV_BROKER_REQUIRE(nullptr == broker.load());
                DJV_BROKER_REQUIRE(
                    app::WindowsOpenForwardStatus::Forwarded == result.status ||
                    app::WindowsOpenForwardStatus::DeliveryUnknown == result.status);
                // The detached worker owns its state until run() exits. Reaching
                // this point proves no joinable-thread terminate/use-after-free.
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

#if defined(DJV_WINDOWS_OPEN_BROKER_TESTING)
            std::atomic<bool> failStopObserved(false);

            bool failRevertToSelf()
            {
                ::SetLastError(ERROR_ACCESS_DENIED);
                return false;
            }

            void throwFromFailStop(const std::string& message)
            {
                failStopObserved = true;
                throw std::runtime_error(std::string("test fail-stop: ") + message);
            }

            void revertFailureFailStopTest()
            {
                app::WindowsOpenBrokerOptions options;
                options.applicationId = uniqueApplicationId("revert-fail-stop");
                options.connectTimeout = std::chrono::seconds(1);
                options.ioTimeout = std::chrono::seconds(1);
                options.startTimeout = std::chrono::seconds(1);

                failStopObserved = false;
                app::setWindowsOpenBrokerSecurityTestHooks(
                    failRevertToSelf, throwFromFailStop);
                app::WindowsOpenBroker broker(options);
                DJV_BROKER_REQUIRE(broker.start(
                    [](const std::vector<std::string>&, std::string&)
                    {
                        return true;
                    }));
                const auto result = app::WindowsOpenBroker::forward(
                    { "must-not-reach-callback.mov" },
                    wideToUtf8(getTempPath()),
                    options);
                const auto deadline =
                    std::chrono::steady_clock::now() + std::chrono::seconds(2);
                while (broker.isRunning() &&
                       std::chrono::steady_clock::now() < deadline)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
                broker.stop();
                app::setWindowsOpenBrokerSecurityTestHooks(nullptr, nullptr);

                DJV_BROKER_REQUIRE(!result);
                DJV_BROKER_REQUIRE(failStopObserved.load());
                DJV_BROKER_REQUIRE(std::string::npos !=
                    broker.getLastError().find("RevertToSelf"));
            }
#endif // DJV_WINDOWS_OPEN_BROKER_TESTING
#endif // _WIN32
        }

        int runWindowsOpenBrokerTests()
        {
            try
            {
#if defined(_WIN32)
                normalizationTest();
                std::cout << "[PASS] UTF-8 and relative path normalization\n";
                identityAndRoundTripTest();
                std::cout << "[PASS] identity, DACL, ACK/NACK, malformed bounds\n";
                boundedStopTest();
                std::cout << "[PASS] bounded cancellation and clean stop\n";
                concurrentStopTest();
                std::cout << "[PASS] concurrent external/callback stop is race-safe\n";
                callbackDestructionTest();
                std::cout << "[PASS] callback-triggered destruction is lifetime-safe\n";
#if defined(DJV_WINDOWS_OPEN_BROKER_TESTING)
                revertFailureFailStopTest();
                std::cout << "[PASS] RevertToSelf failure takes the fail-stop path\n";
#endif // DJV_WINDOWS_OPEN_BROKER_TESTING
#else // _WIN32
                app::WindowsOpenBroker broker;
                const auto result = broker.start(
                    [](const std::vector<std::string>&, std::string&)
                    {
                        return true;
                    });
                DJV_BROKER_REQUIRE(
                    app::WindowsOpenBrokerStartStatus::Unsupported == result.status);
                std::cout << "[PASS] non-Windows unsupported stub\n";
#endif // _WIN32
                return 0;
            }
            catch (const std::exception& exception)
            {
                std::cerr << "[FAIL] " << exception.what() << '\n';
                return 1;
            }
        }
    }
}

#if defined(DJV_WINDOWS_OPEN_BROKER_TEST_MAIN)
int main()
{
    return djv::app_tests::runWindowsOpenBrokerTests();
}
#endif
