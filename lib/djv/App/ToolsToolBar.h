// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/Models/Export.h>

#include <ftk/UI/ToolBar.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Tools tool bar.
        class DJV_APP_API_TYPE ToolsToolBar : public ftk::ToolBar
        {
            FTK_NON_COPYABLE(ToolsToolBar);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::map<std::string, std::shared_ptr<ftk::Action> >&,
                const std::shared_ptr<IWidget>& parent);

            ToolsToolBar() = default;

        public:
            DJV_APP_API ~ToolsToolBar();

            DJV_APP_API static std::shared_ptr<ToolsToolBar> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::map<std::string, std::shared_ptr<ftk::Action> >&,
                const std::shared_ptr<IWidget>& parent = nullptr);
        };
    }
}
