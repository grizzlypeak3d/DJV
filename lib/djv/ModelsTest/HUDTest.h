// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/TestLib/ITest.h>

namespace djv
{
    namespace models_tests
    {
        class HUDTest : public ftk::test::ITest
        {
        protected:
            HUDTest(const std::shared_ptr<ftk::Context>&);

        public:
            static std::shared_ptr<HUDTest> create(const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _lines();
        };
    }
}
