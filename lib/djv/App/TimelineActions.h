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

        //! Timeline actions.
        class DJV_APP_API_TYPE TimelineActions : public IActions
        {
            FTK_NON_COPYABLE(TimelineActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&);

            TimelineActions();

        public:
            DJV_APP_API ~TimelineActions();

            DJV_APP_API static std::shared_ptr<TimelineActions> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&);

        private:
            FTK_PRIVATE();
        };
    }
}
