// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/SysLogTool.h>

#include <djv/App/App.h>

#include <ftk/UI/ClipboardSystem.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/IWindow.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/Settings.h>
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
        struct SysLogTool::Private
        {
            std::shared_ptr<ftk::Settings> settings;

            std::shared_ptr<ftk::TextEdit> textEdit;
            std::shared_ptr<ftk::ToolButton> copyButton;
            std::shared_ptr<ftk::ToolButton> clearButton;
            std::shared_ptr<ftk::CheckBox> autoScrollCheckBox;

            std::shared_ptr<ftk::ListObserver<std::string> > logObserver;
        };

        void SysLogTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                models::Tool::SysLog,
                "djv::app::SysLogTool",
                parent);
            FTK_P();

            p.settings = app->getSettings();

            p.textEdit = ftk::TextEdit::create(context);
            p.textEdit->setReadOnly(true);
            ftk::TextEditOptions textEditOptions;
            textEditOptions.fontInfo.name = ftk::getDefaultFont(ftk::FontType::Mono);
            p.textEdit->setOptions(textEditOptions);
            p.textEdit->setVStretch(ftk::Stretch::Expanding);

            p.copyButton = ftk::ToolButton::create(context, "Copy");

            p.clearButton = ftk::ToolButton::create(context, "Clear");

            p.autoScrollCheckBox = ftk::CheckBox::create(context, "Auto-scroll");
            bool autoScroll = true;
            p.settings->get(
                ftk::Format("/{0}/AutoScroll").arg(getLabel(models::Tool::SysLog)),
                autoScroll);
            p.autoScrollCheckBox->setChecked(autoScroll);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setMarginRole(ftk::SizeRole::MarginSmall);
            layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.textEdit->setParent(layout);
            auto hLayout = ftk::HorizontalLayout::create(context, layout);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.copyButton->setParent(hLayout);
            p.clearButton->setParent(hLayout);
            p.autoScrollCheckBox->setParent(hLayout);
            _setWidget(layout);

            p.copyButton->setClickedCallback(
                [this]
                {
                    FTK_P();
                    auto context = getContext();
                    auto clipboardSystem = context->getSystem<ftk::ClipboardSystem>();
                    clipboardSystem->setText(ftk::join(p.textEdit->getText(), '\n'));
                });

            std::weak_ptr<App> appWeak(app);
            p.clearButton->setClickedCallback(
                [appWeak]
                {
                    appWeak.lock()->getSysLogModel()->clearLog();
                });

            p.logObserver = ftk::ListObserver<std::string>::create(
                app->getSysLogModel()->observeLog(),
                [this](const std::vector<std::string>& value)
                {
                    FTK_P();

                    // Save text edit state.
                    const auto cursor = p.textEdit->getModel()->getCursor();
                    const auto selection = p.textEdit->getModel()->getSelection();
                    const ftk::V2I scrollPos = p.textEdit->getScrollWidget()->getScrollPos();

                    // Update the text.
                    p.textEdit->setText(value);

                    if (p.autoScrollCheckBox->isChecked())
                    {
                        // Auto-scroll.
                        p.textEdit->getModel()->setCursor(ftk::TextEditPos(0, 0));
                        p.textEdit->getModel()->setCursor(ftk::TextEditPos(value.size(), 0));
                    }
                    else
                    {
                        // Restore text edit state.
                        p.textEdit->getModel()->setCursor(cursor);
                        p.textEdit->getModel()->setSelection(selection);
                        p.textEdit->getScrollWidget()->setScrollPos(scrollPos);
                    }
                });
        }

        SysLogTool::SysLogTool() :
            _p(new Private)
        {}

        SysLogTool::~SysLogTool()
        {
            FTK_P();
            p.settings->set(
                ftk::Format("/{0}/AutoScroll").arg(getLabel(models::Tool::SysLog)),
                p.autoScrollCheckBox->isChecked());
        }

        std::shared_ptr<SysLogTool> SysLogTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SysLogTool>(new SysLogTool);
            out->_init(context, app, parent);
            return out;
        }
    }
}
