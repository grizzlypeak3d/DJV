// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/DetectCutsDialog.h>

#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/LineEdit.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Spacer.h>

namespace djv
{
    namespace ui
    {
        struct DetectCutsDialog::Private
        {
            models::CutMarkerOptions options;
            std::shared_ptr<ftk::LineEdit> nameEdit;
            std::shared_ptr<ftk::ColorSwatch> colorSwatch;
            std::shared_ptr<ftk::PushButton> detectButton;
            std::function<void(const models::CutMarkerOptions&)> callback;
        };

        void DetectCutsDialog::_init(
            const std::shared_ptr<ftk::Context>& context,
            const models::CutMarkerOptions& options,
            const std::shared_ptr<IWidget>& parent)
        {
            IDialog::_init(context, "djv::ui::DetectCutsDialog", parent);
            FTK_P();
            setTitle("Detect Cuts");
            p.options = options;

            auto layout = ftk::VerticalLayout::create(context, shared_from_this());
            layout->setSpacingRole(ftk::SizeRole::None);

            auto contents = ftk::VerticalLayout::create(context, layout);
            contents->setMarginRole(ftk::SizeRole::Margin);
            contents->setSpacingRole(ftk::SizeRole::Spacing);
            auto label = ftk::Label::create(
                context,
                "Reads the frames over the in/out range and adds a one-frame "
                "marker at each cut found, except where there is a marker "
                "already.",
                contents);
            label->setWrap(40);
            label->setWrapToFit(true);

            p.nameEdit = ftk::LineEdit::create(context);
            p.nameEdit->setText(options.name);
            p.nameEdit->setPlaceholder("None");
            p.nameEdit->setTooltip("The markers' name. Without one, a marker shows its frame.");
            ftk::setScreenshotTag(p.nameEdit, "DetectCuts.Name");
            p.colorSwatch = ftk::ColorSwatch::create(context);
            p.colorSwatch->setColor(options.color);
            p.colorSwatch->setEditable(true);
            // A swatch, as the drawing color's, not a bar across the row.
            p.colorSwatch->setSizeRole(ftk::SizeRole::MarginLarge);
            p.colorSwatch->setHAlign(ftk::HAlign::Left);
            p.colorSwatch->setTooltip("The markers' color.");
            ftk::setScreenshotTag(p.colorSwatch, "DetectCuts.Color");
            auto formLayout = ftk::FormLayout::create(context, contents);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Name:", p.nameEdit);
            formLayout->addRow("Color:", p.colorSwatch);

            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            auto buttons = ftk::HorizontalLayout::create(context, layout);
            buttons->setMarginRole(ftk::SizeRole::MarginSmall);
            buttons->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto spacer = ftk::Spacer::create(context, ftk::Orientation::Horizontal, buttons);
            spacer->setHStretch(ftk::Stretch::Expanding);
            p.detectButton = ftk::PushButton::create(context, "Detect", buttons);
            ftk::setScreenshotTag(p.detectButton, "DetectCuts.Detect");
            auto cancelButton = ftk::PushButton::create(context, "Cancel", buttons);

            p.nameEdit->setTextChangedCallback(
                [this](const std::string& value)
                {
                    _p->options.name = value;
                });
            p.colorSwatch->setCallback(
                [this](const ftk::Color4F& value)
                {
                    _p->options.color = value;
                });
            p.detectButton->setClickedCallback(
                [this]
                {
                    if (_p->callback)
                    {
                        _p->callback(_p->options);
                    }
                });
            cancelButton->setClickedCallback(
                [this]
                {
                    close();
                });
        }

        DetectCutsDialog::DetectCutsDialog() :
            _p(new Private)
        {}

        DetectCutsDialog::~DetectCutsDialog()
        {}

        std::shared_ptr<DetectCutsDialog> DetectCutsDialog::create(
            const std::shared_ptr<ftk::Context>& context,
            const models::CutMarkerOptions& options,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<DetectCutsDialog>(new DetectCutsDialog);
            out->_init(context, options, parent);
            return out;
        }

        void DetectCutsDialog::setCallback(
            const std::function<void(const models::CutMarkerOptions&)>& value)
        {
            _p->callback = value;
        }

        std::shared_ptr<ftk::IWidget> DetectCutsDialog::getKeyFocus() const
        {
            // Return runs it: the options are usually the ones remembered.
            return _p->detectButton;
        }
    }
}
