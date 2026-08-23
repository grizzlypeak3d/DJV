// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/App/IActions.h>
#include <djv/Models/Export.h>

namespace djv
{
    namespace app
    {
        class MainWindow;

        //! Window actions.
        class DJV_APP_API_TYPE WindowActions : public IActions
        {
            FTK_NON_COPYABLE(WindowActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&);

            WindowActions();

        public:
            DJV_APP_API ~WindowActions();

            DJV_APP_API static std::shared_ptr<WindowActions> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&);

        private:
            FTK_PRIVATE();
        };
    }
}
