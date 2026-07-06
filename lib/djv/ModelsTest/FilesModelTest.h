// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/TestLib/ITest.h>

namespace djv
{
    namespace models_tests
    {
        class FilesModelTest : public ftk::test::ITest
        {
        protected:
            FilesModelTest(const std::shared_ptr<ftk::Context>&);

        public:
            static std::shared_ptr<FilesModelTest> create(
                const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _files();
            void _navigation();
            void _compare();
            void _persistence();
        };
    }
}
