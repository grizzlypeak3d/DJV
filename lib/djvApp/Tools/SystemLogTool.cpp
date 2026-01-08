// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/SystemLogTool.h>

#include <djvApp/App.h>

#include <ftk/UI/ClipboardSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/IWindow.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/ToolButton.h>
#include <ftk/Core/Context.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>

namespace djv
{
    namespace app
    {
        namespace
        {
            const int messagesMax = 100;
        }

        struct SystemLogTool::Private
        {
            std::list<std::string> messages;
            std::shared_ptr<ftk::Label> label;
            std::shared_ptr<ftk::ScrollWidget> scrollWidget;
            std::shared_ptr<ftk::ToolButton> copyButton;
            std::shared_ptr<ftk::ToolButton> clearButton;
            std::shared_ptr<ftk::VerticalLayout> layout;
            std::shared_ptr<ftk::ListObserver<ftk::LogItem> > logObserver;
        };

        void SystemLogTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                Tool::SystemLog,
                "djv::app::SystemLogTool",
                parent);
            FTK_P();

            p.label = ftk::Label::create(context);
            p.label->setFontRole(ftk::FontRole::Mono);
            p.label->setMarginRole(ftk::SizeRole::Margin);
            p.label->setVAlign(ftk::VAlign::Top);

            p.scrollWidget = ftk::ScrollWidget::create(context);
            p.scrollWidget->setWidget(p.label);
            p.scrollWidget->setBorder(false);
            p.scrollWidget->setVStretch(ftk::Stretch::Expanding);

            p.copyButton = ftk::ToolButton::create(context, "Copy");

            p.clearButton = ftk::ToolButton::create(context, "Clear");

            p.layout = ftk::VerticalLayout::create(context);
            p.layout->setSpacingRole(ftk::SizeRole::None);
            p.scrollWidget->setParent(p.layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginInside);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingTool);
            p.copyButton->setParent(hLayout);
            p.clearButton->setParent(hLayout);
            _setWidget(p.layout);

            p.copyButton->setClickedCallback(
                [this]
                {
                    if (auto context = getContext())
                    {
                        const std::string text = ftk::join(
                            { std::begin(_p->messages), std::end(_p->messages) },
                            '\n');
                        auto clipboardSystem = context->getSystem<ftk::ClipboardSystem>();
                        clipboardSystem->setText(text);
                    }
                });

            p.clearButton->setClickedCallback(
                [this]
                {
                    _p->messages.clear();
                    _p->label->setText(std::string());
                });

            p.logObserver = ftk::ListObserver<ftk::LogItem>::create(
                context->getLogSystem()->observeLogItems(),
                [this](const std::vector<ftk::LogItem>& value)
                {
                    for (const auto& i : value)
                    {
                        _p->messages.push_back(ftk::getLabel(i));
                    }
                    while (_p->messages.size() > messagesMax)
                    {
                        _p->messages.pop_front();
                    }
                    _p->label->setText(ftk::join(
                        { std::begin(_p->messages), std::end(_p->messages) },
                        '\n'));
                });
        }

        SystemLogTool::SystemLogTool() :
            _p(new Private)
        {}

        SystemLogTool::~SystemLogTool()
        {}

        std::shared_ptr<SystemLogTool> SystemLogTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SystemLogTool>(new SystemLogTool);
            out->_init(context, app, parent);
            return out;
        }
    }
}
