// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/FrameActions.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>

namespace djv
{
    namespace app
    {
        struct FrameActions::Private
        {
            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
        };

        void FrameActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow)
        {
            IActions::_init(context, app, "Frame");
            FTK_P();

            // Register the commands.
            auto appWeak = std::weak_ptr<App>(app);
            _addCommand(
                "Start",
                "Go to the start frame.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->gotoStart();
                        }
                    }
                });

            _addCommand(
                "End",
                "Go to the end frame.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->gotoEnd();
                        }
                    }
                });

            _addCommand(
                "Prev",
                "Go to the previous frame.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->framePrev();
                        }
                    }
                });

            _addCommand(
                "PrevX10",
                "Go to the previous frame X10.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->timeAction(tl::TimeAction::FramePrevX10);
                        }
                    }
                });

            _addCommand(
                "PrevX100",
                "Go to the previous frame X100.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->timeAction(tl::TimeAction::FramePrevX100);
                        }
                    }
                });

            _addCommand(
                "Next",
                "Go to the next frame.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->frameNext();
                        }
                    }
                });

            _addCommand(
                "NextX10",
                "Go to the next frame X10.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->timeAction(tl::TimeAction::FrameNextX10);
                        }
                    }
                });

            _addCommand(
                "NextX100",
                "Go to the next frame X100.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->timeAction(tl::TimeAction::FrameNextX100);
                        }
                    }
                });

            auto mainWindowWeak = std::weak_ptr<MainWindow>(mainWindow);
            _addCommand(
                "FocusCurrent",
                "Set the keyboard focus to the current frame editor.",
                [mainWindowWeak](const nlohmann::json&)
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->focusCurrentFrame();
                    }
                });

            // Create the actions.
            _actions["Start"] = ftk::Action::create(
                "Goto Start",
                "FrameStart",
                _command("Start"));
            _actions["End"] = ftk::Action::create(
                "Goto End",
                "FrameEnd",
                _command("End"));
            _actions["Prev"] = ftk::Action::create(
                "Previous Frame",
                "FramePrev",
                _command("Prev"));
            _actions["PrevX10"] = ftk::Action::create(
                "Previous Frame X10",
                _command("PrevX10"));
            _actions["PrevX100"] = ftk::Action::create(
                "Previous Frame X100",
                _command("PrevX100"));
            _actions["Next"] = ftk::Action::create(
                "Next Frame",
                "FrameNext",
                _command("Next"));
            _actions["NextX10"] = ftk::Action::create(
                "Next Frame X10",
                _command("NextX10"));
            _actions["NextX100"] = ftk::Action::create(
                "Next Frame X100",
                _command("NextX100"));
            _actions["FocusCurrent"] = ftk::Action::create(
                "Focus Current Frame",
                _command("FocusCurrent"));

            // Register the shortcuts.
            _addShortcut("Start", "Start", ftk::Key::Home);
            _addShortcut("End", "End", ftk::Key::End);
            _addShortcut("Prev", "Previous", ftk::Key::Left);
            _addShortcut("PrevX10", "Previous X10", ftk::KeyShortcut(ftk::Key::Left, static_cast<int>(ftk::KeyModifier::Shift)));
            _addShortcut("PrevX100", "Previous X100", ftk::KeyShortcut(ftk::Key::Left, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("Next", "Next", ftk::Key::Right);
            _addShortcut("NextX10", "Next X10", ftk::KeyShortcut(ftk::Key::Right, static_cast<int>(ftk::KeyModifier::Shift)));
            _addShortcut("NextX100", "Next X100", ftk::KeyShortcut(ftk::Key::Right, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("FocusCurrent", "Focus current", ftk::KeyShortcut(ftk::Key::F, static_cast<int>(ftk::KeyModifier::Control)));

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    _actions["Start"]->setEnabled(value.get());
                    _actions["End"]->setEnabled(value.get());
                    _actions["Prev"]->setEnabled(value.get());
                    _actions["PrevX10"]->setEnabled(value.get());
                    _actions["PrevX100"]->setEnabled(value.get());
                    _actions["Next"]->setEnabled(value.get());
                    _actions["NextX10"]->setEnabled(value.get());
                    _actions["NextX100"]->setEnabled(value.get());
                    _actions["FocusCurrent"]->setEnabled(value.get());
                });
        }

        FrameActions::FrameActions() :
            _p(new Private)
        {}

        FrameActions::~FrameActions()
        {}

        std::shared_ptr<FrameActions> FrameActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow)
        {
            auto out = std::shared_ptr<FrameActions>(new FrameActions);
            out->_init(context, app, mainWindow);
            return out;
        }
    }
}
