// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Widgets/BottomToolBar.h>

#include <djvApp/Actions/AudioActions.h>
#include <djvApp/Actions/FrameActions.h>
#include <djvApp/Actions/PlaybackActions.h>
#include <djvApp/Models/AudioModel.h>
#include <djvApp/Models/TimeUnitsModel.h>
#include <djvApp/Widgets/AudioPopup.h>
#include <djvApp/Widgets/ShuttleWidget.h>
#include <djvApp/Widgets/SpeedPopup.h>
#include <djvApp/App.h>

#include <tlRender/UI/TimeEdit.h>
#include <tlRender/UI/TimeLabel.h>
#include <tlRender/Timeline/Player.h>

#include <ftk/UI/ComboBox.h>
#include <ftk/UI/DoubleEdit.h>
#include <ftk/UI/DoubleModel.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/Spacer.h>
#include <ftk/UI/ToolButton.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct BottomToolBar::Private
        {
            std::weak_ptr<App> app;
            std::shared_ptr<tl::timeline::Player> player;
            std::shared_ptr<ftk::DoubleModel> speedModel;
            OTIO_NS::RationalTime startTime = tl::time::invalidTime;

            std::map<std::string, std::shared_ptr<ftk::ToolButton> > buttons;
            std::shared_ptr<ShuttleWidget> playbackShuttle;
            std::shared_ptr<ShuttleWidget> frameShuttle;
            std::shared_ptr<tl::ui::TimeEdit> currentTimeEdit;
            std::shared_ptr<tl::ui::TimeLabel> durationLabel;
            std::shared_ptr<ftk::DoubleEdit> speedEdit;
            std::shared_ptr<ftk::ToolButton> speedButton;
            std::shared_ptr<SpeedPopup> speedPopup;
            std::shared_ptr<ftk::Label> speedMultLabel;
            std::shared_ptr<ftk::ComboBox> timeUnitsComboBox;
            std::shared_ptr<ftk::ToolButton> audioButton;
            std::shared_ptr<AudioPopup> audioPopup;
            std::shared_ptr<ftk::ToolButton> muteButton;
            std::shared_ptr<ftk::HorizontalLayout> layout;

            std::shared_ptr<ftk::Observer<tl::timeline::TimeUnits> > timeUnitsObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::timeline::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<double> > speedObserver;
            std::shared_ptr<ftk::Observer<double> > speedMultObserver;
            std::shared_ptr<ftk::Observer<double> > speedObserver2;
            std::shared_ptr<ftk::Observer<OTIO_NS::RationalTime> > currentTimeObserver;
            std::shared_ptr<ftk::Observer<OTIO_NS::TimeRange> > inOutRangeObserver;
        };

        void BottomToolBar::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<PlaybackActions>& playbackActions,
            const std::shared_ptr<FrameActions>& frameActions,
            const std::shared_ptr<AudioActions>& audioActions,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(
                context,
                "djv::app::BottomToolBar",
                parent);
            FTK_P();

            p.app = app;

            p.speedModel = ftk::DoubleModel::create(context);
            p.speedModel->setRange(ftk::RangeD(0.0, 1000000.0));
            p.speedModel->setStep(1.F);
            p.speedModel->setLargeStep(10.F);

            auto actions = playbackActions->getActions();
            p.buttons["Stop"] = ftk::ToolButton::create(context, actions["Stop"]);
            p.buttons["Forward"] = ftk::ToolButton::create(context, actions["Forward"]);
            p.buttons["Reverse"] = ftk::ToolButton::create(context, actions["Reverse"]);

            p.playbackShuttle = ShuttleWidget::create(context, "PlaybackShuttle");
            p.playbackShuttle->setTooltip("Playback shuttle");

            actions = frameActions->getActions();
            p.buttons["Start"] = ftk::ToolButton::create(context, actions["Start"]);
            p.buttons["End"] = ftk::ToolButton::create(context, actions["End"]);
            p.buttons["Prev"] = ftk::ToolButton::create(context, actions["Prev"]);
            p.buttons["Prev"]->setRepeatClick(true);
            p.buttons["Next"] = ftk::ToolButton::create(context, actions["Next"]);
            p.buttons["Next"]->setRepeatClick(true);

            p.frameShuttle = ShuttleWidget::create(context, "FrameShuttle");
            p.frameShuttle->setTooltip("Frame shuttle");

            auto timeUnitsModel = app->getTimeUnitsModel();
            p.currentTimeEdit = tl::ui::TimeEdit::create(context, timeUnitsModel);
            p.currentTimeEdit->setTooltip("Current time");

            p.durationLabel = tl::ui::TimeLabel::create(context, timeUnitsModel);
            p.durationLabel->setFontRole(ftk::FontRole::Mono);
            p.durationLabel->setMarginRole(ftk::SizeRole::MarginInside);
            p.durationLabel->setTooltip("Duration of timeline or in/out range");

            p.speedEdit = ftk::DoubleEdit::create(context, p.speedModel);
            p.speedEdit->setTooltip("Current playback speed");

            p.speedButton = ftk::ToolButton::create(context, "FPS");
            p.speedButton->setIcon("MenuArrow");
            p.speedButton->setTooltip("Playback speed");

            p.speedMultLabel = ftk::Label::create(context);
            p.speedMultLabel->setFontRole(ftk::FontRole::Mono);
            p.speedMultLabel->setHMarginRole(ftk::SizeRole::MarginInside);
            p.speedMultLabel->setTooltip("Playback speed multiplier");

            p.timeUnitsComboBox = ftk::ComboBox::create(context, tl::timeline::getTimeUnitsLabels());
            p.timeUnitsComboBox->setTooltip("Time units");

            p.audioButton = ftk::ToolButton::create(context);
            p.audioButton->setIcon("Volume");
            p.audioButton->setTooltip("Audio volume");
            actions = audioActions->getActions();
            p.muteButton = ftk::ToolButton::create(context, actions["Mute"]);

            p.layout = ftk::HorizontalLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::MarginInside);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(ftk::SizeRole::None);
            p.buttons["Reverse"]->setParent(hLayout);
            p.buttons["Stop"]->setParent(hLayout);
            p.buttons["Forward"]->setParent(hLayout);
            p.playbackShuttle->setParent(hLayout);
            p.buttons["Start"]->setParent(hLayout);
            p.buttons["Prev"]->setParent(hLayout);
            p.buttons["Next"]->setParent(hLayout);
            p.buttons["End"]->setParent(hLayout);
            p.frameShuttle->setParent(hLayout);
            p.currentTimeEdit->setParent(p.layout);
            p.durationLabel->setParent(p.layout);
            hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingTool);
            p.speedEdit->setParent(hLayout);
            p.speedButton->setParent(hLayout);
            p.speedMultLabel->setParent(hLayout);
            p.timeUnitsComboBox->setParent(p.layout);
            auto spacer = ftk::Spacer::create(context, ftk::Orientation::Horizontal, p.layout);
            spacer->setHStretch(ftk::Stretch::Expanding);
            hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingTool);
            p.audioButton->setParent(hLayout);
            p.muteButton->setParent(hLayout);

            p.playbackShuttle->setActiveCallback(
                [this](bool value)
                {
                    FTK_P();
                    if (p.player)
                    {
                        if (value)
                        {
                            if (p.player->isStopped())
                            {
                                p.player->forward();
                            }
                        }
                        else
                        {
                            p.player->setSpeedMult(1.0);
                        }
                    }
                });
            p.playbackShuttle->setCallback(
                [this](int value)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->setSpeedMult(1.0 + value / 10.0);
                    }
                });

            p.frameShuttle->setActiveCallback(
                [this](bool)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->stop();
                        p.startTime = p.player->getCurrentTime();
                    }
                });
            p.frameShuttle->setCallback(
                [this](int value)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->seek(OTIO_NS::RationalTime(
                            p.startTime.value() + value,
                            p.startTime.rate()));
                    }
                });

            p.currentTimeEdit->setCallback(
                [this](const OTIO_NS::RationalTime& value)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->stop();
                        p.player->seek(value);
                        p.currentTimeEdit->setValue(p.player->getCurrentTime());
                    }
                });

            p.speedButton->setPressedCallback(
                [this]
                {
                    _showSpeedPopup();
                });

            auto appWeak = std::weak_ptr<App>(app);
            p.timeUnitsComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getTimeUnitsModel()->setTimeUnits(
                            static_cast<tl::timeline::TimeUnits>(value));
                    }
                });

            p.audioButton->setPressedCallback(
                [this]
                {
                    _showAudioPopup();
                });
            p.muteButton->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getAudioModel()->setMute(value);
                    }
                });

            p.timeUnitsObserver = ftk::Observer<tl::timeline::TimeUnits>::create(
                app->getTimeUnitsModel()->observeTimeUnits(),
                [this](tl::timeline::TimeUnits value)
                {
                    _p->timeUnitsComboBox->setCurrentIndex(static_cast<int>(value));
                });

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::timeline::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::timeline::Player>& value)
                {
                    _playerUpdate(value);
                });

            p.speedObserver2 = ftk::Observer<double>::create(
                p.speedModel->observeValue(),
                [this](double value)
                {
                    FTK_P();
                    if (p.player)
                    {
                        p.player->setSpeed(value);
                    }
                });
        }

        BottomToolBar::BottomToolBar() :
            _p(new Private)
        {}

        BottomToolBar::~BottomToolBar()
        {}

        std::shared_ptr<BottomToolBar> BottomToolBar::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<PlaybackActions>& playbackActions,
            const std::shared_ptr<FrameActions>& frameActions,
            const std::shared_ptr<AudioActions>& audioActions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<BottomToolBar>(new BottomToolBar);
            out->_init(context, app, playbackActions, frameActions, audioActions, parent);
            return out;
        }

        void BottomToolBar::focusCurrentFrame()
        {
            if (_p->currentTimeEdit->isEnabled())
            {
                _p->currentTimeEdit->takeKeyFocus();
                _p->currentTimeEdit->selectAll();
            }
        }

        ftk::Size2I BottomToolBar::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void BottomToolBar::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void BottomToolBar::_playerUpdate(const std::shared_ptr<tl::timeline::Player>& value)
        {
            FTK_P();

            p.speedObserver.reset();
            p.speedMultObserver.reset();
            p.currentTimeObserver.reset();
            p.inOutRangeObserver.reset();

            p.player = value;

            if (p.player)
            {
                p.speedObserver = ftk::Observer<double>::create(
                    p.player->observeSpeed(),
                    [this](double value)
                    {
                        _p->speedModel->setValue(value);
                    });

                p.speedMultObserver = ftk::Observer<double>::create(
                    p.player->observeSpeedMult(),
                    [this](double value)
                    {
                        _p->speedMultLabel->setText(ftk::Format("{0}X").arg(value, 1));
                        _p->speedMultLabel->setBackgroundRole(value > 1.0 ?
                            ftk::ColorRole::Checked :
                            ftk::ColorRole::None);
                    });

                p.currentTimeObserver = ftk::Observer<OTIO_NS::RationalTime>::create(
                    p.player->observeCurrentTime(),
                    [this](const OTIO_NS::RationalTime& value)
                    {
                        _p->currentTimeEdit->setValue(value);
                    });

                p.inOutRangeObserver = ftk::Observer<OTIO_NS::TimeRange>::create(
                    p.player->observeInOutRange(),
                    [this](const OTIO_NS::TimeRange& value)
                    {
                        _p->durationLabel->setValue(value.duration());
                    });
            }
            else
            {
                p.currentTimeEdit->setValue(tl::time::invalidTime);
                p.durationLabel->setValue(tl::time::invalidTime);
                p.speedModel->setValue(0.0);
                p.speedMultLabel->setText("1.0X");
                p.speedMultLabel->setBackgroundRole(ftk::ColorRole::None);
            }

            p.playbackShuttle->setEnabled(p.player.get());
            p.frameShuttle->setEnabled(p.player.get());
            p.currentTimeEdit->setEnabled(p.player.get());
            p.durationLabel->setEnabled(p.player.get());
            p.speedEdit->setEnabled(p.player.get());
            p.speedButton->setEnabled(p.player.get());
            p.speedMultLabel->setEnabled(p.player.get());
        }

        void BottomToolBar::_showSpeedPopup()
        {
            FTK_P();
            auto context = getContext();
            auto window = getWindow();
            if (context && window)
            {
                if (!p.speedPopup)
                {
                    const double defaultSpeed =
                        p.player ?
                        p.player->getDefaultSpeed() :
                        0.0;
                    p.speedPopup = SpeedPopup::create(context, defaultSpeed);
                    p.speedPopup->open(window, p.speedButton->getGeometry());
                    std::weak_ptr<BottomToolBar> weak(std::dynamic_pointer_cast<BottomToolBar>(shared_from_this()));
                    p.speedPopup->setCallback(
                        [weak](double value)
                        {
                            if (auto widget = weak.lock())
                            {
                                if (widget->_p->player)
                                {
                                    widget->_p->player->setSpeed(value);
                                }
                                widget->_p->speedPopup->close();
                            }
                        });
                    p.speedPopup->setCloseCallback(
                        [weak]
                        {
                            if (auto widget = weak.lock())
                            {
                                widget->_p->speedPopup.reset();
                            }
                        });
                }
                else
                {
                    p.speedPopup->close();
                    p.speedPopup.reset();
                }
            }
        }

        void BottomToolBar::_showAudioPopup()
        {
            FTK_P();
            auto context = getContext();
            auto app = p.app.lock();
            auto window = getWindow();

            if (context && app && window)
            {
                if (!p.audioPopup)
                {
                    p.audioPopup = AudioPopup::create(context, app);
                    p.audioPopup->open(window, p.audioButton->getGeometry());
                    std::weak_ptr<BottomToolBar> weak(std::dynamic_pointer_cast<BottomToolBar>(shared_from_this()));
                    p.audioPopup->setCloseCallback(
                        [weak]
                        {
                            if (auto widget = weak.lock())
                            {
                                widget->_p->audioPopup.reset();
                            }
                        });
                }
                else
                {
                    p.audioPopup->close();
                    p.audioPopup.reset();
                }
            }
        }
    }
}
