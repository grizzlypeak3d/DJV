// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ToolsActions.h>

#include <djv/App/App.h>
#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/ToolsModel.h>

#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct ToolsActions::Private
        {
            //! The panel tools, which are mutually exclusive. Undo and redo also
            //! live in this group but are not panels.
            std::vector<std::string> toolNames;

            std::shared_ptr<ftk::Observer<std::string> > activeObserver;
            std::shared_ptr<ftk::Observer<bool> > hasUndoObserver;
            std::shared_ptr<ftk::Observer<bool> > hasRedoObserver;
        };

        void ToolsActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            IActions::_init(context, app, "Tools");
            FTK_P();

            auto appWeak = std::weak_ptr<App>(app);
            for (const auto& tool : app->getToolsModel()->getTools())
            {
                // Register the command.
                _addCheckCommand(
                    tool.name,
                    ftk::Format("Toggle the {0} tool.").arg(tool.name),
                    [appWeak, tool](const nlohmann::json& args)
                    {
                        const bool value = args.at("value").get<bool>();
                        if (auto app = appWeak.lock())
                        {
                            auto toolsModel = app->getToolsModel();
                            if (value)
                            {
                                toolsModel->setActiveTool(tool.name);
                            }
                            else if (toolsModel->getActiveTool() == tool.name)
                            {
                                toolsModel->setActiveTool(std::string());
                            }
                        }
                    });

                // Create the action.
                _actions[tool.name] = ftk::Action::create(
                    tool.name,
                    tool.icon,
                    _checkCommand(tool.name));

                // Register the shortcut.
                _addShortcut(tool.name, tool.name, tool.shortcut);

                p.toolNames.push_back(tool.name);
            }

            // Undo and redo apply to the drawing annotations. A focused text
            // widget handles Ctrl+Z itself and consumes the event, so writing a
            // note is unaffected.
            _addCommand(
                "Undo",
                "Undo the last drawing change.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getAnnotationsModel()->undo();
                    }
                });
            _actions["Undo"] = ftk::Action::create(
                "Undo",
                "Undo",
                _command("Undo"));
            _addShortcut(
                "Undo",
                "Undo",
                ftk::KeyShortcut(ftk::Key::Z, static_cast<int>(ftk::commandKeyModifier)));

            _addCommand(
                "Redo",
                "Redo the last undone drawing change.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getAnnotationsModel()->redo();
                    }
                });
            _actions["Redo"] = ftk::Action::create(
                "Redo",
                "Redo",
                _command("Redo"));
            _addShortcut(
                "Redo",
                "Redo",
                ftk::KeyShortcut(
                    ftk::Key::Z,
                    static_cast<int>(ftk::KeyModifier::Shift) |
                    static_cast<int>(ftk::commandKeyModifier)));

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.hasUndoObserver = ftk::Observer<bool>::create(
                app->getAnnotationsModel()->observeHasUndo(),
                [this](bool value)
                {
                    _actions["Undo"]->setEnabled(value);
                });

            p.hasRedoObserver = ftk::Observer<bool>::create(
                app->getAnnotationsModel()->observeHasRedo(),
                [this](bool value)
                {
                    _actions["Redo"]->setEnabled(value);
                });

            p.activeObserver = ftk::Observer<std::string>::create(
                app->getToolsModel()->observeActiveTool(),
                [this](const std::string& value)
                {
                    FTK_P();
                    // Only the tool panels are mutually exclusive; undo and redo
                    // are not panels and must be left alone.
                    for (const auto& name : p.toolNames)
                    {
                        _actions[name]->setChecked(name == value);
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
