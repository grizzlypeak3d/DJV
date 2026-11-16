// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/TestLib/ITest.h>

namespace djv
{
    namespace models_tests
    {
        class ExportQueueTest : public ftk::test::ITest
        {
        protected:
            ExportQueueTest(const std::shared_ptr<ftk::Context>&);

        public:
            static std::shared_ptr<ExportQueueTest> create(const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _labels();
            void _queue();
        };
    }
}
