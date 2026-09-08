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
            std::shared_ptr<ftk::PushButton> closeButton;
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

            setTitle("About");


            auto licensesButton = ftk::PushButton::create(context, "Additional Licenses");
            p.closeButton = ftk::PushButton::create(context, "Close");

            auto layout = ftk::VerticalLayout::create(context, shared_from_this());
            layout->setSpacingRole(ftk::SizeRole::None);

            auto vLayout = ftk::VerticalLayout::create(context);
            vLayout->setMarginRole(ftk::SizeRole::Margin);
            vLayout->setSpacingRole(ftk::SizeRole::Spacing);
            ftk::Label::create(
                context,
                ftk::Format("{0} {1}").
                    arg(appInfoModel->getFullName()).
                    arg(appInfoModel->getVersion()),
                vLayout);
            // Which build this is, which is the question the dialog is
            // usually open to answer.
            std::string built = ftk::Format("Built {0}, {1}").
                arg(appInfoModel->getCommitDate()).
                arg(appInfoModel->getGitCommit());
            // And which DJV it was built on, for an application that is not
            // DJV: the two carry version numbers of their own and release on
            // their own schedules, so a report naming one of them is half an
            // answer.
            if (appInfoModel->getGitCommit() != appInfoModel->getLibraryCommit())
            {
                built = ftk::Format("{0}, on DJV {1} {2}").
                    arg(built).
                    arg(appInfoModel->getLibraryVersion()).
                    arg(appInfoModel->getLibraryCommit());
            }
            ftk::Label::create(context, built, vLayout);
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
            p.closeButton->setParent(hLayout);

            p.closeButton->setClickedCallback(
                [this]
                {
                    close();
                });

            // The notices are installed with the application, so a build
            // that was not installed has none to show. The button says so
            // rather than opening nothing; unlike a menu item, a button can
            // carry a tooltip that is read.
            const std::string url = appInfoModel->getLicensesURL();
            if (url.empty())
            {
                licensesButton->setEnabled(false);
                licensesButton->setTooltip(
                    "The licenses are installed with the application, and "
                    "this build was not installed.");
            }
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

        std::shared_ptr<ftk::IWidget> AboutDialog::getKeyFocus() const
        {
            return _p->closeButton;
        }
    }
}
