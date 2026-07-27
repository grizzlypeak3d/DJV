// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace djv
{
    namespace app
    {
        //! Options shared by the Windows single-instance server and client.
        struct WindowsOpenBrokerOptions
        {
            //! Stable application identifier. It is hashed with the current
            //! logon SID and Windows session; it is never used directly as an
            //! operating-system object name.
            std::string applicationId = "DJV";

            //! Maximum time for a client to wait for the server pipe.
            std::chrono::milliseconds connectTimeout =
                std::chrono::milliseconds(1500);

            //! Maximum time for each complete request or response transfer.
            std::chrono::milliseconds ioTimeout =
                std::chrono::milliseconds(2000);

            //! Maximum time start() waits for the server pipe to become ready.
            std::chrono::milliseconds startTimeout =
                std::chrono::milliseconds(2000);
        };

        enum class WindowsOpenBrokerStartStatus
        {
            Started,
            AlreadyRunning,
            Unsupported,
            InvalidOptions,
            SecurityError,
            IOError
        };

        struct WindowsOpenBrokerStartResult
        {
            WindowsOpenBrokerStartStatus status =
                WindowsOpenBrokerStartStatus::IOError;
            std::string error;

            explicit operator bool() const noexcept
            {
                return WindowsOpenBrokerStartStatus::Started == status;
            }
        };

        enum class WindowsOpenForwardStatus
        {
            Forwarded,
            //! The complete request reached the server pipe, but no valid
            //! terminal response was received. The callback may already have
            //! run; callers must not retry or open locally by default.
            DeliveryUnknown,
            NoServer,
            Rejected,
            InvalidArgument,
            Timeout,
            ProtocolError,
            SecurityError,
            IOError,
            Unsupported
        };

        struct WindowsOpenForwardResult
        {
            WindowsOpenForwardStatus status = WindowsOpenForwardStatus::IOError;
            std::string error;

            explicit operator bool() const noexcept
            {
                return WindowsOpenForwardStatus::Forwarded == status;
            }
        };

        //! Inspectable, non-secret identity of the per-user broker objects.
        struct WindowsOpenBrokerIdentity
        {
            std::wstring pipeName;
            std::wstring mutexName;
            std::wstring userSid;
            //! Per-logon SID used by the object DACL. Unlike userSid, this SID
            //! does not grant another RUNAS/RDP session of the same account.
            std::wstring logonSid;
            uint32_t sessionId = 0;
            bool rejectsRemoteClients = false;
        };

        //! Secure, bounded single-instance open-request broker for Windows.
        //!
            //! The callback runs on the broker worker thread. It must only validate
            //! and enqueue the request for the UI thread; it must not perform long
            //! or blocking work. Returning false sends an explicit NACK to the
            //! client. The optional message is capped before it crosses the pipe.
            //! Calling stop(), or even destroying the broker, from the callback is
            //! supported; the worker keeps its private state alive until it exits.
        class WindowsOpenBroker
        {
        public:
            using Callback = std::function<bool(
                const std::vector<std::string>&,
                std::string&)>;

            static constexpr uint16_t protocolVersion = 1;
            static constexpr size_t maxPathCount = 128;
            static constexpr size_t maxPathBytes = 64U * 1024U;
            static constexpr size_t maxPayloadBytes = 1024U * 1024U;
            static constexpr size_t maxResponseMessageBytes = 4096;

            explicit WindowsOpenBroker(
                const WindowsOpenBrokerOptions& = WindowsOpenBrokerOptions());
            ~WindowsOpenBroker();

            WindowsOpenBroker(const WindowsOpenBroker&) = delete;
            WindowsOpenBroker& operator=(const WindowsOpenBroker&) = delete;
            WindowsOpenBroker(WindowsOpenBroker&&) = delete;
            WindowsOpenBroker& operator=(WindowsOpenBroker&&) = delete;

            //! Acquire the per-logon primary slot and start accepting requests.
            WindowsOpenBrokerStartResult start(Callback);

            //! Cancel pending pipe I/O and join the worker. This is idempotent.
            void stop() noexcept;

            bool isRunning() const noexcept;
            std::string getLastError() const;
            const WindowsOpenBrokerIdentity& getIdentity() const noexcept;

            //! Forward paths to an existing broker and wait for ACK/NACK.
            //!
            //! Relative paths require an absolute UTF-8 working directory.
            //! The server independently validates and normalizes all paths.
            static WindowsOpenForwardResult forward(
                const std::vector<std::string>& paths,
                const std::string& workingDirectoryUtf8,
                const WindowsOpenBrokerOptions& = WindowsOpenBrokerOptions());

            //! Validate UTF-8 and resolve every input to a lexical absolute path.
            static bool normalizePaths(
                const std::vector<std::string>& paths,
                const std::string& workingDirectoryUtf8,
                std::vector<std::string>& out,
                std::string& error);

            //! Resolve the deterministic identity for diagnostics and tests.
            static bool resolveIdentity(
                const WindowsOpenBrokerOptions&,
                WindowsOpenBrokerIdentity&,
                std::string& error);

        private:
            class Private;
            std::shared_ptr<Private> _p;
        };

#if defined(DJV_WINDOWS_OPEN_BROKER_TESTING)
        //! Dependency injection for the fail-stop security regression.
        //! This surface is absent from production builds.
        using WindowsOpenBrokerRevertToSelfTestHook = bool(*)();
        using WindowsOpenBrokerFailStopTestHook = void(*)(const std::string&);
        void setWindowsOpenBrokerSecurityTestHooks(
            WindowsOpenBrokerRevertToSelfTestHook,
            WindowsOpenBrokerFailStopTestHook) noexcept;
#endif // DJV_WINDOWS_OPEN_BROKER_TESTING
    }
}
