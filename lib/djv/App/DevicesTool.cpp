// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/DevicesTool.h>

#include <djv/App/App.h>
#if defined(TLRENDER_BMD)
#include <djv/Models/BMDDevicesModel.h>
#endif // TLRENDER_BMD

#if defined(TLRENDER_BMD)
#include <tlRender/Device/BMDOutputDevice.h>
#endif // TLRENDER_BMD

#include <ftk/UI/Bellows.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/FloatEdit.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>

#include <sstream>

namespace djv
{
    namespace app
    {
        struct DevicesTool::Private
        {
#if defined(TLRENDER_BMD)
            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::shared_ptr<ftk::ComboBox> deviceComboBox;
            std::shared_ptr<ftk::ComboBox> displayModeComboBox;
            std::shared_ptr<ftk::ComboBox> pixelTypeComboBox;
            std::shared_ptr<ftk::CheckBox> _444SDIVideoOutputCheckBox;

            std::shared_ptr<ftk::Observer<tl::bmd::DevicesModelData> > dataObserver;
#endif // TLRENDER_BMD
        };

        void DevicesTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                models::Tool::Devices,
                "djv::app::DevicesTool",
                parent);
            FTK_P();

#if defined(TLRENDER_BMD)
            p.enabledCheckBox = ftk::CheckBox::create(context);

            p.deviceComboBox = ftk::ComboBox::create(context);
            p.deviceComboBox->setHStretch(ftk::Stretch::Expanding);
            p.displayModeComboBox = ftk::ComboBox::create(context);
            p.displayModeComboBox->setHStretch(ftk::Stretch::Expanding);
            p.pixelTypeComboBox = ftk::ComboBox::create(context);
            p.pixelTypeComboBox->setHStretch(ftk::Stretch::Expanding);

            p._444SDIVideoOutputCheckBox = ftk::CheckBox::create(context);

            auto formLayout = ftk::FormLayout::create(context);
            formLayout->setMarginRole(ftk::SizeRole::Margin);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Enabled:", p.enabledCheckBox);
            formLayout->addRow("Device:", p.deviceComboBox);
            formLayout->addRow("Display mode:", p.displayModeComboBox);
            formLayout->addRow("Pixel type:", p.pixelTypeComboBox);
            formLayout->addRow("444 SDI:", p._444SDIVideoOutputCheckBox);

            auto scrollWidget = ftk::ScrollWidget::create(context);
            scrollWidget->setBorder(false);
            scrollWidget->setWidget(formLayout);
            _setWidget(scrollWidget);

            auto appWeak = std::weak_ptr<App>(app);
            p.enabledCheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getBMDDevicesModel()->setDeviceEnabled(value);
                    }
                });

            p.deviceComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getBMDDevicesModel()->setDeviceIndex(value);
                    }
                });
            p.displayModeComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getBMDDevicesModel()->setDisplayModeIndex(value);
                    }
                });
            p.pixelTypeComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getBMDDevicesModel()->setPixelTypeIndex(value);
                    }
                });

            p._444SDIVideoOutputCheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getBMDDevicesModel()->observeData()->get().boolOptions;
                        options[tl::bmd::Option::_444SDIVideoOutput] = value;
                        app->getBMDDevicesModel()->setBoolOptions(options);
                    }
                });

            p.dataObserver = ftk::Observer<tl::bmd::DevicesModelData>::create(
                app->getBMDDevicesModel()->observeData(),
                [this](const tl::bmd::DevicesModelData& value)
                {
                    FTK_P();
                    p.enabledCheckBox->setChecked(value.deviceEnabled);

                    p.deviceComboBox->setItems(value.devices);
                    p.deviceComboBox->setCurrentIndex(value.deviceIndex);
                    p.displayModeComboBox->setItems(value.displayModes);
                    p.displayModeComboBox->setCurrentIndex(value.displayModeIndex);
                    std::vector<std::string> pixelTypes;
                    for (const auto& pixelType : value.pixelTypes)
                    {
                        std::stringstream ss;
                        ss << pixelType;
                        pixelTypes.push_back(ss.str());
                    }
                    p.pixelTypeComboBox->setItems(pixelTypes);
                    p.pixelTypeComboBox->setCurrentIndex(value.pixelTypeIndex);

                    const auto i = value.boolOptions.find(tl::bmd::Option::_444SDIVideoOutput);
                    p._444SDIVideoOutputCheckBox->setChecked(i != value.boolOptions.end() ? i->second : false);
                });
#endif // TLRENDER_BMD
        }

        DevicesTool::DevicesTool() :
            _p(new Private)
        {}

        DevicesTool::~DevicesTool()
        {}

        std::shared_ptr<DevicesTool> DevicesTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<DevicesTool>(new DevicesTool);
            out->_init(context, app, parent);
            return out;
        }
    }
}
