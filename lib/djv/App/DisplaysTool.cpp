// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/DisplaysTool.h>

#include <djv/App/App.h>

#include <djv/Models/ColorModel.h>
#include <djv/Models/DisplaysModel.h>
#include <djv/Models/ToolsModel.h>

#include <djv/UI/DisplayColorWidget.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/Core/Format.h>

#include <algorithm>

namespace djv
{
    namespace app
    {
        struct DisplaysTool::Private
        {
            std::weak_ptr<App> app;

            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
            std::shared_ptr<ftk::VerticalLayout> layout;
            std::shared_ptr<ftk::FormGroup> formGroup;

            std::shared_ptr<ftk::Label> viewportLabel;
            std::shared_ptr<ftk::PushButton> colorToolButton;

            std::shared_ptr<ftk::CheckBox> secondaryCheckBox;
            std::shared_ptr<ftk::ComboBox> monitorComboBox;
            std::vector<std::string> monitorItems;
            std::shared_ptr<ui::DisplayColorWidget> secondaryColorWidget;

            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioObserver;
            std::shared_ptr<ftk::Observer<bool> > secondaryObserver;
            std::shared_ptr<ftk::Observer<models::SecondaryDisplay> > secondaryDisplayObserver;
            std::shared_ptr<ftk::Observer<models::DisplayColorLists> > secondaryListsObserver;
            std::shared_ptr<ftk::ListObserver<ftk::MonitorInfo> > monitorsObserver;
        };

        void DisplaysTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                mainWindow,
                "Displays",
                "Displays",
                "djv::app::DisplaysTool",
                parent);
            FTK_P();
            p.app = app;

            // The viewport: its color is the Color tool's, said here and
            // set there, so there is one place to set it.
            p.viewportLabel = ftk::Label::create(context);
            p.viewportLabel->setHStretch(ftk::Stretch::Expanding);
            p.colorToolButton = ftk::PushButton::create(context, "Color Tool");
            p.colorToolButton->setTooltip("Set the viewport's color in the Color tool.");
            auto viewportLayout = ftk::VerticalLayout::create(context);
            viewportLayout->setMarginRole(ftk::SizeRole::Margin);
            viewportLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.viewportLabel->setParent(viewportLayout);
            auto buttonLayout = ftk::HorizontalLayout::create(context, viewportLayout);
            buttonLayout->addSpacer(ftk::Stretch::Expanding);
            p.colorToolButton->setParent(buttonLayout);

            // The secondary window: on or off in the header, as an output
            // is, and a monitor to fill, and a color of its own.
            p.secondaryCheckBox = ftk::CheckBox::create(context);
            p.secondaryCheckBox->setTooltip(
                "Open the secondary window: a second view of the picture.");
            p.monitorComboBox = ftk::ComboBox::create(context);
            p.monitorComboBox->setHStretch(ftk::Stretch::Expanding);
            p.monitorComboBox->setTooltip(
                "Where the secondary window is: a window of its own, or\n"
                "filling a monitor with the picture alone. Escape in it\n"
                "gives the window back.");
            p.secondaryColorWidget = ui::DisplayColorWidget::create(context, "the window");
            ftk::setScreenshotTag(p.secondaryCheckBox, "Displays.Secondary");
            ftk::setScreenshotTag(p.monitorComboBox, "Displays.Monitor");
            ftk::setScreenshotTag(p.secondaryColorWidget, "Displays.SecondaryColor");
            auto secondaryLayout = ftk::VerticalLayout::create(context);
            secondaryLayout->setMarginRole(ftk::SizeRole::Margin);
            secondaryLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto monitorLayout = ftk::FormLayout::create(context, secondaryLayout);
            monitorLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            monitorLayout->addRow("Monitor:", p.monitorComboBox);
            p.secondaryColorWidget->setParent(secondaryLayout);

            p.layout = ftk::VerticalLayout::create(context);
            p.layout->setSpacingRole(ftk::SizeRole::Border);
            p.bellows["Viewport"] = ftk::Bellows::create(context, "Viewport", p.layout);
            p.bellows["Viewport"]->setWidget(viewportLayout);
            p.bellows["Secondary Window"] = ftk::Bellows::create(context, "Secondary Window", p.layout);
            p.bellows["Secondary Window"]->setWidget(secondaryLayout);
            p.bellows["Secondary Window"]->setToolWidget(p.secondaryCheckBox);
            p.bellows["Viewport"]->setOpen(true);
            p.bellows["Secondary Window"]->setOpen(true);
            // The sections' labels share one width, so they line up as one
            // form would; sections added later join it.
            p.formGroup = ftk::FormGroup::create();
            ftk::setFormGroup(p.layout, p.formGroup);
            _setWidget(p.layout);

            std::weak_ptr<App> appWeak(app);
            p.colorToolButton->setClickedCallback(
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getToolsModel()->showSection("Color", "OCIO");
                    }
                });
            p.secondaryCheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->setSecondaryWindow(value);
                    }
                });
            p.monitorComboBox->setIndexCallback(
                [this, appWeak](int value)
                {
                    FTK_P();
                    if (auto app = appWeak.lock())
                    {
                        app->getDisplaysModel()->setSecondaryMonitor(
                            value > 0 && value < static_cast<int>(p.monitorItems.size()) ?
                            p.monitorItems[value] :
                            std::string());
                    }
                });
            p.secondaryColorWidget->setCallback(
                [appWeak](const models::DisplayColor& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getDisplaysModel()->setSecondaryColor(value);
                    }
                });

            p.ocioObserver = ftk::Observer<tl::OCIOOptions>::create(
                app->getColorModel()->observeResolvedOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                    FTK_P();
                    p.viewportLabel->setText(value.enabled ?
                        ftk::Format("OpenColorIO: {0}, {1}").
                            arg(value.display).arg(value.view).str() :
                        std::string("OpenColorIO is off."));
                    p.secondaryColorWidget->setOCIOEnabled(value.enabled);
                });
            p.secondaryObserver = ftk::Observer<bool>::create(
                app->observeSecondaryWindow(),
                [this](bool value)
                {
                    _p->secondaryCheckBox->setChecked(value);
                });
            p.secondaryDisplayObserver = ftk::Observer<models::SecondaryDisplay>::create(
                app->getDisplaysModel()->observeSecondary(),
                [this](const models::SecondaryDisplay& value)
                {
                    _p->secondaryColorWidget->setColor(value.color);
                    _monitorsUpdate();
                });
            p.secondaryListsObserver = ftk::Observer<models::DisplayColorLists>::create(
                app->getDisplaysModel()->observeSecondaryLists(),
                [this](const models::DisplayColorLists& value)
                {
                    _p->secondaryColorWidget->setLists(value);
                });
            p.monitorsObserver = ftk::ListObserver<ftk::MonitorInfo>::create(
                app->observeMonitors(),
                [this](const std::vector<ftk::MonitorInfo>&)
                {
                    _monitorsUpdate();
                });

            _loadSettings(p.bellows);
        }

        DisplaysTool::DisplaysTool() :
            _p(new Private)
        {}

        DisplaysTool::~DisplaysTool()
        {
            _saveSettings(_p->bellows);
        }

        std::shared_ptr<DisplaysTool> DisplaysTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<DisplaysTool>(new DisplaysTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }

        void DisplaysTool::addSection(
            const std::string& title,
            const std::shared_ptr<IWidget>& widget,
            const std::shared_ptr<IWidget>& headerWidget)
        {
            FTK_P();
            if (auto context = getContext())
            {
                auto bellows = ftk::Bellows::create(context, title, p.layout);
                auto layout = ftk::VerticalLayout::create(context);
                layout->setMarginRole(ftk::SizeRole::Margin);
                widget->setParent(layout);
                bellows->setWidget(layout);
                if (headerWidget)
                {
                    bellows->setToolWidget(headerWidget);
                }
                bellows->setOpen(true);
                p.bellows[title] = bellows;
                ftk::setFormGroup(p.layout, p.formGroup);
                _loadSettings(p.bellows);
            }
        }

        void DisplaysTool::_monitorsUpdate()
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return;
            // "Window", then the monitors by name. One chosen that is not
            // connected is listed as such, so the choice reads as it is.
            const std::string chosen = app->getDisplaysModel()->getSecondary().monitor;
            std::vector<std::string> items = { std::string() };
            std::vector<std::string> labels = { "Window" };
            bool found = false;
            for (const auto& monitor : app->observeMonitors()->get())
            {
                items.push_back(monitor.name);
                labels.push_back(monitor.name);
                found |= monitor.name == chosen;
            }
            if (!chosen.empty() && !found)
            {
                items.push_back(chosen);
                labels.push_back(chosen + " (not connected)");
            }
            p.monitorItems = items;
            p.monitorComboBox->setItems(labels);
            const auto i = std::find(items.begin(), items.end(), chosen);
            p.monitorComboBox->setCurrentIndex(
                i != items.end() ? static_cast<int>(i - items.begin()) : 0);
        }
    }
}
