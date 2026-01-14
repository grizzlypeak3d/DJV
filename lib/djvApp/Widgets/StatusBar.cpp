// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Widgets/StatusBar.h>

#include <djvApp/Models/AudioModel.h>
#include <djvApp/Models/ColorModel.h>
#include <djvApp/Models/ToolsModel.h>
#include <djvApp/Models/ViewportModel.h>
#include <djvApp/App.h>

#if defined(TLRENDER_BMD)
#include <tlRender/Device/BMDOutputDevice.h>
#endif // TLRENDER_BMD

#include <ftk/UI/Divider.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/Spacer.h>
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
            bool displayOptionsEnabled = false;

            std::shared_ptr<ftk::Label> logLabel;
            std::shared_ptr<ftk::Label> infoLabel;
            std::shared_ptr<ftk::Label> channelDisplayLabel;
            std::shared_ptr<ftk::Label> mirrorHLabel;
            std::shared_ptr<ftk::Label> mirrorVLabel;
            std::shared_ptr<ftk::Label> colorControlsLabel;
            std::shared_ptr<ftk::Label> audioSyncLabel;
#if defined(TLRENDER_BMD)
            std::shared_ptr<ftk::Label> outputDeviceLabel;
#endif // TLRENDER_BMD
            std::shared_ptr<ftk::HorizontalLayout> layout;

            std::shared_ptr<ftk::Timer> logTimer;

            std::shared_ptr<ftk::ListObserver<ftk::LogItem> > logObserver;
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

            p.logLabel = ftk::Label::create(context);
            p.logLabel->setHMarginRole(ftk::SizeRole::MarginInside);
            p.logLabel->setHStretch(ftk::Stretch::Expanding);
            p.logLabel->setTooltip(
                "Display warning and error messages.\n"
                "\n"
                "Click to open messages tool.");

            p.infoLabel = ftk::Label::create(context);
            p.infoLabel->setHMarginRole(ftk::SizeRole::MarginInside);

            p.channelDisplayLabel = ftk::Label::create(context, "C");
            p.channelDisplayLabel->setHMarginRole(ftk::SizeRole::MarginInside);
            p.channelDisplayLabel->setTooltip(
                "This displays whether channel display is enabled.");

            p.mirrorHLabel = ftk::Label::create(context, "H");
            p.mirrorHLabel->setHMarginRole(ftk::SizeRole::MarginInside);
            p.mirrorHLabel->setTooltip(
                "This displays whether mirror horizontal is enabled.");

            p.mirrorVLabel = ftk::Label::create(context, "V");
            p.mirrorVLabel->setHMarginRole(ftk::SizeRole::MarginInside);
            p.mirrorVLabel->setTooltip(
                "This displays whether mirror vertical is enabled.");

            p.colorControlsLabel = ftk::Label::create(context, "CC");
            p.colorControlsLabel->setHMarginRole(ftk::SizeRole::MarginInside);
            p.colorControlsLabel->setTooltip(
                "This displays whether color controls are enabled.\n"
                "\n"
                "Click to open color tool.");

            p.audioSyncLabel = ftk::Label::create(context, "AO");
            p.audioSyncLabel->setHMarginRole(ftk::SizeRole::MarginInside);
            p.audioSyncLabel->setTooltip(
                "This displays whether the audio sync offset is enabled.\n"
                "\n"
                "Click to open audio tool.");

#if defined(TLRENDER_BMD)
            p.outputDeviceLabel = ftk::Label::create(context, "OD");
            p.outputDeviceLabel->setHMarginRole(ftk::SizeRole::MarginInside);
            p.outputDeviceLabel->setTooltip(
                "This displays whether the output device is enabled.\n"
                "\n"
                "Click to open output device tool.");
#endif // TLRENDER_BMD

            p.layout = ftk::HorizontalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::SpacingTool);
            p.logLabel->setParent(p.layout);
            ftk::Divider::create(context, ftk::Orientation::Horizontal, p.layout);
            p.infoLabel->setParent(p.layout);
            ftk::Divider::create(context, ftk::Orientation::Horizontal, p.layout);
            p.channelDisplayLabel->setParent(p.layout);
            p.mirrorHLabel->setParent(p.layout);
            p.mirrorVLabel->setParent(p.layout);
            p.colorControlsLabel->setParent(p.layout);
            p.audioSyncLabel->setParent(p.layout);
#if defined(TLRENDER_BMD)
            p.outputDeviceLabel->setParent(p.layout);
#endif // TLRENDER_BMD
            //ftk::Spacer::create(context, ftk::Orientation::Horizontal, p.layout);

            p.logTimer = ftk::Timer::create(context);

            p.logObserver = ftk::ListObserver<ftk::LogItem>::create(
                context->getLogSystem()->observeLogItems(),
                [this](const std::vector<ftk::LogItem>& value)
                {
                    _logUpdate(value);
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
                    _colorControlsUpdate();
                });

            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                app->getColorModel()->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                    _p->lutOptionsEnabled = value.enabled;
                    _colorControlsUpdate();
                });

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                app->getViewportModel()->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    _p->channelDisplayLabel->setBackgroundRole(
                        value.channels != ftk::ChannelDisplay::Color ?
                        ftk::ColorRole::Checked :
                        ftk::ColorRole::None);
                    _p->mirrorHLabel->setBackgroundRole(
                        value.mirror.x ?
                        ftk::ColorRole::Checked :
                        ftk::ColorRole::None);
                    _p->mirrorVLabel->setBackgroundRole(
                        value.mirror.y ?
                        ftk::ColorRole::Checked :
                        ftk::ColorRole::None);
                    _p->displayOptionsEnabled =
                        value.color.enabled      ||
                        value.levels.enabled     ||
                        value.exrDisplay.enabled ||
                        value.softClip.enabled;
                    _colorControlsUpdate();
                });

            p.audioSyncOffsetObserver = ftk::Observer<double>::create(
                app->getAudioModel()->observeSyncOffset(),
                [this](double value)
                {
                    _p->audioSyncLabel->setBackgroundRole(
                        value != 0.0 ?
                        ftk::ColorRole::Checked :
                        ftk::ColorRole::None);
                });

#if defined(TLRENDER_BMD)
            p.bmdActiveObserver = ftk::Observer<bool>::create(
                app->getBMDOutputDevice()->observeActive(),
                [this](bool value)
                {
                    _p->outputDeviceLabel->setBackgroundRole(
                        value ?
                        ftk::ColorRole::Checked :
                        ftk::ColorRole::None);
                });
#endif // TLRENDER_BMD
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
            Tool tool = Tool::None;
            if (ftk::contains(p.logLabel->getGeometry(), event.pos))
            {
                tool = Tool::Messages;
            }
            else if (ftk::contains(p.infoLabel->getGeometry(), event.pos))
            {
                tool = Tool::Info;
            }
            else if (ftk::contains(p.colorControlsLabel->getGeometry(), event.pos))
            {
                tool = Tool::Color;
            }
            else if (ftk::contains(p.audioSyncLabel->getGeometry(), event.pos))
            {
                tool = Tool::Audio;
            }
#if defined(TLRENDER_BMD)
            else if (ftk::contains(p.outputDeviceLabel->getGeometry(), event.pos))
            {
                tool = Tool::Devices;
            }
#endif // TLRENDER_BMD
            if (tool != Tool::None)
            {
                if (auto app = p.app.lock())
                {
                    auto toolsModel = app->getToolsModel();
                    const Tool active = toolsModel->getActiveTool();
                    toolsModel->setActiveTool(tool != active ? tool : Tool::None);
                }
            }
        }

        void StatusBar::_logUpdate(const std::vector<ftk::LogItem>& value)
        {
            FTK_P();
            for (const auto& i : value)
            {
                switch (i.type)
                {
                case ftk::LogType::Warning:
                case ftk::LogType::Error:
                {
                    p.logLabel->setText(ftk::getLabel(i, true));
                    p.logTimer->start(
                        std::chrono::seconds(5),
                        [this]
                        {
                            _p->logLabel->setText(std::string());
                        });
                    break;
                }
                default: break;
                }
            }
        }

        void StatusBar::_infoUpdate(const ftk::Path& path, const tl::IOInfo& info)
        {
            FTK_P();
            const std::string tooltipFormat =
                "{0}\n"
                "\n"
                "Click to open information tool.";
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
            const std::string text = ftk::join(s, ", ");
            p.infoLabel->setText(!text.empty() ? text : "-");

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

        void StatusBar::_colorControlsUpdate()
        {
            FTK_P();
            const bool enabled =
                p.ocioOptionsEnabled    ||
                p.lutOptionsEnabled     ||
                p.displayOptionsEnabled;
            p.colorControlsLabel->setBackgroundRole(
                enabled ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
        }
    }
}
