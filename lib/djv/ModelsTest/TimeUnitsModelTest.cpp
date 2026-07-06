// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/TimeUnitsModelTest.h>

#include <djv/Models/TimeUnitsModel.h>

#include <ftk/Core/Assert.h>

#include <sstream>

namespace djv
{
    namespace models_tests
    {
        TimeUnitsModelTest::TimeUnitsModelTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::TimeUnitsModelTest")
        {}

        std::shared_ptr<TimeUnitsModelTest> TimeUnitsModelTest::create(const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<TimeUnitsModelTest>(new TimeUnitsModelTest(context));
        }

        void TimeUnitsModelTest::run()
        {
        }
    }
}
