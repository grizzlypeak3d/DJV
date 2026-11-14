// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/DisplayColorWidget.h>

#include <ftk/UI/ComboBox.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/Core/Format.h>

#include <algorithm>

namespace djv
{
    namespace ui
    {
        struct DisplayColorWidget::Private
        {
            models::DisplayColorLists lists;
            models::DisplayColor color;
            std::string resolvedInput;
            bool ocioEnabled = true;
            std::function<void(const models::DisplayColor&)> callback;

            std::shared_ptr<ftk::ComboBox> displayComboBox;
            std::shared_ptr<ftk::ComboBox> viewComboBox;
            std::shared_ptr<ftk::Label> resolvedLabel;
            std::shared_ptr<ftk::Label> offLabel;
            std::shared_ptr<ftk::FormLayout> layout;
        };

        void DisplayColorWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::string& what,
            const std::shared_ptr<IWidget>& parent)
        {
            ftk::IContainer::_init(context, "djv::ui::DisplayColorWidget", parent);
            FTK_P();

            p.displayComboBox = ftk::ComboBox::create(context);
            p.displayComboBox->setHStretch(ftk::Stretch::Expanding);
            p.displayComboBox->setTooltip(ftk::Format(
                "The OpenColorIO display {0} is rendered for.\n"
                "The viewport's unless another is chosen: a PQ display\n"
                "for an HDR monitor, while the viewport stays as it is.").
                arg(what));
            p.viewComboBox = ftk::ComboBox::create(context);
            p.viewComboBox->setHStretch(ftk::Stretch::Expanding);
            p.viewComboBox->setTooltip(ftk::Format(
                "The OpenColorIO view of {0}'s display.").arg(what));

            p.resolvedLabel = ftk::Label::create(context);
            p.resolvedLabel->setTooltip(ftk::Format(
                "The input color space {0} renders from,\n"
                "and where it came from.").arg(what));

            std::string note = ftk::Format(
                "OpenColorIO is off, so {0} follows the viewport unmanaged. "
                "Picking a display turns it on for {1} alone.").arg(what).arg(what);
            p.offLabel = ftk::Label::create(context, note);
            p.offLabel->setWrapToFit(true);
            p.offLabel->setWrap(40);

            p.layout = ftk::FormLayout::create(context);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Display:", p.displayComboBox);
            p.layout->addRow("View:", p.viewComboBox);
            p.layout->addRow("Resolved:", p.resolvedLabel);
            p.layout->addRow(std::string(), p.offLabel);
            _setWidget(p.layout);

            // The first item is the viewport's display, which is no display
            // of its own; the view stays as it was, and is checked against
            // the new display's (see models::validateDisplayColor()).
            p.displayComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    models::DisplayColor color = p.color;
                    color.display =
                        value > 0 && value <= static_cast<int>(p.lists.displays.size()) ?
                        p.lists.displays[value - 1] :
                        std::string();
                    if (p.callback)
                    {
                        p.callback(color);
                    }
                });
            p.viewComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    models::DisplayColor color = p.color;
                    if (value >= 0 && value < static_cast<int>(p.lists.views.size()))
                    {
                        color.view = p.lists.views[value];
                    }
                    if (p.callback)
                    {
                        p.callback(color);
                    }
                });

            _widgetUpdate();
        }

        DisplayColorWidget::DisplayColorWidget() :
            _p(new Private)
        {}

        DisplayColorWidget::~DisplayColorWidget()
        {}

        std::shared_ptr<DisplayColorWidget> DisplayColorWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::string& what,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<DisplayColorWidget>(new DisplayColorWidget);
            out->_init(context, what, parent);
            return out;
        }

        void DisplayColorWidget::setLists(const models::DisplayColorLists& value)
        {
            FTK_P();
            if (value == p.lists)
                return;
            p.lists = value;
            std::vector<std::string> items = { "Same as viewport" };
            items.insert(items.end(), p.lists.displays.begin(), p.lists.displays.end());
            p.displayComboBox->setItems(items);
            p.viewComboBox->setItems(p.lists.views);
            _widgetUpdate();
        }

        void DisplayColorWidget::setColor(const models::DisplayColor& value)
        {
            FTK_P();
            if (value == p.color)
                return;
            p.color = value;
            _widgetUpdate();
        }

        void DisplayColorWidget::setResolvedInput(const std::string& value)
        {
            FTK_P();
            if (value == p.resolvedInput)
                return;
            p.resolvedInput = value;
            _widgetUpdate();
        }

        void DisplayColorWidget::setOCIOEnabled(bool value)
        {
            FTK_P();
            if (value == p.ocioEnabled)
                return;
            p.ocioEnabled = value;
            _widgetUpdate();
        }

        void DisplayColorWidget::setCallback(const std::function<void(const models::DisplayColor&)>& value)
        {
            _p->callback = value;
        }

        void DisplayColorWidget::_widgetUpdate()
        {
            FTK_P();
            const auto display = std::find(
                p.lists.displays.begin(), p.lists.displays.end(), p.color.display);
            const bool own = display != p.lists.displays.end();
            p.displayComboBox->setCurrentIndex(
                own ? static_cast<int>(display - p.lists.displays.begin()) + 1 : 0);
            const auto view = std::find(
                p.lists.views.begin(), p.lists.views.end(), p.color.view);
            p.viewComboBox->setCurrentIndex(
                view != p.lists.views.end() ?
                static_cast<int>(view - p.lists.views.begin()) :
                (p.lists.views.empty() ? -1 : 0));
            // The viewport's view goes with the viewport's display.
            p.viewComboBox->setEnabled(own);
            p.resolvedLabel->setText(p.resolvedInput);
            p.layout->setRowVisible(
                p.resolvedLabel,
                !p.resolvedInput.empty() && (p.ocioEnabled || own));
            p.layout->setRowVisible(p.offLabel, !p.ocioEnabled && !own);
        }
    }
}
