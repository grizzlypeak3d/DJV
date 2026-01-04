// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Widgets/SysInfoDialog.h>
#include <djvApp/MainWindow.h>

#include <ftk/UI/ClipboardSystem.h>
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
        struct SysInfoDialog::Private
        {
            std::string text;
        };

        void SysInfoDialog::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IDialog::_init(
                context,
                "djv::app::SysInfoDialog",
                parent);
            FTK_P();

            std::vector<std::pair<std::string, std::string> > labels;
            const auto sysInfo = ftk::getSysInfo();
            labels.push_back(std::make_pair("System: ", sysInfo.name));
            labels.push_back(std::make_pair("CPU Cores: ", ftk::Format("{0}").arg(sysInfo.cores)));
            labels.push_back(std::make_pair("Memory: ", ftk::Format("{0}GB").arg(sysInfo.ramGB)));
            labels.push_back(std::make_pair("", ""));
            const auto windowInfo = mainWindow->getWindowInfo();
            for (const auto& i : windowInfo)
            {
                labels.push_back(std::make_pair(i.first + ": ", i.second));
            }

            size_t sizeMax = 0;
            for (const auto& i : labels)
            {
                sizeMax = std::max(sizeMax, i.first.size());
            }
            for (auto& i : labels)
            {
                if (!i.first.empty())
                {
                    i.first.resize(sizeMax, ' ');
                }
            }

            std::vector<std::string> text;
            for (const auto& i : labels)
            {
                text.push_back(i.first + i.second);
            }
            p.text = ftk::join(text, '\n');

            auto titleLabel = ftk::Label::create(context, "System Information");
            titleLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            auto copyButton = ftk::PushButton::create(context, "Copy");
            auto closeButton = ftk::PushButton::create(context, "Close");

            auto layout = ftk::VerticalLayout::create(context, shared_from_this());
            layout->setSpacingRole(ftk::SizeRole::None);
            titleLabel->setParent(layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);

            auto label = ftk::Label::create(context, p.text);
            label->setFontRole(ftk::FontRole::Mono);
            label->setMarginRole(ftk::SizeRole::MarginSmall);
            auto scrollWidget = ftk::ScrollWidget::create(context, ftk::ScrollType::Vertical, layout);
            scrollWidget->setBorder(false);
            scrollWidget->setWidget(label);

            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            auto hLayout = ftk::HorizontalLayout::create(context, layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            copyButton->setParent(hLayout);
            hLayout->addSpacer(ftk::SizeRole::Spacing, ftk::Stretch::Expanding);
            closeButton->setParent(hLayout);

            copyButton->setClickedCallback(
                [this]
                {
                    if (auto context = getContext())
                    {
                        auto clipboardSystem = context->getSystem<ftk::ClipboardSystem>();
                        clipboardSystem->setText(_p->text);
                    }
                });

            closeButton->setClickedCallback(
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
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SysInfoDialog>(new SysInfoDialog);
            out->_init(context, mainWindow, parent);
            return out;
        }
    }
}
