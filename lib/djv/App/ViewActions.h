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

        //! View actions.
        //!
        //! \todo Add an action for toggling the UI visibility.
        class DJV_APP_API_TYPE ViewActions : public IActions
        {
            FTK_NON_COPYABLE(ViewActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&);

            ViewActions();

        public:
            DJV_APP_API ~ViewActions();

            DJV_APP_API static std::shared_ptr<ViewActions> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&);

        private:
            FTK_PRIVATE();
        };
    }
}
