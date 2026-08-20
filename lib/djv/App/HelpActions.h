// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/IActions.h>
#include <djv/Models/Export.h>

namespace djv
{
    namespace app
    {
        class MainWindow;

        //! Help actions.
        class DJV_API_TYPE HelpActions : public IActions
        {
            FTK_NON_COPYABLE(HelpActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&);

            HelpActions();

        public:
            DJV_API ~HelpActions();

            DJV_API static std::shared_ptr<HelpActions> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&);

        private:
            FTK_PRIVATE();
        };
    }
}
