// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/ToolBar.h>

namespace djv
{
    namespace app
    {
        //! Tools tool bar.
        class ToolsToolBar : public ftk::ToolBar
        {
            FTK_NON_COPYABLE(ToolsToolBar);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::map<std::string, std::shared_ptr<ftk::Action> >&,
                const std::shared_ptr<IWidget>& parent);

            ToolsToolBar() = default;

        public:
            ~ToolsToolBar();

            static std::shared_ptr<ToolsToolBar> create(
                const std::shared_ptr<ftk::Context>&,
                const std::map<std::string, std::shared_ptr<ftk::Action> >&,
                const std::shared_ptr<IWidget>& parent = nullptr);
        };
    }
}
