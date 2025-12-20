// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Widgets/StatusBar.h>

#include <djvApp/Models/ToolsModel.h>
#include <djvApp/App.h>

#if defined(TLRENDER_BMD)
#include <tlRender/Device/BMDOutputDevice.h>
#endif // TLRENDER_BMD

#include <ftk/UI/Divider.h>
#include <ftk/UI/Icon.h>
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

            std::shared_ptr<ftk::Label> logLabel;
            std::shared_ptr<ftk::Label> infoLabel;
#if defined(TLRENDER_BMD)
            std::shared_ptr<ftk::Icon> deviceActiveIcon;
#endif // TLRENDER_BMD
            std::shared_ptr<ftk::HorizontalLayout> layout;

            std::shared_ptr<ftk::Timer> logTimer;

            std::shared_ptr<ftk::ListObserver<ftk::LogItem> > logObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::timeline::Player> > > playerObserver;
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
            p.logLabel->setMarginRole(ftk::SizeRole::MarginSmall, ftk::SizeRole::MarginInside);
            p.logLabel->setHStretch(ftk::Stretch::Expanding);

            p.infoLabel = ftk::Label::create(context);
            p.infoLabel->setMarginRole(ftk::SizeRole::MarginSmall, ftk::SizeRole::MarginInside);

#if defined(TLRENDER_BMD)
            p.deviceActiveIcon = ftk::Icon::create(context, "Devices");
            p.deviceActiveIcon->setMarginRole(ftk::SizeRole::MarginInside);
            p.deviceActiveIcon->setTooltip("Output device");
#endif // TLRENDER_BMD

            p.layout = ftk::HorizontalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::SpacingTool);
            p.logLabel->setParent(p.layout);
            ftk::Divider::create(context, ftk::Orientation::Horizontal, p.layout);
            p.infoLabel->setParent(p.layout);
#if defined(TLRENDER_BMD)
            ftk::Divider::create(context, ftk::Orientation::Horizontal, p.layout);
            p.deviceActiveIcon->setParent(p.layout);
            //ftk::Spacer::create(context, ftk::Orientation::Horizontal, p.layout);
#endif // TLRENDER_BMD

            _deviceUpdate(false);

            p.logTimer = ftk::Timer::create(context);

            p.logObserver = ftk::ListObserver<ftk::LogItem>::create(
                context->getLogSystem()->observeLogItems(),
                [this](const std::vector<ftk::LogItem>& value)
                {
                    _logUpdate(value);
                });

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::timeline::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::timeline::Player>& player)
                {
                    _infoUpdate(
                        player ? player->getPath() : ftk::Path(),
                        player ? player->getIOInfo() : tl::io::Info());
                });

#if defined(TLRENDER_BMD)
            p.bmdActiveObserver = ftk::Observer<bool>::create(
                app->getBMDOutputDevice()->observeActive(),
                [this](bool value)
                {
                    _deviceUpdate(value);
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

        void StatusBar::setGeometry(const ftk::Box2I & value)
        {
            IMouseWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void StatusBar::sizeHintEvent(const ftk::SizeHintEvent & event)
        {
            setSizeHint(_p->layout->getSizeHint());
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
#if defined(TLRENDER_BMD)
            else if (ftk::contains(p.deviceActiveIcon->getGeometry(), event.pos))
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
                case ftk::LogType::Error:
                {
                    const std::string s = ftk::getLabel(i, true);
                    p.logLabel->setText(s);
                    p.logLabel->setTooltip(s);
                    p.logTimer->start(
                        std::chrono::seconds(5),
                        [this]
                        {
                            _p->logLabel->setText(std::string());
                            _p->logLabel->setTooltip(std::string());
                        });
                    break;
                }
                default: break;
                }
            }
        }

        void StatusBar::_infoUpdate(const ftk::Path& path, const tl::io::Info& info)
        {
            FTK_P();
            std::vector<std::string> s;
            s.push_back(ftk::elide(path.getFileName()));
            if (!info.video.empty())
            {
                s.push_back(std::string(
                    ftk::Format("video: {0}x{1}:{2} {3}").
                    arg(info.video[0].size.w).
                    arg(info.video[0].size.h).
                    arg(info.video[0].getAspect(), 2).
                    arg(info.video[0].type)));
            }
            if (info.audio.isValid())
            {
                s.push_back(std::string(
                    ftk::Format("audio: {0}ch {1} {2}kHz").
                    arg(info.audio.channelCount).
                    arg(info.audio.dataType).
                    arg(info.audio.sampleRate / 1000)));
            }
            const std::string text = ftk::join(s, ", ");
            p.infoLabel->setText(text);

            std::vector<std::string> t;
            t.push_back(path.get());
            if (!info.video.empty())
            {
                t.push_back(std::string(
                    ftk::Format("Video: {0}x{1}:{2} {3}").
                    arg(info.video[0].size.w).
                    arg(info.video[0].size.h).
                    arg(info.video[0].getAspect()).
                    arg(info.video[0].type)));
            }
            if (info.audio.isValid())
            {
                t.push_back(std::string(
                    ftk::Format("Audio: {0} {1} {2} {3}kHz").
                    arg(info.audio.channelCount).
                    arg(1 == info.audio.channelCount ? "channel" : "channels").
                    arg(info.audio.dataType).
                    arg(info.audio.sampleRate / 1000)));
            }
            const std::string tooltip = ftk::join(t, "\n");
            p.infoLabel->setTooltip(tooltip);
        }

        void StatusBar::_deviceUpdate(bool value)
        {
            FTK_P();
#if defined(TLRENDER_BMD)
            p.deviceActiveIcon->setBackgroundRole(value ? ftk::ColorRole::Checked : ftk::ColorRole::None);
#endif // TLRENDER_BMD  
        }
    }
}
