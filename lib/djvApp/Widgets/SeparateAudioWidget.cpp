// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Widgets/SeparateAudioDialog.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/FileEdit.h>
#include <ftk/UI/GroupBox.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/Spacer.h>

namespace djv
{
    namespace app
    {
        struct SeparateAudioWidget::Private
        {
            std::shared_ptr<ftk::FileEdit> videoFileEdit;
            std::shared_ptr<ftk::FileEdit> audioFileEdit;
            std::shared_ptr<ftk::PushButton> okButton;
            std::shared_ptr<ftk::PushButton> cancelButton;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::function<void(const ftk::Path&, const ftk::Path&)> callback;
            std::function<void(void)> cancelCallback;
        };

        void SeparateAudioWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            IMouseWidget::_init(
                context,
                "djv::app::SeparateAudioWidget",
                parent);
            FTK_P();

            setHStretch(ftk::Stretch::Expanding);
            _setMouseHoverEnabled(true);
            _setMousePressEnabled(true);

            p.videoFileEdit = ftk::FileEdit::create(context);

            p.audioFileEdit = ftk::FileEdit::create(context);

            p.okButton = ftk::PushButton::create(context, "OK");
            p.cancelButton = ftk::PushButton::create(context, "Cancel");

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::None);
            auto label = ftk::Label::create(context, "Open Separate Audio", p.layout);
            label->setMarginRole(ftk::SizeRole::MarginSmall);
            label->setBackgroundRole(ftk::ColorRole::Button);
            auto vLayout = ftk::VerticalLayout::create(context, p.layout);
            vLayout->setVStretch(ftk::Stretch::Expanding);
            vLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            vLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto groupBox = ftk::GroupBox::create(context, "Video", vLayout);
            p.videoFileEdit->setParent(groupBox);
            groupBox = ftk::GroupBox::create(context, "Audio", vLayout);
            p.audioFileEdit->setParent(groupBox);
            auto spacer = ftk::Spacer::create(context, ftk::Orientation::Vertical, vLayout);
            spacer->setSpacingRole(ftk::SizeRole::None);
            spacer->setVStretch(ftk::Stretch::Expanding);
            ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            spacer = ftk::Spacer::create(context, ftk::Orientation::Horizontal, hLayout);
            spacer->setSpacingRole(ftk::SizeRole::None);
            spacer->setHStretch(ftk::Stretch::Expanding);
            p.okButton->setParent(hLayout);
            p.cancelButton->setParent(hLayout);

            p.okButton->setClickedCallback(
                [this]
                {
                    if (_p->callback)
                    {
                        _p->callback(
                            ftk::Path(_p->videoFileEdit->getPath()),
                            ftk::Path(_p->audioFileEdit->getPath()));
                    }
                });

            p.cancelButton->setClickedCallback(
                [this]
                {
                    if (_p->cancelCallback)
                    {
                        _p->cancelCallback();
                    }
                });
        }

        SeparateAudioWidget::SeparateAudioWidget() :
            _p(new Private)
        {}

        SeparateAudioWidget::~SeparateAudioWidget()
        {}

        std::shared_ptr<SeparateAudioWidget> SeparateAudioWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SeparateAudioWidget>(new SeparateAudioWidget);
            out->_init(context, parent);
            return out;
        }

        void SeparateAudioWidget::setCallback(const std::function<void(
            const ftk::Path&,
            const ftk::Path&)>& value)
        {
            _p->callback = value;
        }

        void SeparateAudioWidget::setCancelCallback(const std::function<void(void)>& value)
        {
            _p->cancelCallback = value;
        }

        ftk::Size2I SeparateAudioWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void SeparateAudioWidget::setGeometry(const ftk::Box2I& value)
        {
            IMouseWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }
    }
}
