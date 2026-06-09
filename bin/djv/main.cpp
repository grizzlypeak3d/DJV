// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/App.h>

#include <tlRender/UI/Init.h>
#include <tlRender/Device/Init.h>

#include <ftk/Core/Context.h>
#include <ftk/Core/Error.h>

#include <iostream>

#if defined(_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#endif // _WINDOWS

FTK_MAIN()
{
    try
    {
        auto context = ftk::Context::create();
        tl::ui::init(context);
        tl::device::init(context);
        auto args = ftk::convert(argc, argv);
        auto app = djv::app::App::create(context, args);
        if (app->hasCmdLineHelp())
            return 0;
        if (app->hasPrintVersion())
        {
            std::cout << DJV_VERSION_FULL << std::endl;
            return 0;
        }
        app->run();
    }
    catch(const std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

#if defined(_WINDOWS)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    return wmain(__argc, __wargv);
}
#endif // _WINDOWS
