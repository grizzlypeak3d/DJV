// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

// Ends the way it is told to, with the crash log installed, for
// check.cmake to read the log after. A handler of the test's own is
// installed first and exits with a code of its own: the crash log has to
// hand the crash on to it, and the system's crash reporter is never
// bothered by a test.

#include <djv/App/CrashLog.h>

#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#if defined(_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#else // _WINDOWS
#include <csignal>
#include <unistd.h>
#endif // _WINDOWS

namespace
{
    // What the earlier handlers exit with.
    const int crashExit = 42;
    const int terminateExit = 43;

#if defined(_WINDOWS)
    LONG WINAPI earlierFilter(EXCEPTION_POINTERS*)
    {
        ExitProcess(crashExit);
        return EXCEPTION_EXECUTE_HANDLER;
    }
#else // _WINDOWS
    void earlierHandler(int)
    {
        _exit(crashExit);
    }
#endif // _WINDOWS

    void earlierTerminate()
    {
        std::_Exit(terminateExit);
    }

    // Reads through a pointer the compiler cannot see is null.
    volatile int* volatile nowhere = nullptr;
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: djv-crash-test clean|error|crash|terminate LOG" << std::endl;
        return 2;
    }
    const std::string how = argv[1];
    const std::string log = argv[2];
    {
        std::ofstream f(log);
        f << "The log before\n";
    }

#if defined(_WINDOWS)
    // Only for the crash: on Windows an exception nothing caught goes to
    // the C++ runtime's filter, which is what calls std::terminate, and a
    // filter of the test's own in its place would take that too.
    if ("crash" == how)
    {
        SetUnhandledExceptionFilter(earlierFilter);
    }
#else // _WINDOWS
    std::signal(SIGSEGV, earlierHandler);
    std::signal(SIGBUS, earlierHandler);
#endif // _WINDOWS
    std::set_terminate(earlierTerminate);

    djv::app::CrashLog crashLog(log);
    if (!crashLog.isInstalled())
    {
        // A sanitizer build: nothing to test.
        std::cout << "Not installed" << std::endl;
        return 0;
    }
    if ("clean" == how)
    {
        return 0;
    }
    else if ("error" == how)
    {
        crashLog.setError("Cannot open the file");
        return 1;
    }
    else if ("crash" == how)
    {
        return *nowhere;
    }
    else if ("terminate" == how)
    {
        throw std::runtime_error("Nothing caught this");
    }
    return 2;
}
