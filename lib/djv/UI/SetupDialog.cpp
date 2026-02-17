// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/SetupDialog.h>

#include <djv/UI/SettingsWidgets.h>
#include <djv/Models/SettingsModel.h>

#include <ftk/UI/ComboBox.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/StackLayout.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace ui
    {
        struct SetupStartWidget::Private
        {
            std::shared_ptr<ftk::VerticalLayout> layout;
        };

        void SetupStartWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            IWidget::_init(context, "djv::ui::SetupStartWidget", parent);
            FTK_P();
            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            ftk::Label::create(context, ftk::Format("Welcome to DJV version {0}.").arg(DJV_VERSION_FULL), p.layout);
            ftk::Label::create(context, "Start by configuring some settings.", p.layout);
            ftk::Label::create(context, "Changes can also be made later in the settings tool.", p.layout);
        }

        SetupStartWidget::SetupStartWidget() :
            _p(new Private)
        {}

        SetupStartWidget::~SetupStartWidget()
        {}

        std::shared_ptr<SetupStartWidget> SetupStartWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            auto out = std::shared_ptr<SetupStartWidget>(new SetupStartWidget);
            out->_init(context, parent);
            return out;
        }

        ftk::Size2I SetupStartWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void SetupStartWidget::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        struct SetupDialog::Private
        {
            std::shared_ptr<ftk::PushButton> nextButton;
            std::shared_ptr<ftk::PushButton> prevButton;
            std::shared_ptr<ftk::PushButton> closeButton;
            std::shared_ptr<ftk::StackLayout> stackLayout;
            std::vector<std::shared_ptr<ftk::IWidget> > widgets;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<bool> > nextObserver;
            std::shared_ptr<ftk::Observer<bool> > prevObserver;
        };

        void SetupDialog::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            IDialog::_init(
                context,
                "djv::ui::SetupDialog",
                parent);
            FTK_P();

            auto label = ftk::Label::create(
                context,
                ftk::Format("Setup").arg(DJV_VERSION_FULL));
            label->setFontRole(ftk::FontRole::Title);
            label->setMarginRole(ftk::SizeRole::Margin);

            p.nextButton = ftk::PushButton::create(context, "Next");
            p.prevButton = ftk::PushButton::create(context, "Previous");
            p.closeButton = ftk::PushButton::create(context, "Close");

            p.stackLayout = ftk::StackLayout::create(context);
            p.stackLayout->setMarginRole(ftk::SizeRole::MarginDialog);
            p.widgets.push_back(SetupStartWidget::create(context, p.stackLayout));
            auto vLayout = ftk::VerticalLayout::create(context, p.stackLayout);
            vLayout->setSpacingRole(ftk::SizeRole::SpacingLarge);
            ftk::Label::create(context, "Configure the style:", vLayout);
            p.widgets.push_back(StyleSettingsWidget::create(context, settings, vLayout));
            vLayout = ftk::VerticalLayout::create(context, p.stackLayout);
            vLayout->setSpacingRole(ftk::SizeRole::SpacingLarge);
            ftk::Label::create(context, "Configure the memory cache:", vLayout);
            p.widgets.push_back(CacheSettingsWidget::create(context, settings, vLayout));
            vLayout = ftk::VerticalLayout::create(context, p.stackLayout);
            vLayout->setSpacingRole(ftk::SizeRole::SpacingLarge);
            ftk::Label::create(context, "Configure the time settings:", vLayout);
            p.widgets.push_back(TimeSettingsWidget::create(context, timeUnitsModel, vLayout));

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::None);
            label->setParent(p.layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            p.stackLayout->setParent(p.layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setMarginRole(ftk::SizeRole::Margin);
            p.prevButton->setParent(hLayout);
            p.nextButton->setParent(hLayout);
            hLayout->addSpacer(ftk::SizeRole::Spacing, ftk::Stretch::Expanding);
            p.closeButton->setParent(hLayout);

            p.nextButton->setClickedCallback(
                [this]
                {
                    _p->stackLayout->nextIndex();
                });

            p.prevButton->setClickedCallback(
                [this]
                {
                    _p->stackLayout->prevIndex();
                });

            p.closeButton->setClickedCallback(
                [this]
                {
                    close();
                });

            p.nextObserver = ftk::Observer<bool>::create(
                p.stackLayout->observeHasNextIndex(),
                [this](bool value)
                {
                    _p->nextButton->setEnabled(value);
                });

            p.prevObserver = ftk::Observer<bool>::create(
                p.stackLayout->observeHasPrevIndex(),
                [this](bool value)
                {
                    _p->prevButton->setEnabled(value);
                });
        }

        SetupDialog::SetupDialog() :
            _p(new Private)
        {}

        SetupDialog::~SetupDialog()
        {}

        std::shared_ptr<SetupDialog> SetupDialog::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SetupDialog>(new SetupDialog);
            out->_init(context, settings, timeUnitsModel, parent);
            return out;
        }
    }
}
