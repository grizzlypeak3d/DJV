// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/Models/Export.h>

#include <ftk/UI/IContainer.h>
#include <ftk/UI/ToolBar.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Tab bar.
        class DJV_APP_API_TYPE TabBar : public ftk::IContainer
        {
            FTK_NON_COPYABLE(TabBar);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            TabBar();

        public:
            DJV_APP_API ~TabBar();

            DJV_APP_API static std::shared_ptr<TabBar> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);
        private:
            FTK_PRIVATE();
        };
    }
}
