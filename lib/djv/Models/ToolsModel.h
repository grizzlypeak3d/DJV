// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/Action.h>
#include <ftk/UI/Event.h>
#include <ftk/Core/Observable.h>

namespace ftk
{
    class Settings;
}

namespace djv
{
    namespace models
    {
        //! Tool information.
        struct ToolInfo
        {
            std::string      name;
            std::string      icon;
            std::string      sort;
            bool             toolBar = false;
            ftk::KeyShortcut shortcut;
        };

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

            //! Get the tools.
            const std::vector<ToolInfo>& getTools() const;
            
            //! Add a tool.
            void addTool(const ToolInfo&);

            //! Get the active tool.
            const std::string& getActiveTool() const;

            //! Observe the active tool.
            std::shared_ptr<ftk::Observable<std::string> > observeActiveTool() const;

            //! Set the active tool.
            void setActiveTool(const std::string&);

        private:
            FTK_PRIVATE();
        };
    }
}
