// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/TestLib/ITest.h>

namespace djv
{
    namespace models_tests
    {
        class ReviewTest : public ftk::test::ITest
        {
        protected:
            ReviewTest(const std::shared_ptr<ftk::Context>&);

        public:
            static std::shared_ptr<ReviewTest> create(
                const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _version();
            void _roundTrip();
            void _unreadableSection();
            void _unknownSpace();
            void _unknownKeys();
        };
    }
}
