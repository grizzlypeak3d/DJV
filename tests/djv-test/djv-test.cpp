// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include "djv-test.h"

#include <djv/ModelsTest/AudioModelTest.h>
#include <djv/ModelsTest/TimeUnitsModelTest.h>

#include <tlRender/Timeline/Init.h>

#include <ftk/Core/CmdLine.h>
#include <ftk/Core/Context.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>
#include <ftk/Core/Time.h>

#include <algorithm>
#include <iostream>

using namespace djv;

namespace djv
{
    namespace tests
    {
        struct App::Private
        {
            std::shared_ptr<ftk::CmdLineListArg<std::string> > testNames;
            std::vector<std::shared_ptr<ftk::test::ITest> > tests;
            std::chrono::steady_clock::time_point startTime;
        };

        void App::_init(
            const std::shared_ptr<ftk::Context>& context,
            std::vector<std::string>& argv)
        {
            FTK_P();
            p.testNames = ftk::CmdLineListArg<std::string>::create(
                "Test",
                "Names of the tests to run.",
                true);
            IApp::_init(
                context,
                argv,
                "djv-test",
                "Test application",
                { p.testNames });
            p.startTime = std::chrono::steady_clock::now();
            tl::init(context);

            // Models tests.
            p.tests.push_back(models_tests::AudioModelTest::create(context));
            p.tests.push_back(models_tests::TimeUnitsModelTest::create(context));
        }

        App::App() :
            _p(new Private)
        {}

        App::~App()
        {}

        std::shared_ptr<App> App::create(
            const std::shared_ptr<ftk::Context>& context,
            std::vector<std::string>& argv)
        {
            auto out = std::shared_ptr<App>(new App);
            out->_init(context, argv);
            return out;
        }

        void App::run()
        {
            FTK_P();

            // Get the tests to run.
            std::vector<std::shared_ptr<ftk::test::ITest> > runTests;
            const auto& cmdLineTests = p.testNames->getList();
            if (!cmdLineTests.empty())
            {
                for (const auto& test : cmdLineTests)
                {
                    const auto i = std::find_if(
                        p.tests.begin(),
                        p.tests.end(),
                        [test](const std::shared_ptr<ftk::test::ITest>& other)
                        {
                            return ftk::contains(other->getName(), test, ftk::CaseCompare::Insensitive);
                        });
                    if (i != p.tests.end())
                    {
                        runTests.push_back(*i);
                    }
                }
            }
            else
            {
                for (const auto& test : p.tests)
                {
                    runTests.push_back(test);
                }
            }

            // Run the tests.
            for (const auto& test : runTests)
            {
                _context->tick();
                _print(ftk::Format("Running test: {0}").arg(test->getName()));
                test->run();
            }

            const auto now = std::chrono::steady_clock::now();
            const std::chrono::duration<float> diff = now - p.startTime;
            _print(ftk::Format("Seconds elapsed: {0}").arg(diff.count(), 2));
        }
    }
}

FTK_MAIN()
{
    try
    {
        auto context = ftk::Context::create();
        auto args = ftk::convert(argc, argv);
        auto app = djv::tests::App::create(context, args);
        if (app->hasCmdLineHelp())
            return 0;
        app->run();
    }
    catch (const std::exception& e)
    {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
