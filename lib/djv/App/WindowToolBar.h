// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <ftk/UI/ToolBar.h>

namespace djv
{
    namespace app
    {
        //! Window tool bar.
        class DJV_API_TYPE WindowToolBar : public ftk::ToolBar
        {
            FTK_NON_COPYABLE(WindowToolBar);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::map<std::string, std::shared_ptr<ftk::Action> >&,
                const std::shared_ptr<IWidget>& parent);

            WindowToolBar() = default;

        public:
            DJV_API ~WindowToolBar();

            DJV_API static std::shared_ptr<WindowToolBar> create(
                const std::shared_ptr<ftk::Context>&,
                const std::map<std::string, std::shared_ptr<ftk::Action> >&,
                const std::shared_ptr<IWidget>& parent = nullptr);
        };
    }
}
