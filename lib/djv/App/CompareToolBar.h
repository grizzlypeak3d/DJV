// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <ftk/UI/ToolBar.h>

namespace djv
{
    namespace app
    {
        //! Compare tool bar.
        class DJV_API_TYPE CompareToolBar : public ftk::ToolBar
        {
            FTK_NON_COPYABLE(CompareToolBar);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::map<std::string, std::shared_ptr<ftk::Action> >&,
                const std::shared_ptr<IWidget>& parent);

            CompareToolBar() = default;

        public:
            DJV_API ~CompareToolBar();

            DJV_API static std::shared_ptr<CompareToolBar> create(
                const std::shared_ptr<ftk::Context>&,
                const std::map<std::string, std::shared_ptr<ftk::Action> >&,
                const std::shared_ptr<IWidget>& parent = nullptr);
        };
    }
}
