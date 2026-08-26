// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/TestLib/ITest.h>

namespace djv
{
    namespace models_tests
    {
        class PlaylistTest : public ftk::test::ITest
        {
        protected:
            PlaylistTest(const std::shared_ptr<ftk::Context>&);

        public:
            static std::shared_ptr<PlaylistTest> create(
                const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _roundTrip();
            void _foreign();
            void _version();
        };
    }
}
