// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/TestLib/ITest.h>

namespace djv
{
    namespace models_tests
    {
        class ToolsModelTest : public ftk::test::ITest
        {
        protected:
            ToolsModelTest(const std::shared_ptr<ftk::Context>&);

        public:
            static std::shared_ptr<ToolsModelTest> create(
                const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _behavior();
            void _persistence();
        };
    }
}
