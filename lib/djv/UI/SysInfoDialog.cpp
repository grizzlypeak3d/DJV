// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/SysInfoDialog.h>

#include <djv/Models/AppInfoModel.h>

#include <ftk/UI/ClipboardSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/IWindow.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/TextEdit.h>

#include <ftk/Core/String.h>

namespace djv
{
    namespace ui
    {
        struct SysInfoDialog::Private
        {
            std::vector<std::string> text;
            std::shared_ptr<ftk::PushButton> closeButton;
        };

        void SysInfoDialog::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::vector<std::string>& text,
            const std::shared_ptr<IWidget>& parent)
        {
            IDialog::_init(
                context,
                "djv::ui::SysInfoDialog",
                parent);
            FTK_P();
            
            p.text = text;

            setTitle("System Information");


            auto copyButton = ftk::PushButton::create(context, "Copy");
            p.closeButton = ftk::PushButton::create(context, "Close");

            auto layout = ftk::VerticalLayout::create(context, shared_from_this());
            layout->setSpacingRole(ftk::SizeRole::None);
            layout->setStretch(ftk::Stretch::Expanding, ftk::Stretch::Expanding);

            auto textEdit = ftk::TextEdit::create(context, layout);
            textEdit->setReadOnly(true);
            ftk::TextEditOptions textEditOptions;
            textEditOptions.fontInfo.name = ftk::getDefaultFont(ftk::FontType::Mono);
            textEdit->setOptions(textEditOptions);
            textEdit->setMarginRole(ftk::SizeRole::Margin);
            textEdit->setVStretch(ftk::Stretch::Expanding);
            textEdit->setText(p.text);

            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            auto hLayout = ftk::HorizontalLayout::create(context, layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            copyButton->setParent(hLayout);
            hLayout->addSpacer(ftk::SizeRole::Spacing, ftk::Stretch::Expanding);
            p.closeButton->setParent(hLayout);

            copyButton->setClickedCallback(
                [this]
                {
                    if (auto context = getContext())
                    {
                        auto clipboardSystem = context->getSystem<ftk::ClipboardSystem>();
                        clipboardSystem->setText(ftk::join(_p->text, '\n'));
                    }
                });

            p.closeButton->setClickedCallback(
                [this]
                {
                    close();
                });
        }

        SysInfoDialog::SysInfoDialog() :
            _p(new Private)
        {}

        SysInfoDialog::~SysInfoDialog()
        {}

        std::shared_ptr<SysInfoDialog> SysInfoDialog::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::vector<std::string>& text,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SysInfoDialog>(new SysInfoDialog);
            out->_init(context, text, parent);
            return out;
        }

        std::shared_ptr<ftk::IWidget> SysInfoDialog::getKeyFocus() const
        {
            return _p->closeButton;
        }
    }
}
