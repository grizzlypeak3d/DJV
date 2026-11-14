// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/DisplaysTool.h>

#include <djv/App/App.h>

#include <djv/Models/ColorModel.h>
#include <djv/Models/DisplaysModel.h>
#include <djv/Models/ViewportModel.h>

#include <djv/UI/DisplayColorWidget.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/FloatEdit.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#if defined(FTK_GPU)
#include <ftk/GPU/System.h>
#endif // FTK_GPU

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

            std::vector<tl::HDR_EOTF> hdrTransfers;
            std::shared_ptr<ftk::ComboBox> hdrTransferComboBox;
            std::shared_ptr<ftk::FloatEdit> hdrWhiteEdit;

            std::shared_ptr<ftk::CheckBox> secondaryCheckBox;
            std::shared_ptr<ftk::ComboBox> monitorComboBox;
            std::vector<std::string> monitorItems;
            std::shared_ptr<ui::DisplayColorWidget> secondaryColorWidget;
            std::shared_ptr<ftk::Label> secondaryHDRLabel;
            std::optional<tl::HDR_EOTF> secondaryHDRTransfer;

            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioObserver;
            std::shared_ptr<ftk::Observer<tl::HDR_EOTF> > hdrTransferObserver;
            std::shared_ptr<ftk::Observer<float> > hdrWhiteObserver;
            std::shared_ptr<ftk::Observer<bool> > secondaryObserver;
            std::shared_ptr<ftk::Observer<models::SecondaryDisplay> > secondaryDisplayObserver;
            std::shared_ptr<ftk::Observer<models::DisplayColorLists> > secondaryListsObserver;
            std::shared_ptr<ftk::Observer<std::optional<tl::HDR_EOTF> > > secondaryHDRObserver;
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

            // The viewport. Its color is the Color tool's; what is here is
            // whether the picture is HDR, and how bright the interface's
            // white is beside it: what the display shows, so here with the
            // display, not with how the picture is looked at. Gamma HDR is
            // not offered: nothing here draws it.
            p.hdrTransfers = { tl::HDR_EOTF::SDR, tl::HDR_EOTF::ST2084 };
            p.hdrTransferComboBox = ftk::ComboBox::create(
                context,
                std::vector<std::string>({ "SDR", "PQ" }));
            p.hdrTransferComboBox->setHStretch(ftk::Stretch::Expanding);
            p.hdrTransferComboBox->setTooltip(
                "What the picture is display encoded for. Choose PQ for a PQ "
                "encoded picture with OpenColorIO off, or with an HDR display "
                "such as Rec.2100-PQ: on an HDR display the picture is then "
                "shown as HDR. The secondary window follows this unless it "
                "has a display of its own. Windows are HDR only with the GPU "
                "renderer.");
            ftk::setScreenshotTag(p.hdrTransferComboBox, "Displays.HDRTransfer");
            p.hdrWhiteEdit = ftk::FloatEdit::create(context);
            p.hdrWhiteEdit->setRange(80.F, 1000.F);
            p.hdrWhiteEdit->setStep(1.F);
            p.hdrWhiteEdit->setLargeStep(10.F);
            p.hdrWhiteEdit->setPrecision(0);
            p.hdrWhiteEdit->setDefault(203.F);
            p.hdrWhiteEdit->setTooltip(
                "The luminance, in nits, that the window's white stands for "
                "when a PQ picture is shown, where the system does not say. "
                "203 is the reference white for HDR.");
            ftk::setScreenshotTag(p.hdrWhiteEdit, "Displays.HDRWhite");
#if defined(FTK_GPU)
            const bool hdr = ftk::gpu::isEnabled();
#else // FTK_GPU
            const bool hdr = false;
#endif // FTK_GPU
            p.hdrTransferComboBox->setEnabled(hdr);
            p.hdrWhiteEdit->setEnabled(hdr);
            auto hdrLayout = ftk::FormLayout::create(context);
            hdrLayout->setMarginRole(ftk::SizeRole::Margin);
            hdrLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            hdrLayout->addRow("HDR picture:", p.hdrTransferComboBox);
            hdrLayout->addRow("HDR white:", p.hdrWhiteEdit);

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
            // Whether it is HDR, which follows from its display or the
            // viewport's setting: said, so a picture that is HDR on the
            // television and not on the desktop reads as meant.
            p.secondaryHDRLabel = ftk::Label::create(context);
            p.secondaryHDRLabel->setHStretch(ftk::Stretch::Expanding);
            p.secondaryHDRLabel->setTooltip(
                "Whether the window shows the picture as HDR: from its own "
                "display when it has one, a PQ display being HDR, or else as "
                "the viewport's HDR picture.");
            p.secondaryHDRLabel->setEnabled(hdr);
            ftk::setScreenshotTag(p.secondaryHDRLabel, "Displays.SecondaryHDR");
            auto secondaryHDRLayout = ftk::FormLayout::create(context, secondaryLayout);
            secondaryHDRLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            secondaryHDRLayout->addRow("HDR:", p.secondaryHDRLabel);

            p.layout = ftk::VerticalLayout::create(context);
            p.layout->setSpacingRole(ftk::SizeRole::Border);
            p.bellows["Viewport"] = ftk::Bellows::create(context, "Viewport", p.layout);
            p.bellows["Viewport"]->setWidget(hdrLayout);
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
            std::weak_ptr<models::ViewportModel> viewportModelWeak(app->getViewportModel());
            p.hdrTransferComboBox->setIndexCallback(
                [this, viewportModelWeak](int value)
                {
                    FTK_P();
                    auto viewportModel = viewportModelWeak.lock();
                    if (viewportModel &&
                        value >= 0 &&
                        value < static_cast<int>(p.hdrTransfers.size()))
                    {
                        viewportModel->setHDRTransfer(p.hdrTransfers[value]);
                    }
                });
            p.hdrWhiteEdit->setCallback(
                [viewportModelWeak](float value)
                {
                    if (auto viewportModel = viewportModelWeak.lock())
                    {
                        viewportModel->setHDRWhite(value);
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
                    _p->secondaryColorWidget->setOCIOEnabled(value.enabled);
                });
            p.hdrTransferObserver = ftk::Observer<tl::HDR_EOTF>::create(
                app->getViewportModel()->observeHDRTransfer(),
                [this](tl::HDR_EOTF value)
                {
                    FTK_P();
                    const auto i = std::find(p.hdrTransfers.begin(), p.hdrTransfers.end(), value);
                    p.hdrTransferComboBox->setCurrentIndex(
                        i != p.hdrTransfers.end() ? static_cast<int>(i - p.hdrTransfers.begin()) : 0);
                    _secondaryHDRUpdate();
                });
            p.secondaryHDRObserver = ftk::Observer<std::optional<tl::HDR_EOTF> >::create(
                app->getDisplaysModel()->observeSecondaryHDRTransfer(),
                [this](const std::optional<tl::HDR_EOTF>& value)
                {
                    _p->secondaryHDRTransfer = value;
                    _secondaryHDRUpdate();
                });
            p.hdrWhiteObserver = ftk::Observer<float>::create(
                app->getViewportModel()->observeHDRWhite(),
                [this](float value)
                {
                    _p->hdrWhiteEdit->setValue(value);
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

        void DisplaysTool::_secondaryHDRUpdate()
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app || !p.secondaryHDRLabel)
                return;
            auto label = [](tl::HDR_EOTF value)
            {
                return tl::HDR_EOTF::ST2084 == value ? std::string("PQ") : std::string("SDR");
            };
            p.secondaryHDRLabel->setText(p.secondaryHDRTransfer.has_value() ?
                label(*p.secondaryHDRTransfer) + ", from its display" :
                label(app->getViewportModel()->observeHDRTransfer()->get()) + ", as the viewport");
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
