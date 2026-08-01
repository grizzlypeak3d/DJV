// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/ToolsModel.h>

#include <ftk/UI/Settings.h>
#include <ftk/Core/Error.h>
#include <ftk/Core/String.h>

#include <algorithm>
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
            std::shared_ptr<ftk::ObservableList<std::string> > openTools;
        };

        void ToolsModel::_init(const std::shared_ptr<ftk::Settings>& settings)
        {
            FTK_P();

            p.settings = settings;

            p.tools.push_back({ "Files", "Files", "A", true, ftk::Key::F1 });
            p.tools.push_back({ "Export", "Export", "B", true, ftk::Key::F2 });
            p.tools.push_back({ "View", "View", "C", true, ftk::Key::F3 });
            p.tools.push_back({ "Color", "ColorControls", "D", true, ftk::Key::F4 });
            p.tools.push_back({ "Color Picker", "ColorPicker", "E", true, ftk::Key::F5 });
            p.tools.push_back({ "Magnify", "Magnify", "F", true, ftk::Key::F6 });
            p.tools.push_back({ "Information", "Info", "G", true, ftk::Key::F7 });
            p.tools.push_back({ "Audio", "Audio", "H", true, ftk::Key::F8 });
            p.tools.push_back({ "Review", "Review", "I", true, ftk::Key::F9 });
            p.tools.push_back({ "Settings", "Settings", "W", true, ftk::Key::F10 });
            p.tools.push_back({ "Messages", "Messages", "X", false, ftk::Key::F11 });
            p.tools.push_back({ "System Log", std::string(), "Y", false, ftk::Key::F12 });
            p.tools.push_back({ "Diagnostics", std::string(), "Z", false, ftk::KeyShortcut() });

            // More than one tool can be open now, so what was written before
            // says nothing about which; the key moves rather than trying to
            // read the old one as a list of one.
            std::vector<std::string> open;
            p.settings->get("/Tools/Open.1", open);
            p.openTools = ftk::ObservableList<std::string>::create(_sorted(open));
        }

        ToolsModel::ToolsModel() :
            _p(new Private)
        {}

        ToolsModel::~ToolsModel()
        {
            save();
        }

        void ToolsModel::save()
        {
            FTK_P();
            p.settings->set("/Tools/Open.1", p.openTools->get());
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

        const std::vector<std::string>& ToolsModel::getOpenTools() const
        {
            return _p->openTools->get();
        }

        std::shared_ptr<ftk::IObservableList<std::string> > ToolsModel::observeOpenTools() const
        {
            return _p->openTools;
        }

        bool ToolsModel::isToolOpen(const std::string& value) const
        {
            FTK_P();
            const auto& open = p.openTools->get();
            return std::find(open.begin(), open.end(), value) != open.end();
        }

        void ToolsModel::setToolOpen(const std::string& value, bool open)
        {
            FTK_P();
            auto tools = p.openTools->get();
            const auto i = std::find(tools.begin(), tools.end(), value);
            if (open && i == tools.end())
            {
                tools.push_back(value);
            }
            else if (!open && i != tools.end())
            {
                tools.erase(i);
            }
            p.openTools->setIfChanged(_sorted(tools));
        }

        void ToolsModel::closeTools()
        {
            _p->openTools->setIfChanged(std::vector<std::string>());
        }

        std::vector<std::string> ToolsModel::_sorted(
            const std::vector<std::string>& value) const
        {
            FTK_P();
            std::vector<std::string> out;
            for (const auto& tool : p.tools)
            {
                if (std::find(value.begin(), value.end(), tool.name) != value.end())
                {
                    out.push_back(tool.name);
                }
            }
            return out;
        }
    }
}
