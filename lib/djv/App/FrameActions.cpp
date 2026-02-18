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

            auto appWeak = std::weak_ptr<App>(app);
            _actions["Start"] = ftk::Action::create(
                "Goto Start",
                "FrameStart",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->gotoStart();
                        }
                    }
                });

            _actions["End"] = ftk::Action::create(
                "Goto End",
                "FrameEnd",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->gotoEnd();
                        }
                    }
                });

            _actions["Prev"] = ftk::Action::create(
                "Previous Frame",
                "FramePrev",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->framePrev();
                        }
                    }
                });

            _actions["PrevX10"] = ftk::Action::create(
                "Previous Frame X10",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->timeAction(tl::TimeAction::FramePrevX10);
                        }
                    }
                });

            _actions["PrevX100"] = ftk::Action::create(
                "Previous Frame X100",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->timeAction(tl::TimeAction::FramePrevX100);
                        }
                    }
                });

            _actions["Next"] = ftk::Action::create(
                "Next Frame",
                "FrameNext",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->frameNext();
                        }
                    }
                });

            _actions["NextX10"] = ftk::Action::create(
                "Next Frame X10",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->timeAction(tl::TimeAction::FrameNextX10);
                        }
                    }
                });

            _actions["NextX100"] = ftk::Action::create(
                "Next Frame X100",
                [appWeak]
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
            _actions["FocusCurrent"] = ftk::Action::create(
                "Focus Current Frame",
                [mainWindowWeak]
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->focusCurrentFrame();
                    }
                });

            _tooltips =
            {
                { "Start", "Go to the start frame." },
                { "End", "Go to the end frame." },
                { "Prev", "Go to the previous frame." },
                { "PrevX10", "Go to the previous frame X10." },
                { "PrevX100", "Go to the previous frame X100." },
                { "Next", "Go to the next frame." },
                { "NextX10", "Go to the next frame X10." },
                { "NextX100", "Go to the next frame X100." },
                { "FocusCurrent", "Set the keyboard focus to the current frame editor." }
            };

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
