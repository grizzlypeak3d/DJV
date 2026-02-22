// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/MessagesTool.h>

#include <djv/App/App.h>

#include <ftk/UI/ClipboardSystem.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/IWindow.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/SysLogModel.h>
#include <ftk/UI/TextEdit.h>
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
            const int messagesMax = 20;
        }

        struct MessagesTool::Private
        {
            std::shared_ptr<ftk::TextEdit> textEdit;
            std::shared_ptr<ftk::ToolButton> copyButton;
            std::shared_ptr<ftk::CheckBox> autoScrollCheckBox;
            std::shared_ptr<ftk::VerticalLayout> layout;
            std::shared_ptr<ftk::ListObserver<std::string> > messagesObserver;
        };

        void MessagesTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                models::Tool::Messages,
                "djv::app::MessagesTool",
                parent);
            FTK_P();

            p.textEdit = ftk::TextEdit::create(context);
            p.textEdit->setReadOnly(true);
            p.textEdit->setVStretch(ftk::Stretch::Expanding);

            p.copyButton = ftk::ToolButton::create(context, "Copy");

            p.autoScrollCheckBox = ftk::CheckBox::create(context, "Auto-scroll");
            p.autoScrollCheckBox->setChecked(true);

            p.layout = ftk::VerticalLayout::create(context);
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.textEdit->setParent(p.layout);
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.copyButton->setParent(hLayout);
            p.autoScrollCheckBox->setParent(hLayout);
            _setWidget(p.layout);

            p.copyButton->setClickedCallback(
                [this]
                {
                    FTK_P();
                    auto context = getContext();
                    auto clipboardSystem = context->getSystem<ftk::ClipboardSystem>();
                    clipboardSystem->setText(ftk::join(p.textEdit->getText(), '\n'));
                });

            p.messagesObserver = ftk::ListObserver<std::string>::create(
                app->getSysLogModel()->observeMessages(),
                [this](const std::vector<std::string>& value)
                {
                    FTK_P();
                    const auto cursorPrev = p.textEdit->getModel()->getCursor();
                    const auto selectionPrev = p.textEdit->getModel()->getSelection();
                    p.textEdit->setText(value);
                    if (p.autoScrollCheckBox->isChecked())
                    {
                        p.textEdit->getModel()->setCursor(
                            ftk::TextEditPos(0, 0));
                        p.textEdit->getModel()->setCursor(
                            ftk::TextEditPos(value.size(), 0));
                    }
                    else
                    {
                        p.textEdit->getModel()->setCursor(cursorPrev);
                        p.textEdit->getModel()->setSelection(selectionPrev);
                    }
                });
        }

        MessagesTool::MessagesTool() :
            _p(new Private)
        {}

        MessagesTool::~MessagesTool()
        {}

        std::shared_ptr<MessagesTool> MessagesTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<MessagesTool>(new MessagesTool);
            out->_init(context, app, parent);
            return out;
        }
    }
}
