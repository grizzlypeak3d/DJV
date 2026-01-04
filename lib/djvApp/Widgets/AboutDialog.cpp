// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Widgets/AboutDialog.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/OS.h>

namespace djv
{
    namespace app
    {
        struct AboutDialog::Private
        {};

        void AboutDialog::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            IDialog::_init(
                context,
                "djv::app::AboutDialog",
                parent);
            FTK_P();

            auto titleLabel = ftk::Label::create(context, "About");
            titleLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            auto licensesButton = ftk::PushButton::create(context, "Licenses");
            auto closeButton = ftk::PushButton::create(context, "Close");

            auto layout = ftk::VerticalLayout::create(context, shared_from_this());
            layout->setSpacingRole(ftk::SizeRole::None);
            titleLabel->setParent(layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);

            auto vLayout = ftk::VerticalLayout::create(context);
            vLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            vLayout->setSpacingRole(ftk::SizeRole::Spacing);
            ftk::Label::create(
                context,
                ftk::Format("DJV {0}").arg(DJV_VERSION_FULL),
                vLayout);
            ftk::Label::create(
                context,
                ftk::Format("Copyright Contributors to the DJV project."),
                vLayout);
            licensesButton->setParent(vLayout);
            auto scrollWidget = ftk::ScrollWidget::create(context, ftk::ScrollType::Vertical, layout);
            scrollWidget->setBorder(false);
            scrollWidget->setWidget(vLayout);

            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            auto hLayout = ftk::HorizontalLayout::create(context, layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            hLayout->addSpacer(ftk::SizeRole::Spacing, ftk::Stretch::Expanding);
            closeButton->setParent(hLayout);

            closeButton->setClickedCallback(
                [this]
                {
                    close();
                });

            licensesButton->setClickedCallback(
                [this]
                {
                    ftk::openURL("https://github.com/grizzlypeak3d/DJV/tree/main/etc/Legal");
                });
        }

        AboutDialog::AboutDialog() :
            _p(new Private)
        {}

        AboutDialog::~AboutDialog()
        {}

        std::shared_ptr<AboutDialog> AboutDialog::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<AboutDialog>(new AboutDialog);
            out->_init(context, parent);
            return out;
        }
    }
}
