// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Actions/PlaybackActions.h>

#include <djvApp/App.h>

#include <tlRender/UI/TimelineWidget.h>

namespace djv
{
    namespace app
    {
        struct PlaybackActions::Private
        {
            std::shared_ptr<tl::timeline::Player> player;

            std::map<tl::timeline::Playback, std::shared_ptr<ftk::Action> > playbackItems;
            std::map<tl::timeline::Loop, std::shared_ptr<ftk::Action> > loopItems;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::timeline::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<tl::timeline::Playback> > playbackObserver;
            std::shared_ptr<ftk::Observer<tl::timeline::Loop> > loopObserver;
        };

        void PlaybackActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            IActions::_init(context, app, "Playback");
            FTK_P();

            auto appWeak = std::weak_ptr<App>(app);
            _actions["Stop"] = ftk::Action::create(
                "Stop",
                "PlaybackStop",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->stop();
                        }
                    }
                });

            _actions["Forward"] = ftk::Action::create(
                "Forward",
                "PlaybackForward",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->forward();
                        }
                    }
                });

            _actions["Reverse"] = ftk::Action::create(
                "Reverse",
                "PlaybackReverse",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->reverse();
                        }
                    }
                });

            _actions["Toggle"] = ftk::Action::create(
                "Toggle Playback",
                [this, appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            const tl::timeline::Playback playback = player->observePlayback()->get();
                            player->setPlayback(
                                tl::timeline::Playback::Stop == playback ?
                                _playbackPrev :
                                tl::timeline::Playback::Stop);
                            if (playback != tl::timeline::Playback::Stop)
                            {
                                _playbackPrev = playback;
                            }
                        }
                    }
                });

            _actions["JumpBack1s"] = ftk::Action::create(
                "Jump Back 1s",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->timeAction(tl::timeline::TimeAction::JumpBack1s);
                        }
                    }
                });

            _actions["JumpBack10s"] = ftk::Action::create(
                "Jump Back 10s",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->timeAction(tl::timeline::TimeAction::JumpBack10s);
                        }
                    }
                });

            _actions["JumpForward1s"] = ftk::Action::create(
                "Jump Forward 1s",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->timeAction(tl::timeline::TimeAction::JumpForward1s);
                        }
                    }
                });

            _actions["JumpForward10s"] = ftk::Action::create(
                "Jump Forward 10s",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->timeAction(tl::timeline::TimeAction::JumpForward10s);
                        }
                    }
                });

            _actions["Loop"] = ftk::Action::create(
                "Loop Playback",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->setLoop(tl::timeline::Loop::Loop);
                        }
                    }
                });

            _actions["Once"] = ftk::Action::create(
                "Playback Once",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->setLoop(tl::timeline::Loop::Once);
                        }
                    }
                });

            _actions["PingPong"] = ftk::Action::create(
                "Ping-Pong Playback",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->setLoop(tl::timeline::Loop::PingPong);
                        }
                    }
                });

            _actions["SetInPoint"] = ftk::Action::create(
                "Set In Point",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->setInPoint();
                        }
                    }
                });

            _actions["ResetInPoint"] = ftk::Action::create(
                "Reset In Point",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->resetInPoint();
                        }
                    }
                });

            _actions["SetOutPoint"] = ftk::Action::create(
                "Set Out Point",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->setOutPoint();
                        }
                    }
                });

            _actions["ResetOutPoint"] = ftk::Action::create(
                "Reset Out Point",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            player->resetOutPoint();
                        }
                    }
                });

            p.playbackItems[tl::timeline::Playback::Stop] = _actions["Stop"];
            p.playbackItems[tl::timeline::Playback::Forward] = _actions["Forward"];
            p.playbackItems[tl::timeline::Playback::Reverse] = _actions["Reverse"];

            p.loopItems[tl::timeline::Loop::Loop] = _actions["Loop"];
            p.loopItems[tl::timeline::Loop::Once] = _actions["Once"];
            p.loopItems[tl::timeline::Loop::PingPong] = _actions["PingPong"];

            _tooltips =
            {
                { "Stop", "Stop playback." },
                { "Forward", "Start forward playback." },
                { "Reverse", "Start reverse playback." },
                { "Toggle", "Toggle playback." },
                { "JumpBack1s", "Jump back 1 second." },
                { "JumpBack10s", "Jump back 10 seconds." },
                { "JumpForward1s", "Jump forward 1 second." },
                { "JumpForward10s", "Jump forward 10 seconds." },
                { "Loop", "Loop playback." },
                { "Once", "Playback once and then stop" },
                { "PingPong", "Ping pong playback." },
                { "SetInPoint", "Set the playback in point." },
                { "ResetInPoint", "Reet the playback in point." },
                { "SetOutPoint", "Set the playback out point." },
                { "ResetOutPoint", "Reet the playback out point." }
            };

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());
            _playbackUpdate();
            _loopUpdate();

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::timeline::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::timeline::Player>& value)
                {
                    _setPlayer(value);
                });
        }

        PlaybackActions::PlaybackActions() :
            _p(new Private)
        {}

        PlaybackActions::~PlaybackActions()
        {}

        std::shared_ptr<PlaybackActions> PlaybackActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            auto out = std::shared_ptr<PlaybackActions>(new PlaybackActions);
            out->_init(context, app);
            return out;
        }

        void PlaybackActions::_setPlayer(const std::shared_ptr<tl::timeline::Player>& value)
        {
            FTK_P();
            p.playbackObserver.reset();
            p.loopObserver.reset();

            p.player = value;

            if (p.player)
            {
                p.playbackObserver = ftk::Observer<tl::timeline::Playback>::create(
                    p.player->observePlayback(),
                    [this](tl::timeline::Playback)
                    {
                        _playbackUpdate();
                    });

                p.loopObserver = ftk::Observer<tl::timeline::Loop>::create(
                    p.player->observeLoop(),
                    [this](tl::timeline::Loop)
                    {
                        _loopUpdate();
                    });
            }

            _actions["Stop"]->setEnabled(p.player.get());
            _actions["Forward"]->setEnabled(p.player.get());
            _actions["Reverse"]->setEnabled(p.player.get());
            _actions["Toggle"]->setEnabled(p.player.get());
            _actions["JumpBack1s"]->setEnabled(p.player.get());
            _actions["JumpBack10s"]->setEnabled(p.player.get());
            _actions["JumpForward1s"]->setEnabled(p.player.get());
            _actions["JumpForward10s"]->setEnabled(p.player.get());
            _actions["Loop"]->setEnabled(p.player.get());
            _actions["Once"]->setEnabled(p.player.get());
            _actions["PingPong"]->setEnabled(p.player.get());
            _actions["SetInPoint"]->setEnabled(p.player.get());
            _actions["ResetInPoint"]->setEnabled(p.player.get());
            _actions["SetOutPoint"]->setEnabled(p.player.get());
            _actions["ResetOutPoint"]->setEnabled(p.player.get());
        }

        void PlaybackActions::_playbackUpdate()
        {
            FTK_P();
            std::map<tl::timeline::Playback, bool> values;
            for (const auto& value : tl::timeline::getPlaybackEnums())
            {
                values[value] = false;
            }
            values[p.player ?
                p.player->observePlayback()->get() :
                tl::timeline::Playback::Stop] = true;
            for (auto i : values)
            {
                p.playbackItems[i.first]->setChecked(i.second);
            }
        }

        void PlaybackActions::_loopUpdate()
        {
            FTK_P();
            std::map<tl::timeline::Loop, bool> values;
            for (const auto& value : tl::timeline::getLoopEnums())
            {
                values[value] = false;
            }
            values[p.player ?
                p.player->observeLoop()->get() :
                tl::timeline::Loop::Loop] = true;
            for (auto i : values)
            {
                p.loopItems[i.first]->setChecked(i.second);
            }
        }
    }
}
