// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include "djv-test.h"

#include <djv/ModelsTest/AudioModelTest.h>
#include <djv/ModelsTest/FilesModelTest.h>
#include <djv/ModelsTest/RecentFilesModelTest.h>
#include <djv/ModelsTest/TimeUnitsModelTest.h>
#include <djv/ModelsTest/ToolsModelTest.h>
#include <djv/ModelsTest/ViewportModelTest.h>

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
            p.tests.push_back(models_tests::FilesModelTest::create(context));
            p.tests.push_back(models_tests::RecentFilesModelTest::create(context));
            p.tests.push_back(models_tests::TimeUnitsModelTest::create(context));
            p.tests.push_back(models_tests::ToolsModelTest::create(context));
            p.tests.push_back(models_tests::ViewportModelTest::create(context));
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

        int App::run()
        {
            FTK_P();

            // Get the tests to run.
            std::vector<std::shared_ptr<ftk::test::ITest> > runTests;
            std::vector<std::string> unmatched;
            const auto& cmdLineTests = p.testNames->getList();
            if (!cmdLineTests.empty())
            {
                // Every test whose name contains the argument, not just the
                // first: a group name is the useful way to ask for part of the
                // suite, and taking one match silently ran less than that.
                for (const auto& test : cmdLineTests)
                {
                    size_t matched = 0;
                    for (const auto& other : p.tests)
                    {
                        if (ftk::contains(other->getName(), test, ftk::CaseCompare::Insensitive))
                        {
                            ++matched;
                            if (std::find(runTests.begin(), runTests.end(), other) ==
                                runTests.end())
                            {
                                runTests.push_back(other);
                            }
                        }
                    }
                    if (0 == matched)
                    {
                        unmatched.push_back(test);
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

            // A filter that matched nothing used to run zero tests and exit
            // successfully, which reads exactly like a suite that passed.
            if (!unmatched.empty())
            {
                for (const auto& name : unmatched)
                {
                    _print(ftk::Format("ERROR: no tests match: {0}").arg(name));
                }
                return 1;
            }

            // Run the tests.
            size_t failureCount = 0;
            for (const auto& test : runTests)
            {
                _context->tick();
                _print(ftk::Format("Running test: {0}").arg(test->getName()));
                test->run();
                failureCount += test->getFailureCount();
            }

            const auto now = std::chrono::steady_clock::now();
            const std::chrono::duration<float> diff = now - p.startTime;
            _print(ftk::Format("Seconds elapsed: {0}").arg(diff.count(), 2));
            _print(ftk::Format("Tests run: {0}").arg(runTests.size()));
            _print(ftk::Format("Failures: {0}").arg(failureCount));

            // The count is printed rather than returned: exit codes are
            // truncated to eight bits, so a run with a multiple of 256
            // failures would report success.
            return failureCount > 0 ? 1 : 0;
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
        return app->run();
    }
    catch (const std::exception& e)
    {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
