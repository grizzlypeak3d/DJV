// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/TimeUnitsModelTest.h>

#include <tlRender/UI/Init.h>

#include <ftk/Core/Context.h>

#include <iostream>
#include <vector>

using namespace djv;

int main(int argc, char* argv[])
{
    try
    {
        auto context = ftk::Context::create();

        auto logObserver = ftk::ListObserver<ftk::LogItem>::create(
            context->getSystem<ftk::LogSystem>()->observeLogItems(),
            [](const std::vector<ftk::LogItem>& value)
            {
                for (const auto& i : value)
                {
                    std::cout << "[LOG] " << ftk::getLabel(i) << std::endl;
                }
            },
            ftk::ObserverAction::Suppress);

        context->tick();

        std::vector<std::shared_ptr<ftk::test::ITest> > tests;
        tests.push_back(models_tests::TimeUnitsModelTest::create(context));

        for (const auto& test : tests)
        {
            std::cout << "Running test: " << test->getName() << std::endl;
            test->run();
            context->tick();
        }

        std::cout << "Finished tests" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
