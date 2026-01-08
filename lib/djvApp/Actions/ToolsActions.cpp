// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Actions/ToolsActions.h>

#include <djvApp/Models/ToolsModel.h>
#include <djvApp/App.h>

namespace djv
{
    namespace app
    {
        struct ToolsActions::Private
        {
            std::shared_ptr<ftk::Observer<Tool> > activeObserver;
        };

        void ToolsActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            IActions::_init(context, app, "Tools");
            FTK_P();

            auto appWeak = std::weak_ptr<App>(app);
            const auto enums = getToolEnums();
            const auto labels = getToolLabels();
            for (size_t i = 1; i < enums.size(); ++i)
            {
                const auto tool = enums[i];
                auto action = ftk::Action::create(
                    to_string(tool),
                    getIcon(tool),
                    [appWeak, tool](bool value)
                    {
                        if (auto app = appWeak.lock())
                        {
                            auto toolsModel = app->getToolsModel();
                            const Tool active = toolsModel->getActiveTool();
                            toolsModel->setActiveTool(tool != active ? tool : Tool::None);
                        }
                    });
                _actions[labels[i]] = action;
            }

            _tooltips =
            {
                { "Files", "Toggle the files tool." },
                { "Export", "Toggle the export tool." },
                { "View", "Toggle the view tool." },
                { "Color", "Toggle the color controls tool." },
                { "Color Picker", "Toggle the color picker tool." },
                { "Magnify", "Toggle the magnify tool." },
                { "Information", "Toggle the information tool." },
                { "Audio", "Toggle the audio tool." },
                { "Devices", "Toggle the devices tool." },
                { "Settings", "Toggle the settings." },
                { "Messages", "Toggle the messages." },
                { "System Log", "Toggle the system log." }
            };

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.activeObserver = ftk::Observer<Tool>::create(
                app->getToolsModel()->observeActiveTool(),
                [this](Tool value)
                {
                    const auto enums = getToolEnums();
                    const auto labels = getToolLabels();
                    for (size_t i = 1; i < enums.size(); ++i)
                    {
                        _actions[labels[i]]->setChecked(enums[i] == value);
                    }
                });
        }

        ToolsActions::ToolsActions() :
            _p(new Private)
        {}

        ToolsActions::~ToolsActions()
        {}

        std::shared_ptr<ToolsActions> ToolsActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            auto out = std::shared_ptr<ToolsActions>(new ToolsActions);
            out->_init(context, app);
            return out;
        }
    }
}
