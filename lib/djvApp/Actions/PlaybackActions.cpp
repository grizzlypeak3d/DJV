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
            std::shared_ptr<tl::Player> player;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<tl::Playback> > playbackObserver;
            std::shared_ptr<ftk::Observer<tl::Loop> > loopObserver;
        };

        void PlaybackActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            IActions::_init(context, app, "Playback");
            FTK_P();

            _actions["Stop"] = ftk::Action::create(
                "Stop",
                "PlaybackStop",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->stop();
                    }
                });

            _actions["Forward"] = ftk::Action::create(
                "Forward",
                "PlaybackForward",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->forward();
                    }
                });

            _actions["Reverse"] = ftk::Action::create(
                "Reverse",
                "PlaybackReverse",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->reverse();
                    }
                });

            _actions["Toggle"] = ftk::Action::create(
                "Toggle Playback",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->togglePlayback();
                    }
                });

            _actions["JumpBack1s"] = ftk::Action::create(
                "Jump Back 1s",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->timeAction(tl::TimeAction::JumpBack1s);
                    }
                });

            _actions["JumpBack10s"] = ftk::Action::create(
                "Jump Back 10s",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->timeAction(tl::TimeAction::JumpBack10s);
                    }
                });

            _actions["JumpForward1s"] = ftk::Action::create(
                "Jump Forward 1s",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->timeAction(tl::TimeAction::JumpForward1s);
                    }
                });

            _actions["JumpForward10s"] = ftk::Action::create(
                "Jump Forward 10s",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->timeAction(tl::TimeAction::JumpForward10s);
                    }
                });

            _actions["Loop"] = ftk::Action::create(
                "Loop Playback",
                "PlaybackLoop",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->setLoop(tl::Loop::Loop);
                    }
                });

            _actions["Once"] = ftk::Action::create(
                "Playback Once",
                "PlaybackOnce",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->setLoop(tl::Loop::Once);
                    }
                });

            _actions["PingPong"] = ftk::Action::create(
                "Ping-Pong Playback",
                "PlaybackPingPong",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->setLoop(tl::Loop::PingPong);
                    }
                });

            _actions["SetInPoint"] = ftk::Action::create(
                "Set In Point",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->setInPoint();
                    }
                });

            _actions["ResetInPoint"] = ftk::Action::create(
                "Reset In Point",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->resetInPoint();
                    }
                });

            _actions["SetOutPoint"] = ftk::Action::create(
                "Set Out Point",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->setOutPoint();
                    }
                });

            _actions["ResetOutPoint"] = ftk::Action::create(
                "Reset Out Point",
                [this]
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->resetOutPoint();
                    }
                });

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
            _playbackUpdate(tl::Playback::Stop);
            _loopUpdate(tl::Loop::Loop);

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
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

        void PlaybackActions::_setPlayer(const std::shared_ptr<tl::Player>& value)
        {
            FTK_P();

            p.playbackObserver.reset();
            p.loopObserver.reset();

            p.player = value;

            if (p.player)
            {
                p.playbackObserver = ftk::Observer<tl::Playback>::create(
                    p.player->observePlayback(),
                    [this](tl::Playback value)
                    {
                        _playbackUpdate(value);
                    });

                p.loopObserver = ftk::Observer<tl::Loop>::create(
                    p.player->observeLoop(),
                    [this](tl::Loop value)
                    {
                        _loopUpdate(value);
                    });
            }
            else
            {
                _playbackUpdate(tl::Playback::Stop);
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

        void PlaybackActions::_playbackUpdate(tl::Playback value)
        {
            FTK_P();
            _actions["Stop"]->setChecked(tl::Playback::Stop == value);
            _actions["Forward"]->setChecked(tl::Playback::Forward == value);
            _actions["Reverse"]->setChecked(tl::Playback::Reverse == value);
        }

        void PlaybackActions::_loopUpdate(tl::Loop value)
        {
            FTK_P();
            _actions["Loop"]->setChecked(tl::Loop::Loop == value);
            _actions["Once"]->setChecked(tl::Loop::Once == value);
            _actions["PingPong"]->setChecked(tl::Loop::PingPong == value);
        }
    }
}
