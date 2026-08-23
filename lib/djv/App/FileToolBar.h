// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/Models/Export.h>
#include <djv/Models/FilesModel.h>

#include <ftk/UI/ToolBar.h>

namespace djv
{
    namespace app
    {
        //! File tool bar.
        class DJV_APP_API_TYPE FileToolBar : public ftk::ToolBar
        {
            FTK_NON_COPYABLE(FileToolBar);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::map<std::string, std::shared_ptr<ftk::Action> >&,
                const std::shared_ptr<IWidget>& parent);

            FileToolBar() = default;

        public:
            DJV_APP_API ~FileToolBar();

            DJV_APP_API static std::shared_ptr<FileToolBar> create(
                const std::shared_ptr<ftk::Context>&,
                const std::map<std::string, std::shared_ptr<ftk::Action> >&,
                const std::shared_ptr<IWidget>& parent = nullptr);
        };
    }
}
