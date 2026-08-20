// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

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
        struct DJV_API_TYPE ToolInfo
        {
            std::string      name;
            std::string      icon;
            std::string      sort;
            bool             toolBar = false;
            ftk::KeyShortcut shortcut;
        };

        //! Tools model.
        class DJV_API_TYPE ToolsModel : public std::enable_shared_from_this<ToolsModel>
        {
            FTK_NON_COPYABLE(ToolsModel);

        protected:
            void _init(const std::shared_ptr<ftk::Settings>&);

            ToolsModel();

        public:
            DJV_API ~ToolsModel();

            //! Create a new model.
            DJV_API static std::shared_ptr<ToolsModel> create(
                const std::shared_ptr<ftk::Settings>&);

            //! Get the tools.
            DJV_API const std::vector<ToolInfo>& getTools() const;
            
            //! Add a tool.
            DJV_API void addTool(const ToolInfo&);

            //! Get the open tools, in the order they are listed above rather
            //! than the order they were opened, so that opening one does not
            //! move the others around.
            DJV_API const std::vector<std::string>& getOpenTools() const;

            //! Observe the open tools.
            DJV_API std::shared_ptr<ftk::IObservableList<std::string> > observeOpenTools() const;

            //! Get whether a tool is open.
            DJV_API bool isToolOpen(const std::string&) const;

            //! Open or close a tool.
            DJV_API void setToolOpen(const std::string&, bool);

            //! Close every tool.
            DJV_API void closeTools();

        private:
            // Kept in the order the tools are listed, and anything not in that
            // list dropped: a settings file can name a tool that no longer
            // exists.
            std::vector<std::string> _sorted(const std::vector<std::string>&) const;

            FTK_PRIVATE();
        };
    }
}
