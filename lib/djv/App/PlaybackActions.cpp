// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/PlaybackActions.h>

#include <djv/App/App.h>

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

            // Register the commands.
            _addCommand(
                "Stop",
                "Stop playback.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->stop();
                    }
                });

            _addCommand(
                "Forward",
                "Start forward playback.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->forward();
                    }
                });

            _addCommand(
                "Reverse",
                "Start reverse playback.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->reverse();
                    }
                });

            _addCommand(
                "Toggle",
                "Toggle playback.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->togglePlayback();
                    }
                });

            _addCommand(
                "JumpBack1s",
                "Jump back 1 second.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->timeAction(tl::TimeAction::JumpBack1s);
                    }
                });

            _addCommand(
                "JumpBack10s",
                "Jump back 10 seconds.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->timeAction(tl::TimeAction::JumpBack10s);
                    }
                });

            _addCommand(
                "JumpForward1s",
                "Jump forward 1 second.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->timeAction(tl::TimeAction::JumpForward1s);
                    }
                });

            _addCommand(
                "JumpForward10s",
                "Jump forward 10 seconds.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->timeAction(tl::TimeAction::JumpForward10s);
                    }
                });

            _addCommand(
                "Loop",
                "Loop playback continuously.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->setLoop(tl::Loop::Loop);
                    }
                });

            _addCommand(
                "Once",
                "Playback once and stop.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->setLoop(tl::Loop::Once);
                    }
                });

            _addCommand(
                "PingPong",
                "Playback forward and reverse continuously.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->setLoop(tl::Loop::PingPong);
                    }
                });

            _addCommand(
                "SetInPoint",
                "Set the playback in point to the current frame.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->setInPoint();
                    }
                });

            _addCommand(
                "ResetInPoint",
                "Reset the playback in point.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->resetInPoint();
                    }
                });

            _addCommand(
                "SetOutPoint",
                "Set the playback out point to the current frame.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->setOutPoint();
                    }
                });

            _addCommand(
                "ResetOutPoint",
                "Reset the playback out point.",
                [this](const nlohmann::json&)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->resetOutPoint();
                    }
                });

            // Commands without menu actions, for scripting and automation.
            _addCommand(
                "Seek",
                "Seek to a frame, relative to the timeline start; "
                "e.g., { \"frame\": 100 }.",
                [this](const nlohmann::json& args)
                {
                    FTK_P();
                    if (p.player)
                    {
                        const auto start = p.player->getTimeRange().start_time();
                        p.player->seek(OTIO_NS::RationalTime(
                            start.value() + args.at("frame").get<double>(),
                            start.rate()));
                    }
                });

            _addCommand(
                "InOutRange",
                "Set the playback in/out range from inclusive frames relative "
                "to the timeline start; e.g., { \"in\": 10, \"out\": 50 }.",
                [this](const nlohmann::json& args)
                {
                    FTK_P();
                    if (p.player)
                    {
                        const auto start = p.player->getTimeRange().start_time();
                        const double rate = start.rate();
                        p.player->setInOutRange(
                            OTIO_NS::TimeRange::range_from_start_end_time_inclusive(
                                OTIO_NS::RationalTime(
                                    start.value() + args.at("in").get<double>(),
                                    rate),
                                OTIO_NS::RationalTime(
                                    start.value() + args.at("out").get<double>(),
                                    rate)));
                    }
                });

            // Create the actions.
            _actions["Stop"] = ftk::Action::create(
                "Stop",
                "PlaybackStop",
                _command("Stop"));
            _actions["Forward"] = ftk::Action::create(
                "Forward",
                "PlaybackForward",
                _command("Forward"));
            _actions["Reverse"] = ftk::Action::create(
                "Reverse",
                "PlaybackReverse",
                _command("Reverse"));
            _actions["Toggle"] = ftk::Action::create(
                "Toggle Playback",
                _command("Toggle"));
            _actions["JumpBack1s"] = ftk::Action::create(
                "Jump Back 1s",
                _command("JumpBack1s"));
            _actions["JumpBack10s"] = ftk::Action::create(
                "Jump Back 10s",
                _command("JumpBack10s"));
            _actions["JumpForward1s"] = ftk::Action::create(
                "Jump Forward 1s",
                _command("JumpForward1s"));
            _actions["JumpForward10s"] = ftk::Action::create(
                "Jump Forward 10s",
                _command("JumpForward10s"));
            _actions["Loop"] = ftk::Action::create(
                "Playback Loop",
                "PlaybackLoop",
                _command("Loop"));
            _actions["Once"] = ftk::Action::create(
                "Playback Once",
                "PlaybackOnce",
                _command("Once"));
            _actions["PingPong"] = ftk::Action::create(
                "Playback Ping-Pong",
                "PlaybackPingPong",
                _command("PingPong"));
            _actions["SetInPoint"] = ftk::Action::create(
                "Set In Point",
                _command("SetInPoint"));
            _actions["ResetInPoint"] = ftk::Action::create(
                "Reset In Point",
                _command("ResetInPoint"));
            _actions["SetOutPoint"] = ftk::Action::create(
                "Set Out Point",
                _command("SetOutPoint"));
            _actions["ResetOutPoint"] = ftk::Action::create(
                "Reset Out Point",
                _command("ResetOutPoint"));

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
