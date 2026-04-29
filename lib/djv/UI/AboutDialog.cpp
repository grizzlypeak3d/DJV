// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/AboutDialog.h>

#include <djv/Models/AppInfoModel.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/OS.h>

namespace djv
{
    namespace ui
    {
        struct AboutDialog::Private
        {
        };

        void AboutDialog::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::AppInfoModel>& appInfoModel,
            const std::shared_ptr<IWidget>& parent)
        {
            IDialog::_init(
                context,
                "djv::app::AboutDialog",
                parent);
            FTK_P();

            auto titleLabel = ftk::Label::create(context, "About");
            titleLabel->setFontSize(14);
            titleLabel->setMarginRole(ftk::SizeRole::Margin);
            titleLabel->setBackgroundRole(ftk::ColorRole::Button);

            auto licensesButton = ftk::PushButton::create(context, "Additional Licenses");
            auto closeButton = ftk::PushButton::create(context, "Close");

            auto layout = ftk::VerticalLayout::create(context, shared_from_this());
            layout->setSpacingRole(ftk::SizeRole::None);
            titleLabel->setParent(layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);

            auto vLayout = ftk::VerticalLayout::create(context);
            vLayout->setMarginRole(ftk::SizeRole::Margin);
            vLayout->setSpacingRole(ftk::SizeRole::Spacing);
            ftk::Label::create(
                context,
                ftk::Format("{0} {1}").
                    arg(appInfoModel->getFullName()).
                    arg(appInfoModel->getVersion()),
                vLayout);
            ftk::Label::create(
                context,
                appInfoModel->getLicense(),
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

            const std::string url = appInfoModel->getLicensesURL();
            licensesButton->setClickedCallback(
                [url]
                {
                    ftk::openURL(url);
                });
        }

        AboutDialog::AboutDialog() :
            _p(new Private)
        {}

        AboutDialog::~AboutDialog()
        {}

        std::shared_ptr<AboutDialog> AboutDialog::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::AppInfoModel>& appInfoModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<AboutDialog>(new AboutDialog);
            out->_init(context, appInfoModel, parent);
            return out;
        }
    }
}
