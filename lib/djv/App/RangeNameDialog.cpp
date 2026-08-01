// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/RangeNameDialog.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/LineEdit.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>

namespace djv
{
    namespace app
    {
        //! Inner widget, following the pattern of SaveReviewDialog.
        class RangeNameDialogWidget : public ftk::IMouseWidget
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::string& title,
                const std::string& text,
                const std::string& name,
                const std::shared_ptr<IWidget>& parent);

            RangeNameDialogWidget();

        public:
            virtual ~RangeNameDialogWidget();

            static std::shared_ptr<RangeNameDialogWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::string& title,
                const std::string& text,
                const std::string& name,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setCallback(const std::function<void(const std::string&)>&);
            void setCancelCallback(const std::function<void(void)>&);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;

        private:
            void _accept();

            std::shared_ptr<ftk::Label> _titleLabel;
            std::shared_ptr<ftk::Label> _label;
            std::shared_ptr<ftk::LineEdit> _lineEdit;
            std::shared_ptr<ftk::PushButton> _okButton;
            std::shared_ptr<ftk::PushButton> _cancelButton;
            std::shared_ptr<ftk::VerticalLayout> _layout;
            std::function<void(const std::string&)> _callback;
            std::function<void(void)> _cancelCallback;
            int _sizeHint = 0;
        };

        void RangeNameDialogWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::string& title,
            const std::string& text,
            const std::string& name,
            const std::shared_ptr<IWidget>& parent)
        {
            IMouseWidget::_init(context, "djv::app::RangeNameDialogWidget", parent);

            _setMouseHoverEnabled(true);
            _setMousePressEnabled(true);

            _titleLabel = ftk::Label::create(context, title);
            _titleLabel->setFontSize(14);
            _titleLabel->setMarginRole(ftk::SizeRole::Margin);
            _titleLabel->setBackgroundRole(ftk::ColorRole::Header);

            _label = ftk::Label::create(context, text);
            _label->setMarginRole(ftk::SizeRole::MarginSmall);
            _label->setTextRole(ftk::ColorRole::TextDisabled);
            _label->setAlign(ftk::HAlign::Left, ftk::VAlign::Center);

            _lineEdit = ftk::LineEdit::create(context);
            _lineEdit->setText(name);

            _okButton = ftk::PushButton::create(context, "OK");
            _cancelButton = ftk::PushButton::create(context, "Cancel");

            _layout = ftk::VerticalLayout::create(context, shared_from_this());
            _layout->setSpacingRole(ftk::SizeRole::None);
            _titleLabel->setParent(_layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, _layout);
            auto vLayout = ftk::VerticalLayout::create(context, _layout);
            vLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            vLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            _label->setParent(vLayout);
            _lineEdit->setParent(vLayout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, _layout);
            auto hLayout = ftk::HorizontalLayout::create(context, _layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            hLayout->addSpacer(ftk::SizeRole::None, ftk::Stretch::Expanding);
            _okButton->setParent(hLayout);
            _cancelButton->setParent(hLayout);

            // Typing replaces the default, confirming keeps it.
            _lineEdit->takeKeyFocus();
            _lineEdit->selectAll();

            // Enter in the field is the same as pressing OK: naming a range is
            // a keyboard gesture, reaching for the mouse defeats the point.
            _lineEdit->setCallback(
                [this](const std::string&)
                {
                    _accept();
                });

            _okButton->setClickedCallback(
                [this]
                {
                    _accept();
                });
            _cancelButton->setClickedCallback(
                [this]
                {
                    if (_cancelCallback)
                    {
                        _cancelCallback();
                    }
                });
        }

        RangeNameDialogWidget::RangeNameDialogWidget()
        {}

        RangeNameDialogWidget::~RangeNameDialogWidget()
        {}

        std::shared_ptr<RangeNameDialogWidget> RangeNameDialogWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::string& title,
            const std::string& text,
            const std::string& name,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<RangeNameDialogWidget>(new RangeNameDialogWidget);
            out->_init(context, title, text, name, parent);
            return out;
        }

        void RangeNameDialogWidget::setCallback(const std::function<void(const std::string&)>& value)
        {
            _callback = value;
        }

        void RangeNameDialogWidget::setCancelCallback(const std::function<void(void)>& value)
        {
            _cancelCallback = value;
        }

        void RangeNameDialogWidget::_accept()
        {
            if (_callback)
            {
                _callback(_lineEdit->getText());
            }
        }

        ftk::Size2I RangeNameDialogWidget::getSizeHint() const
        {
            ftk::Size2I out = _layout->getSizeHint();
            out.w = std::max(out.w, _sizeHint * 2);
            return out;
        }

        void RangeNameDialogWidget::setGeometry(const ftk::Box2I& value)
        {
            IMouseWidget::setGeometry(value);
            _layout->setGeometry(value);
        }

        void RangeNameDialogWidget::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            IMouseWidget::sizeHintEvent(event);
            _sizeHint = event.style->getSizeRole(ftk::SizeRole::ScrollArea, event.displayScale);
        }

        struct RangeNameDialog::Private
        {
            std::shared_ptr<RangeNameDialogWidget> widget;
            std::function<void(const std::string&)> callback;
        };

        void RangeNameDialog::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::string& title,
            const std::string& text,
            const std::string& name,
            const std::shared_ptr<IWidget>& parent)
        {
            IDialog::_init(context, "djv::app::RangeNameDialog", parent);
            FTK_P();

            p.widget = RangeNameDialogWidget::create(
                context, title, text, name, shared_from_this());

            p.widget->setCallback(
                [this](const std::string& value)
                {
                    if (_p->callback)
                    {
                        _p->callback(value);
                    }
                });
            p.widget->setCancelCallback(
                [this]
                {
                    close();
                });
        }

        RangeNameDialog::RangeNameDialog() :
            _p(new Private)
        {}

        RangeNameDialog::~RangeNameDialog()
        {}

        std::shared_ptr<RangeNameDialog> RangeNameDialog::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::string& title,
            const std::string& text,
            const std::string& name,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<RangeNameDialog>(new RangeNameDialog);
            out->_init(context, title, text, name, parent);
            return out;
        }

        void RangeNameDialog::setCallback(const std::function<void(const std::string&)>& value)
        {
            _p->callback = value;
        }
    }
}
