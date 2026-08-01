// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/SaveReviewDialog.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>

namespace djv
{
    namespace app
    {
        //! Inner widget, following the pattern of ftk::ConfirmDialog.
        class SaveReviewDialogWidget : public ftk::IMouseWidget
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::string& title,
                const std::string& text,
                const std::shared_ptr<IWidget>& parent);

            SaveReviewDialogWidget();

        public:
            virtual ~SaveReviewDialogWidget();

            static std::shared_ptr<SaveReviewDialogWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::string& title,
                const std::string& text,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setCallback(const std::function<void(SaveReviewResult)>&);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;

        private:
            std::shared_ptr<ftk::Label> _titleLabel;
            std::shared_ptr<ftk::Label> _label;
            std::shared_ptr<ftk::ScrollWidget> _scrollWidget;
            std::shared_ptr<ftk::PushButton> _saveButton;
            std::shared_ptr<ftk::PushButton> _discardButton;
            std::shared_ptr<ftk::PushButton> _cancelButton;
            std::shared_ptr<ftk::VerticalLayout> _layout;
            std::function<void(SaveReviewResult)> _callback;
            int _sizeHint = 0;
        };

        void SaveReviewDialogWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::string& title,
            const std::string& text,
            const std::shared_ptr<IWidget>& parent)
        {
            IMouseWidget::_init(context, "djv::app::SaveReviewDialogWidget", parent);

            _setMouseHoverEnabled(true);
            _setMousePressEnabled(true);

            _titleLabel = ftk::Label::create(context, title);
            _titleLabel->setFontSize(14);
            _titleLabel->setMarginRole(ftk::SizeRole::Margin);
            _titleLabel->setBackgroundRole(ftk::ColorRole::Header);

            _label = ftk::Label::create(context, text);
            _label->setMarginRole(ftk::SizeRole::Margin);
            _label->setAlign(ftk::HAlign::Left, ftk::VAlign::Top);

            _scrollWidget = ftk::ScrollWidget::create(context);
            _scrollWidget->setBorder(false);
            _scrollWidget->setSizeHintRole(ftk::SizeRole::ScrollAreaSmall);
            _scrollWidget->setWidget(_label);

            _saveButton = ftk::PushButton::create(context, "Save");
            _discardButton = ftk::PushButton::create(context, "Don't Save");
            _cancelButton = ftk::PushButton::create(context, "Cancel");

            _layout = ftk::VerticalLayout::create(context, shared_from_this());
            _layout->setSpacingRole(ftk::SizeRole::None);
            _titleLabel->setParent(_layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, _layout);
            _scrollWidget->setParent(_layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, _layout);
            auto hLayout = ftk::HorizontalLayout::create(context, _layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            hLayout->addSpacer(ftk::SizeRole::None, ftk::Stretch::Expanding);
            _saveButton->setParent(hLayout);
            _discardButton->setParent(hLayout);
            _cancelButton->setParent(hLayout);

            _saveButton->setClickedCallback(
                [this]
                {
                    if (_callback)
                    {
                        _callback(SaveReviewResult::Save);
                    }
                });
            _discardButton->setClickedCallback(
                [this]
                {
                    if (_callback)
                    {
                        _callback(SaveReviewResult::Discard);
                    }
                });
            _cancelButton->setClickedCallback(
                [this]
                {
                    if (_callback)
                    {
                        _callback(SaveReviewResult::Cancel);
                    }
                });
        }

        SaveReviewDialogWidget::SaveReviewDialogWidget()
        {}

        SaveReviewDialogWidget::~SaveReviewDialogWidget()
        {}

        std::shared_ptr<SaveReviewDialogWidget> SaveReviewDialogWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::string& title,
            const std::string& text,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SaveReviewDialogWidget>(new SaveReviewDialogWidget);
            out->_init(context, title, text, parent);
            return out;
        }

        void SaveReviewDialogWidget::setCallback(const std::function<void(SaveReviewResult)>& value)
        {
            _callback = value;
        }

        ftk::Size2I SaveReviewDialogWidget::getSizeHint() const
        {
            ftk::Size2I out = _layout->getSizeHint();
            out.w = std::max(out.w, _sizeHint * 2);
            return out;
        }

        void SaveReviewDialogWidget::setGeometry(const ftk::Box2I& value)
        {
            IMouseWidget::setGeometry(value);
            _layout->setGeometry(value);
        }

        void SaveReviewDialogWidget::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            IMouseWidget::sizeHintEvent(event);
            _sizeHint = event.style->getSizeRole(ftk::SizeRole::ScrollArea, event.displayScale);
        }

        struct SaveReviewDialog::Private
        {
            std::shared_ptr<SaveReviewDialogWidget> widget;
            std::function<void(SaveReviewResult)> callback;
        };

        void SaveReviewDialog::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::string& title,
            const std::string& text,
            const std::shared_ptr<IWidget>& parent)
        {
            IDialog::_init(context, "djv::app::SaveReviewDialog", parent);
            FTK_P();

            p.widget = SaveReviewDialogWidget::create(context, title, text, shared_from_this());

            p.widget->setCallback(
                [this](SaveReviewResult value)
                {
                    if (_p->callback)
                    {
                        _p->callback(value);
                    }
                });
        }

        SaveReviewDialog::SaveReviewDialog() :
            _p(new Private)
        {}

        SaveReviewDialog::~SaveReviewDialog()
        {}

        std::shared_ptr<SaveReviewDialog> SaveReviewDialog::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::string& title,
            const std::string& text,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SaveReviewDialog>(new SaveReviewDialog);
            out->_init(context, title, text, parent);
            return out;
        }

        void SaveReviewDialog::setCallback(const std::function<void(SaveReviewResult)>& value)
        {
            _p->callback = value;
        }
    }
}
