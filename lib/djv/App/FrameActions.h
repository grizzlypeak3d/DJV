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

        //! Frame actions.
        class DJV_APP_API_TYPE FrameActions : public IActions
        {
            FTK_NON_COPYABLE(FrameActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&);

            FrameActions();

        public:
            DJV_APP_API ~FrameActions();

            DJV_APP_API static std::shared_ptr<FrameActions> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&);

        private:
            void _markersUpdate();

            FTK_PRIVATE();
        };
    }
}

