// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Widgets/FileToolBar.h>

namespace djv
{
    namespace app
    {
        void FileToolBar::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::map<std::string, std::shared_ptr<ftk::Action> >& actions,
            const std::shared_ptr<IWidget>& parent)
        {
            ToolBar::_init(context, ftk::Orientation::Horizontal, parent);
            auto tmp = actions;
            addAction(tmp["Open"]);
            addAction(tmp["OpenAudio"]);
            addAction(tmp["Close"]);
            addAction(tmp["CloseAll"]);
            addAction(tmp["Reload"]);
        }

        FileToolBar::~FileToolBar()
        {}

        std::shared_ptr<FileToolBar> FileToolBar::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::map<std::string, std::shared_ptr<ftk::Action> >& actions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<FileToolBar>(new FileToolBar);
            out->_init(context, actions, parent);
            return out;
        }
    }
}
