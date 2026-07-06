// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/Core/IApp.h>

namespace djv
{
    namespace tests
    {
        //! Test application.
        class App : public ftk::IApp
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                std::vector<std::string>& argv);

            App();

        public:
            virtual ~App();

            static std::shared_ptr<App> create(
                const std::shared_ptr<ftk::Context>&,
                std::vector<std::string>&);

            void run();

        private:
            FTK_PRIVATE();
        };
    }
}
