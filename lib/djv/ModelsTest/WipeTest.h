// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/TestLib/ITest.h>

namespace djv
{
    namespace models_tests
    {
        class WipeTest : public ftk::test::ITest
        {
        protected:
            WipeTest(const std::shared_ptr<ftk::Context>&);

        public:
            static std::shared_ptr<WipeTest> create(
                const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _widget();
            void _handles();
            void _drag();
        };
    }
}
