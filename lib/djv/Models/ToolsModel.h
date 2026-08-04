// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/Action.h>
#include <ftk/UI/Event.h>
#include <ftk/Core/Observable.h>
#include <ftk/Core/ObservableList.h>

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

            //! Get the open tools, in the order they are listed above rather
            //! than the order they were opened, so that opening one does not
            //! move the others around.
            const std::vector<std::string>& getOpenTools() const;

            //! Observe the open tools.
            std::shared_ptr<ftk::IObservableList<std::string> > observeOpenTools() const;

            //! Get whether a tool is open.
            bool isToolOpen(const std::string&) const;

            //! Open or close a tool.
            void setToolOpen(const std::string&, bool);

            //! Close every tool.
            void closeTools();

        private:
            // Kept in the order the tools are listed, and anything not in that
            // list dropped: a settings file can name a tool that no longer
            // exists.
            std::vector<std::string> _sorted(const std::vector<std::string>&) const;

            FTK_PRIVATE();
        };
    }
}
