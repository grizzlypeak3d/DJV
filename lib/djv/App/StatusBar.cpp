// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/StatusBar.h>

#include <djv/App/App.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/ToolsModel.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/SysLogModel.h>
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

            std::shared_ptr<ftk::Label> messagesLabel;
            std::shared_ptr<ftk::Label> infoLabel;
            std::shared_ptr<ftk::HorizontalLayout> layout;

            std::shared_ptr<ftk::Timer> messagesTimer;

            std::shared_ptr<tl::Player> player;

            std::shared_ptr<ftk::ListObserver<ftk::LogItem> > messagesObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<std::string> > mediaReferenceKeyObserver;
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

            p.layout = ftk::HorizontalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::SpacingTool);
            p.messagesLabel->setParent(p.layout);
            ftk::Divider::create(context, ftk::Orientation::Horizontal, p.layout);
            p.infoLabel->setParent(p.layout);

            p.messagesTimer = ftk::Timer::create(context);

            p.messagesObserver = ftk::ListObserver<ftk::LogItem>::create(
                app->getSysLogModel()->observeMessages(),
                [this](const std::vector<ftk::LogItem>& value)
                {
                    FTK_P();
                    std::string text;
                    if (!value.empty())
                    {
                        // The message alone: it has just appeared, so the time
                        // it appeared at says nothing. The messages tool has
                        // the timestamps.
                        text = ftk::getLabel(value.back(), ftk::LogLabel::Message);
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
                    FTK_P();
                    p.player = player;
                    p.mediaReferenceKeyObserver.reset();
                    if (player)
                    {
                        // The information describes the media reference being
                        // read, so it is refreshed when the key changes. The
                        // observer also reports the current key, which covers
                        // the new player.
                        p.mediaReferenceKeyObserver = ftk::Observer<std::string>::create(
                            player->observeMediaReferenceKey(),
                            [this](const std::string&)
                            {
                                FTK_P();
                                _infoUpdate(
                                    p.player->getPath(),
                                    p.player->getIOInfo());
                            });
                    }
                    else
                    {
                        _infoUpdate(ftk::Path(), tl::IOInfo());
                    }
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
            std::string tool;
            if (ftk::contains(p.messagesLabel->getGeometry(), event.pos))
            {
                tool = "Messages";
            }
            else if (ftk::contains(p.infoLabel->getGeometry(), event.pos))
            {
                tool = "Information";
            }
            if (!tool.empty())
            {
                if (auto app = p.app.lock())
                {
                    auto toolsModel = app->getToolsModel();
                    const bool open = !toolsModel->isToolOpen(tool);
                    toolsModel->setToolOpen(tool, open);
                    if (open)
                    {
                        // See the note in ToolsActions: a tool opened while
                        // the panel is hidden has to bring it back.
                        auto window = app->getSettingsModel()->getWindow();
                        if (!window.tools)
                        {
                            window.tools = true;
                            app->getSettingsModel()->setWindow(window);
                        }
                    }
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
                    ftk::Format("V: {0}").
                    arg(ftk::getLabel(info.video[0]))));
            }
            if (info.audio.isValid())
            {
                s.push_back(std::string(
                    ftk::Format("A: {0}").
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

    }
}
