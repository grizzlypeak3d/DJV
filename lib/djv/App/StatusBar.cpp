// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/StatusBar.h>

#include <djv/App/App.h>
#include <djv/UI/StatusIndicatorPopup.h>
#include <djv/Models/AudioModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/ToolsModel.h>
#include <djv/Models/ViewportModel.h>

#if defined(TLRENDER_BMD)
#include <tlRender/Device/BMDOutputDevice.h>
#endif // TLRENDER_BMD

#include <ftk/UI/Divider.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/SysLogModel.h>
#include <ftk/UI/ToolButton.h>
#include <ftk/Core/Context.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>
#include <ftk/Core/Timer.h>

namespace djv
{
    namespace app
    {
        struct StatusBar::Private
        {
            std::weak_ptr<App> app;

            bool ocioOptionsEnabled = false;
            bool lutOptionsEnabled = false;
            bool channelsOptionsEnabled = false;
            bool mirrorOptionsEnabled = false;
            bool colorOptionsEnabled = false;
            bool audioOffsetEnabled = false;
            bool outputDeviceEnabled = false;

            std::shared_ptr<ftk::Label> messagesLabel;
            std::shared_ptr<ftk::Label> infoLabel;
            std::shared_ptr<ftk::ToolButton> indicatorButton;
            std::shared_ptr<ui::StatusIndicatorPopup> indicatorPopup;
            std::shared_ptr<ftk::HorizontalLayout> layout;

            std::shared_ptr<ftk::Timer> messagesTimer;

            std::shared_ptr<ftk::ListObserver<std::string> > messagesObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<double> > audioSyncOffsetObserver;
#if defined(TLRENDER_BMD)
            std::shared_ptr<ftk::Observer<bool> > bmdActiveObserver;
#endif // TLRENDER_BMD
        };

        void StatusBar::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IMouseWidget::_init(
                context,
                "djv::app::StatusBar",
                parent);
            FTK_P();

            setHStretch(ftk::Stretch::Expanding);
            _setMouseHoverEnabled(true);
            _setMousePressEnabled(true);

            p.app = app;

            p.messagesLabel = ftk::Label::create(context);
            p.messagesLabel->setMarginRole(ftk::SizeRole::MarginSmall, ftk::SizeRole::MarginInside);
            p.messagesLabel->setHStretch(ftk::Stretch::Expanding);
            p.messagesLabel->setClipText(true);
            p.messagesLabel->setTooltip(
                "Display messages.\n"
                "\n"
                "Click to open messages tool.");

            p.infoLabel = ftk::Label::create(context);
            p.infoLabel->setMarginRole(ftk::SizeRole::MarginSmall, ftk::SizeRole::MarginInside);
            p.infoLabel->setClipText(true);

            p.indicatorButton = ftk::ToolButton::create(context);
            p.indicatorButton->setIcon("MenuChecked");
            p.indicatorButton->setPopupIcon(true);
            p.indicatorButton->setTooltip(
                "This indicator shows options that can affect video, audio, or performance.\n"
                "Click to show which options are enabled.");

            p.layout = ftk::HorizontalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::SpacingTool);
            p.messagesLabel->setParent(p.layout);
            ftk::Divider::create(context, ftk::Orientation::Horizontal, p.layout);
            p.infoLabel->setParent(p.layout);
            ftk::Divider::create(context, ftk::Orientation::Horizontal, p.layout);
            p.indicatorButton->setParent(p.layout);

            p.messagesTimer = ftk::Timer::create(context);

            p.messagesObserver = ftk::ListObserver<std::string>::create(
                app->getSysLogModel()->observeMessages(),
                [this](const std::vector<std::string>& value)
                {
                    FTK_P();
                    std::string text;
                    if (!value.empty())
                    {
                        text = value.back();
                    }
                    p.messagesLabel->setText(text);
                    p.messagesLabel->setTooltip(text);
                    if (!value.empty())
                    {
                        p.messagesTimer->start(
                            std::chrono::seconds(5),
                            [this]
                            {
                                _p->messagesLabel->setText(std::string());
                                _p->messagesLabel->setTooltip(std::string());
                            });
                    }
                });

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& player)
                {
                    _infoUpdate(
                        player ? player->getPath() : ftk::Path(),
                        player ? player->getIOInfo() : tl::IOInfo());
                });

            p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                app->getColorModel()->observeOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                    _p->ocioOptionsEnabled = value.enabled;
                    _indicatorUpdate();
                });

            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                app->getColorModel()->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                    _p->lutOptionsEnabled = value.enabled;
                    _indicatorUpdate();
                });

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                app->getViewportModel()->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    FTK_P();
                    p.channelsOptionsEnabled =
                        value.channels != ftk::ChannelDisplay::Color;
                    p.mirrorOptionsEnabled =
                        value.mirror.x ||
                        value.mirror.y;
                    p.colorOptionsEnabled =
                        value.color.enabled    ||
                        value.levels.enabled   ||
                        value.exposure.enabled ||
                        value.softClip.enabled;
                    _indicatorUpdate();
                });

            p.audioSyncOffsetObserver = ftk::Observer<double>::create(
                app->getAudioModel()->observeSyncOffset(),
                [this](double value)
                {
                    _p->audioOffsetEnabled = value != 0.0;
                    _indicatorUpdate();
                });

#if defined(TLRENDER_BMD)
            p.bmdActiveObserver = ftk::Observer<bool>::create(
                app->getBMDOutputDevice()->observeActive(),
                [this](bool value)
                {
                    _p->outputDeviceEnabled = value;
                    _indicatorUpdate();
                });
#endif // TLRENDER_BMD

            p.indicatorButton->setPressedCallback(
                [this]
                {
                    _showIndicatorPopup();
                });
        }

        StatusBar::StatusBar() :
            _p(new Private)
        {}

        StatusBar::~StatusBar()
        {}

        std::shared_ptr<StatusBar> StatusBar::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<StatusBar>(new StatusBar);
            out->_init(context, app, parent);
            return out;
        }

        ftk::Size2I StatusBar::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void StatusBar::setGeometry(const ftk::Box2I & value)
        {
            IMouseWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void StatusBar::mousePressEvent(ftk::MouseClickEvent& event)
        {
            IMouseWidget::mousePressEvent(event);
            event.accept = true;
        }

        void StatusBar::mouseReleaseEvent(ftk::MouseClickEvent& event)
        {
            IMouseWidget::mouseReleaseEvent(event);
            FTK_P();
            event.accept = true;
            models::Tool tool = models::Tool::None;
            if (ftk::contains(p.messagesLabel->getGeometry(), event.pos))
            {
                tool = models::Tool::Messages;
            }
            else if (ftk::contains(p.infoLabel->getGeometry(), event.pos))
            {
                tool = models::Tool::Info;
            }
            if (tool != models::Tool::None)
            {
                if (auto app = p.app.lock())
                {
                    auto toolsModel = app->getToolsModel();
                    const models::Tool active = toolsModel->getActiveTool();
                    toolsModel->setActiveTool(tool != active ? tool : models::Tool::None);
                }
            }
        }

        void StatusBar::_infoUpdate(const ftk::Path& path, const tl::IOInfo& info)
        {
            FTK_P();
            const std::string tooltipFormat =
                "{0}\n"
                "\n"
                "Click to open the information tool.";
            const std::string tooltipDefault =
                "Display information about the current file.";

            std::vector<std::string> s;
            s.push_back(ftk::elide(path.getFileName()));
            if (!info.video.empty())
            {
                s.push_back(std::string(
                    ftk::Format("video: {0}").
                    arg(ftk::getLabel(info.video[0]))));
            }
            if (info.audio.isValid())
            {
                s.push_back(std::string(
                    ftk::Format("audio: {0}").
                    arg(tl::getLabel(info.audio, true))));
            }
            p.infoLabel->setText(ftk::join(s, ", "));

            s.clear();
            s.push_back(path.get());
            if (!info.video.empty())
            {
                s.push_back(std::string(
                    ftk::Format("Video: {0}").
                    arg(ftk::getLabel(info.video[0]))));
            }
            if (info.audio.isValid())
            {
                s.push_back(std::string(
                    ftk::Format("Audio: {0}").
                    arg(tl::getLabel(info.audio))));
            }
            const std::string tooltip = ftk::join(s, "\n");
            p.infoLabel->setTooltip(ftk::Format(tooltipFormat).
                arg(!tooltip.empty() ? tooltip : tooltipDefault));
        }

        void StatusBar::_indicatorUpdate()
        {
            FTK_P();
            const bool enabled =
                p.ocioOptionsEnabled  ||
                p.lutOptionsEnabled   ||
                p.channelsOptionsEnabled ||
                p.mirrorOptionsEnabled ||
                p.colorOptionsEnabled ||
                p.audioOffsetEnabled  ||
                p.outputDeviceEnabled;
            p.indicatorButton->setBackgroundRole(
                enabled ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
            if (p.indicatorPopup)
            {
                p.indicatorPopup->setOCIO(p.ocioOptionsEnabled);
                p.indicatorPopup->setLUT(p.lutOptionsEnabled);
                p.indicatorPopup->setChannels(p.channelsOptionsEnabled);
                p.indicatorPopup->setMirror(p.mirrorOptionsEnabled);
                p.indicatorPopup->setColor(p.colorOptionsEnabled);
                p.indicatorPopup->setAudioOffset(p.audioOffsetEnabled);
                p.indicatorPopup->setOutputDevice(p.outputDeviceEnabled);
            }
        }

        void StatusBar::_showIndicatorPopup()
        {
            FTK_P();
            if (!p.indicatorPopup)
            {
                p.indicatorPopup = ui::StatusIndicatorPopup::create(getContext());
                _indicatorUpdate();
                p.indicatorPopup->open(getWindow(), p.indicatorButton->getGeometry());
                std::weak_ptr<StatusBar> weak(std::dynamic_pointer_cast<StatusBar>(shared_from_this()));
                p.indicatorPopup->setCloseCallback(
                    [weak]
                    {
                        if (auto widget = weak.lock())
                        {
                            widget->_p->indicatorPopup.reset();
                        }
                    });
            }
            else
            {
                p.indicatorPopup->close();
                p.indicatorPopup.reset();
            }
        }
    }
}
