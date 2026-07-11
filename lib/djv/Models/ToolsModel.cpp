// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/ToolsModel.h>

#include <ftk/UI/Settings.h>
#include <ftk/Core/Error.h>
#include <ftk/Core/String.h>

#include <array>
#include <iostream>
#include <sstream>

namespace djv
{
    namespace models
    {
        struct ToolsModel::Private
        {
            std::shared_ptr<ftk::Settings> settings;
            std::vector<ToolInfo> tools;
            std::shared_ptr<ftk::Observable<std::string> > activeTool;
        };

        void ToolsModel::_init(const std::shared_ptr<ftk::Settings>& settings)
        {
            FTK_P();

            p.settings = settings;

            p.tools.push_back({ "Files", "Files", "A", true });
            p.tools.push_back({ "Export", "Export", "B", true });
            p.tools.push_back({ "View", "View", "C", true });
            p.tools.push_back({ "Color", "ColorControls", "D", true });
            p.tools.push_back({ "Color Picker", "ColorPicker", "E", true });
            p.tools.push_back({ "Magnify", "Magnify", "F", true });
            p.tools.push_back({ "Information", "Info", "G", true });
            p.tools.push_back({ "Audio", "Audio", "H", true });
            p.tools.push_back({ "Settings", "Settings", "W", true });
            p.tools.push_back({ "Messages", "Messages", "X" });
            p.tools.push_back({ "System Log", std::string(), "Y" });
            p.tools.push_back({ "Diagnostics", std::string(), "Z" });

            std::string s;
            p.settings->get("/Tools/Tool.2", s);
            p.activeTool = ftk::Observable<std::string>::create(s);
        }

        ToolsModel::ToolsModel() :
            _p(new Private)
        {}

        ToolsModel::~ToolsModel()
        {
            FTK_P();
            p.settings->set("/Tools/Tool.2", p.activeTool->get());
        }

        std::shared_ptr<ToolsModel> ToolsModel::create(const std::shared_ptr<ftk::Settings>& settings)
        {
            auto out = std::shared_ptr<ToolsModel>(new ToolsModel);
            out->_init(settings);
            return out;
        }

        const std::vector<ToolInfo>& ToolsModel::getTools() const
        {
            return _p->tools;
        }
        
        void ToolsModel::addTool(const ToolInfo& value)
        {
            _p->tools.push_back(value);
        }

        const std::string& ToolsModel::getActiveTool() const
        {
            return _p->activeTool->get();
        }

        std::shared_ptr<ftk::Observable<std::string> > ToolsModel::observeActiveTool() const
        {
            return _p->activeTool;
        }

        void ToolsModel::setActiveTool(const std::string& value)
        {
            _p->activeTool->setIfChanged(value);
        }
    }
}
