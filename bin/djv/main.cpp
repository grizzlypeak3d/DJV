// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/App.h>

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
    int r = 1;
    try
    {
        auto context = ftk::Context::create();
        tl::ui::init(context);
        tl::device::init(context);
        auto args = ftk::convert(argc, argv);
        auto app = djv::app::App::create(context, args);
        r = app->getExit();
        if (0 == r)
        {
            if (app->hasPrintVersion())
            {
                std::cout << DJV_VERSION_FULL << std::endl;
            }
            else
            {
                app->run();
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
    return r;
}

#if defined(_WINDOWS)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    return wmain(__argc, __wargv);
}
#endif // _WINDOWS
