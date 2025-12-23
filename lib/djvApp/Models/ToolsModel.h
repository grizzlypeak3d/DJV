// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/Event.h>
#include <ftk/Core/Observable.h>

namespace ftk
{
    class Settings;
}

namespace djv
{
    namespace app
    {
        //! Tools.
        enum class Tool
        {
            None,
            Files,
            Export,
            View,
            Color,
            ColorPicker,
            Magnify,
            Info,
            Audio,
            Devices,
            Settings,
            Messages,
            SystemLog,

            Count,
            First = None
        };
        FTK_ENUM(Tool);

        //! Get the tool icon.
        std::string getIcon(Tool);

        //! Get the tools shown in the toolbar.
        std::vector<Tool> getToolsInToolbar();

        //! Tools model.
        class ToolsModel : public std::enable_shared_from_this<ToolsModel>
        {
            FTK_NON_COPYABLE(ToolsModel);

        protected:
            void _init(const std::shared_ptr<ftk::Settings>&);

            ToolsModel();

        public:
            ~ToolsModel();

            //! Create a new model.
            static std::shared_ptr<ToolsModel> create(
                const std::shared_ptr<ftk::Settings>&);

            //! Get the active tool.
            Tool getActiveTool() const;

            //! Observe the active tool.
            std::shared_ptr<ftk::Observable<Tool> > observeActiveTool() const;

            //! Set the active tool.
            void setActiveTool(Tool);

        private:
            FTK_PRIVATE();
        };
    }
}
