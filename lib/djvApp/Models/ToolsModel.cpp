// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Models/ToolsModel.h>

#include <ftk/UI/Settings.h>
#include <ftk/Core/Error.h>
#include <ftk/Core/String.h>

#include <array>
#include <iostream>
#include <sstream>

namespace djv
{
    namespace app
    {
        FTK_ENUM_IMPL(
            Tool,
            "None",
            "Files",
            "Export",
            "View",
            "Color",
            "Color Picker",
            "Magnify",
            "Information",
            "Audio",
            "Devices",
            "Settings",
            "Messages",
            "System Log");

        std::string getIcon(Tool value)
        {
            const std::array<std::string, static_cast<size_t>(Tool::Count)> data =
            {
                "",
                "Files",
                "Export",
                "View",
                "ColorControls",
                "ColorPicker",
                "Magnify",
                "Info",
                "Audio",
                "Devices",
                "Settings",
                "Messages",
                ""
            };
            return data[static_cast<size_t>(value)];
        }

        std::vector<Tool> getToolsInToolbar()
        {
            const std::vector<Tool> out
            {
                Tool::Files,
                Tool::Export,
                Tool::View,
                Tool::Color,
                Tool::ColorPicker,
                Tool::Magnify,
                Tool::Info,
                Tool::Audio,
                Tool::Devices,
                Tool::Settings,
                Tool::Messages
            };
            return out;
        }

        struct ToolsModel::Private
        {
            std::shared_ptr<ftk::Settings> settings;

            std::shared_ptr<ftk::Observable<Tool> > activeTool;
        };

        void ToolsModel::_init(const std::shared_ptr<ftk::Settings>& settings)
        {
            FTK_P();

            p.settings = settings;

            std::string s;
            p.settings->get("/Tools/Tool.1", s);
            Tool tool = Tool::None;
            from_string(s, tool);
            p.activeTool = ftk::Observable<Tool>::create(tool);
        }

        ToolsModel::ToolsModel() :
            _p(new Private)
        {}

        ToolsModel::~ToolsModel()
        {
            FTK_P();
            p.settings->set("/Tools/Tool.1", to_string(p.activeTool->get()));
        }

        std::shared_ptr<ToolsModel> ToolsModel::create(const std::shared_ptr<ftk::Settings>& settings)
        {
            auto out = std::shared_ptr<ToolsModel>(new ToolsModel);
            out->_init(settings);
            return out;
        }

        Tool ToolsModel::getActiveTool() const
        {
            return _p->activeTool->get();
        }

        std::shared_ptr<ftk::Observable<Tool> > ToolsModel::observeActiveTool() const
        {
            return _p->activeTool;
        }

        void ToolsModel::setActiveTool(Tool value)
        {
            _p->activeTool->setIfChanged(value);
        }
    }
}
