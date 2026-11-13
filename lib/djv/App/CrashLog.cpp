// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/CrashLog.h>

#include <ftk/Core/LogSystem.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

#if defined(_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#include <csignal>
#else // _WINDOWS
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#if defined(__APPLE__) || defined(__GLIBC__)
#include <execinfo.h>
#define DJV_CRASH_LOG_BACKTRACE
#endif // __APPLE__
#endif // _WINDOWS

// A sanitizer has handlers of its own, and its reports say more.
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define DJV_CRASH_LOG_SANITIZER
#endif
#if defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define DJV_CRASH_LOG_SANITIZER
#endif
#endif

namespace djv
{
    namespace app
    {
        namespace
        {
            const char* prefix = "djv::app::CrashLog";

            // What the handlers need, set up before any of them can run:
            // a signal handler may only write bytes it already has.
#if defined(_WINDOWS)
            std::wstring logPath;
            LPTOP_LEVEL_EXCEPTION_FILTER oldFilter = nullptr;
            void (*oldAbort)(int) = SIG_DFL;
#else // _WINDOWS
            int logFd = -1;
            const int signals[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT };
            const size_t signalCount = sizeof(signals) / sizeof(signals[0]);
            struct sigaction oldActions[signalCount];
            std::vector<char> altStack;
            stack_t oldAltStack;
#endif // _WINDOWS
            std::terminate_handler oldTerminate = nullptr;

            // Write bytes to the log. Safe in a signal handler on POSIX
            // (write and strlen are); on Windows the file is opened for
            // each write, since the log system opens it for each batch and
            // a handle held open could keep it out.
            void writeLog(const char* s)
            {
#if defined(_WINDOWS)
                if (logPath.empty())
                    return;
                HANDLE file = CreateFileW(
                    logPath.c_str(),
                    FILE_APPEND_DATA,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
                if (file != INVALID_HANDLE_VALUE)
                {
                    DWORD written = 0;
                    WriteFile(file, s, static_cast<DWORD>(std::strlen(s)), &written, NULL);
                    CloseHandle(file);
                }
#else // _WINDOWS
                if (logFd < 0)
                    return;
                const size_t size = std::strlen(s);
                ssize_t r = ::write(logFd, s, size);
                (void)r;
#endif // _WINDOWS
            }

            // A line as the log writes them, with the time.
            void writeLine(const std::string& value)
            {
                ftk::LogItem item;
                item.time = std::chrono::system_clock::now();
                item.prefix = prefix;
                item.message = value;
                writeLog((ftk::getLabel(item) + "\n").c_str());
            }

            void terminateHandler()
            {
                std::string message = "Crash: std::terminate";
                if (auto e = std::current_exception())
                {
                    message = "Crash: an exception nothing caught";
                    try
                    {
                        std::rethrow_exception(e);
                    }
                    catch (const std::exception& error)
                    {
                        message += ": ";
                        message += error.what();
                    }
                    catch (...)
                    {}
                }
                writeLine(message);
                // Abort as before, which the signal handlers follow with
                // the backtrace.
                if (oldTerminate)
                {
                    oldTerminate();
                }
                std::abort();
            }

#if defined(_WINDOWS)
            void writeHex(char* out, uint64_t value, int digits)
            {
                const char* hex = "0123456789ABCDEF";
                out[0] = '0';
                out[1] = 'x';
                for (int i = 0; i < digits; ++i)
                {
                    out[2 + i] = hex[(value >> ((digits - 1 - i) * 4)) & 0xF];
                }
                out[2 + digits] = 0;
            }

            LONG WINAPI exceptionFilter(EXCEPTION_POINTERS* info)
            {
                // A C++ exception nothing caught: the runtime's filter,
                // next in the chain, calls std::terminate, and the
                // terminate handler says what the exception was.
                if (0xE06D7363 == info->ExceptionRecord->ExceptionCode)
                {
                    return oldFilter ?
                        oldFilter(info) :
                        EXCEPTION_CONTINUE_SEARCH;
                }
                char code[16];
                char address[24];
                writeHex(code, info->ExceptionRecord->ExceptionCode, 8);
                writeHex(
                    address,
                    reinterpret_cast<uint64_t>(info->ExceptionRecord->ExceptionAddress),
                    16);
                writeLog(prefix);
                writeLog(": Crash: exception ");
                writeLog(code);
                writeLog(" at ");
                writeLog(address);
                writeLog("\n");
                return oldFilter ?
                    oldFilter(info) :
                    EXCEPTION_CONTINUE_SEARCH;
            }

            void abortHandler(int)
            {
                writeLog(prefix);
                writeLog(": Crash: abort\n");
                signal(SIGABRT, oldAbort);
                raise(SIGABRT);
            }
#else // _WINDOWS
            const char* signalText(int value)
            {
                switch (value)
                {
                case SIGSEGV: return ": Crash: SIGSEGV, a bad memory access\n";
                case SIGBUS: return ": Crash: SIGBUS, a bad memory access\n";
                case SIGILL: return ": Crash: SIGILL, an illegal instruction\n";
                case SIGFPE: return ": Crash: SIGFPE, an arithmetic error\n";
                case SIGABRT: return ": Crash: SIGABRT, an abort\n";
                default: break;
                }
                return ": Crash: a signal\n";
            }

            void signalHandler(int value, siginfo_t*, void*)
            {
                writeLog(prefix);
                writeLog(signalText(value));
#if defined(DJV_CRASH_LOG_BACKTRACE)
                if (logFd >= 0)
                {
                    void* frames[64];
                    const int count = backtrace(frames, 64);
                    backtrace_symbols_fd(frames, count, logFd);
                }
#endif // DJV_CRASH_LOG_BACKTRACE
                // Put back what handled the signal before and send it
                // again: the system's crash reporter, or a handler of the
                // program's own, still sees it.
                for (size_t i = 0; i < signalCount; ++i)
                {
                    if (signals[i] == value)
                    {
                        sigaction(value, &oldActions[i], nullptr);
                        break;
                    }
                }
                raise(value);
            }
#endif // _WINDOWS
        }

        CrashLog::CrashLog(const std::filesystem::path& logFile)
        {
#if !defined(DJV_CRASH_LOG_SANITIZER)
            if (logFile.empty())
                return;
#if defined(_WINDOWS)
            logPath = logFile.wstring();
            oldFilter = SetUnhandledExceptionFilter(exceptionFilter);
            oldAbort = signal(SIGABRT, abortHandler);
#else // _WINDOWS
            logFd = ::open(logFile.c_str(), O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644);
            if (logFd < 0)
                return;
#if defined(DJV_CRASH_LOG_BACKTRACE)
            // The first call loads what it needs, which a signal handler
            // must not have to.
            void* frames[1];
            backtrace(frames, 1);
#endif // DJV_CRASH_LOG_BACKTRACE

            // A stack of its own, so a stack overflow on the main thread
            // can still be written about.
            altStack.resize(64 * 1024);
            stack_t stack;
            stack.ss_sp = altStack.data();
            stack.ss_size = altStack.size();
            stack.ss_flags = 0;
            sigaltstack(&stack, &oldAltStack);

            struct sigaction action;
            std::memset(&action, 0, sizeof(action));
            action.sa_sigaction = signalHandler;
            action.sa_flags = SA_SIGINFO | SA_ONSTACK;
            sigemptyset(&action.sa_mask);
            for (size_t i = 0; i < signalCount; ++i)
            {
                sigaction(signals[i], &action, &oldActions[i]);
            }
#endif // _WINDOWS
            oldTerminate = std::set_terminate(terminateHandler);
            _installed = true;
#endif // DJV_CRASH_LOG_SANITIZER
        }

        CrashLog::~CrashLog()
        {
            if (!_installed)
                return;
            writeLine(_error.empty() ? "Clean exit" : ("Exit on an error: " + _error));
            std::set_terminate(oldTerminate);
#if defined(_WINDOWS)
            SetUnhandledExceptionFilter(oldFilter);
            signal(SIGABRT, oldAbort);
            logPath.clear();
#else // _WINDOWS
            for (size_t i = 0; i < signalCount; ++i)
            {
                sigaction(signals[i], &oldActions[i], nullptr);
            }
            sigaltstack(&oldAltStack, nullptr);
            ::close(logFd);
            logFd = -1;
#endif // _WINDOWS
        }

        bool CrashLog::isInstalled() const
        {
            return _installed;
        }

        void CrashLog::setError(const std::string& value)
        {
            _error = value;
        }
    }
}
