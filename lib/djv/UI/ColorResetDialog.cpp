// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/ColorResetDialog.h>

#include <ftk/UI/CheckBox.h>

#include <array>
#include <ftk/UI/Divider.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/Spacer.h>

namespace djv
{
    namespace ui
    {
        namespace
        {
            class ColorResetWidget : public ftk::IMouseWidget
            {
            protected:
                void _init(
                    const std::shared_ptr<ftk::Context>& context,
                    const ColorResetGroups& set,
                    const std::shared_ptr<IWidget>& parent)
                {
                    IMouseWidget::_init(context, "djv::ui::ColorResetWidget", parent);

                    _setMouseHoverEnabled(true);
                    _setMousePressEnabled(true);

                    auto label = ftk::Label::create(
                        context,
                        "Reset these color settings to their defaults:");
                    label->setAlign(ftk::HAlign::Left, ftk::VAlign::Center);

                    // The same sections, in the same order, as the color
                    // tool.
                    _checkBoxes.push_back(ftk::CheckBox::create(context, "OCIO"));
                    _checkBoxes.push_back(ftk::CheckBox::create(context, "LUT"));
                    _checkBoxes.push_back(ftk::CheckBox::create(context, "Color"));
                    _checkBoxes.push_back(ftk::CheckBox::create(context, "Levels"));
                    _checkBoxes[0]->setTooltip(
                        "The configuration, the color spaces, and the extension assignments.");
                    _checkBoxes[2]->setTooltip(
                        "The color controls, exposure, and soft clip.");

                    _resetButton = ftk::PushButton::create(context, "Reset");
                    _cancelButton = ftk::PushButton::create(context, "Cancel");

                    _layout = ftk::VerticalLayout::create(context, shared_from_this());
                    _layout->setSpacingRole(ftk::SizeRole::None);
                    auto vLayout = ftk::VerticalLayout::create(context, _layout);
                    vLayout->setMarginRole(ftk::SizeRole::Margin);
                    vLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
                    label->setParent(vLayout);
                    // Ticked where there is something to reset. A section
                    // already at its default cannot be ticked: resetting it
                    // would do nothing, and leaving it ticked said the
                    // opposite (DJV #687).
                    const std::array<bool, 4> setGroups =
                    {
                        set.ocio,
                        set.lut,
                        set.color,
                        set.levels
                    };
                    for (size_t i = 0; i < _checkBoxes.size(); ++i)
                    {
                        _checkBoxes[i]->setChecked(setGroups[i]);
                        _checkBoxes[i]->setEnabled(setGroups[i]);
                        if (!setGroups[i])
                        {
                            _checkBoxes[i]->setTooltip("Already at its defaults.");
                        }
                        _checkBoxes[i]->setParent(vLayout);
                    }
                    ftk::Divider::create(context, ftk::Orientation::Vertical, _layout);
                    auto hLayout = ftk::HorizontalLayout::create(context, _layout);
                    hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
                    hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
                    auto spacer = ftk::Spacer::create(context, ftk::Orientation::Horizontal, hLayout);
                    spacer->setSpacingRole(ftk::SizeRole::None);
                    spacer->setHStretch(ftk::Stretch::Expanding);
                    _resetButton->setParent(hLayout);
                    _cancelButton->setParent(hLayout);

                    for (const auto& checkBox : _checkBoxes)
                    {
                        checkBox->setCheckedCallback(
                            [this](bool)
                            {
                                _widgetUpdate();
                            });
                    }

                    _resetButton->setClickedCallback(
                        [this]
                        {
                            if (_callback)
                            {
                                ColorResetGroups groups;
                                groups.ocio = _checkBoxes[0]->isChecked();
                                groups.lut = _checkBoxes[1]->isChecked();
                                groups.color = _checkBoxes[2]->isChecked();
                                groups.levels = _checkBoxes[3]->isChecked();
                                _callback(groups);
                            }
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

                ColorResetWidget() = default;

            public:
                static std::shared_ptr<ColorResetWidget> create(
                    const std::shared_ptr<ftk::Context>& context,
                    const ColorResetGroups& set,
                    const std::shared_ptr<IWidget>& parent)
                {
                    auto out = std::shared_ptr<ColorResetWidget>(new ColorResetWidget);
                    out->_init(context, set, parent);
                    return out;
                }

                void setCallback(const std::function<void(const ColorResetGroups&)>& value)
                {
                    _callback = value;
                }

                void setCancelCallback(const std::function<void(void)>& value)
                {
                    _cancelCallback = value;
                }

                std::shared_ptr<ftk::IWidget> getCancelButton() const
                {
                    return _cancelButton;
                }

                ftk::Size2I getSizeHint() const override
                {
                    return _layout->getSizeHint();
                }

                void setGeometry(const ftk::Box2I& value) override
                {
                    IMouseWidget::setGeometry(value);
                    _layout->setGeometry(value);
                }

            private:
                void _widgetUpdate()
                {
                    bool any = false;
                    for (const auto& checkBox : _checkBoxes)
                    {
                        any |= checkBox->isChecked();
                    }
                    _resetButton->setEnabled(any);
                }

                std::vector<std::shared_ptr<ftk::CheckBox> > _checkBoxes;
                std::shared_ptr<ftk::PushButton> _resetButton;
                std::shared_ptr<ftk::PushButton> _cancelButton;
                std::shared_ptr<ftk::VerticalLayout> _layout;
                std::function<void(const ColorResetGroups&)> _callback;
                std::function<void(void)> _cancelCallback;
            };
        }

        struct ColorResetDialog::Private
        {
            std::shared_ptr<ColorResetWidget> widget;
            std::function<void(const ColorResetGroups&)> callback;
        };

        void ColorResetDialog::_init(
            const std::shared_ptr<ftk::Context>& context,
            const ColorResetGroups& set,
            const std::shared_ptr<IWidget>& parent)
        {
            IDialog::_init(context, "djv::ui::ColorResetDialog", parent);
            FTK_P();

            setTitle("Reset Color");

            p.widget = ColorResetWidget::create(context, set, shared_from_this());

            p.widget->setCallback(
                [this](const ColorResetGroups& value)
                {
                    // Copied, since closing can release whoever set it.
                    auto callback = _p->callback;
                    close();
                    if (callback)
                    {
                        callback(value);
                    }
                });

            p.widget->setCancelCallback(
                [this]
                {
                    close();
                });
        }

        ColorResetDialog::ColorResetDialog() :
            _p(new Private)
        {}

        ColorResetDialog::~ColorResetDialog()
        {}

        std::shared_ptr<ColorResetDialog> ColorResetDialog::create(
            const std::shared_ptr<ftk::Context>& context,
            const ColorResetGroups& set,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ColorResetDialog>(new ColorResetDialog);
            out->_init(context, set, parent);
            return out;
        }

        void ColorResetDialog::setCallback(const std::function<void(const ColorResetGroups&)>& value)
        {
            _p->callback = value;
        }

        std::shared_ptr<ftk::IWidget> ColorResetDialog::getKeyFocus() const
        {
            return _p->widget->getCancelButton();
        }
    }
}
