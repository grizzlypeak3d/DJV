// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/App.h>
#include <djv/App/CrashLog.h>

#include <tlRender/UI/Init.h>

#include <ftk/Core/Context.h>
#include <ftk/Core/Error.h>

#include <iostream>
#include <memory>

#if defined(_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#endif // _WINDOWS

FTK_MAIN()
{
    // Outside the application's scope, so the exit line is written after
    // the application, and the log with it, have finished.
    std::unique_ptr<djv::app::CrashLog> crashLog;
    int out = 0;
    try
    {
        auto context = ftk::Context::create();
        tl::ui::init(context);
        auto args = ftk::convert(argc, argv);
        auto app = djv::app::App::create(context, args);
        if (app->hasCmdLineHelp())
            return 0;
        crashLog = std::make_unique<djv::app::CrashLog>(app->getLogFilePath());
        app->run();
    }
    catch(const std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        if (crashLog)
        {
            crashLog->setError(e.what());
        }
        out = 1;
    }
    return out;
}

#if defined(_WINDOWS)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    return wmain(__argc, __wargv);
}
#endif // _WINDOWS
