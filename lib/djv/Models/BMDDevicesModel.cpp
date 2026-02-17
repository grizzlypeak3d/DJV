// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/BMDDevicesModel.h>

#include <ftk/UI/Settings.h>

namespace djv
{
    namespace models
    {
        struct BMDDevicesModel::Private
        {
            std::shared_ptr<ftk::Settings> settings;
        };

        void BMDDevicesModel::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            tl::bmd::DevicesModel::_init(context);
            FTK_P();

            p.settings = settings;

            tl::bmd::DevicesModelData data;
            p.settings->getT("/BMD", data);
            setDeviceIndex(data.deviceIndex);
            setDisplayModeIndex(data.displayModeIndex);
            setPixelTypeIndex(data.pixelTypeIndex);
            setDeviceEnabled(data.deviceEnabled);
            setBoolOptions(data.boolOptions);
            setHDRMode(data.hdrMode);
            setHDRData(data.hdrData);
        }

        BMDDevicesModel::BMDDevicesModel() :
            _p(new Private)
        {}

        BMDDevicesModel::~BMDDevicesModel()
        {
            FTK_P();
            p.settings->setT("/BMD", observeData()->get());
        }

        std::shared_ptr<BMDDevicesModel> BMDDevicesModel::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            auto out = std::shared_ptr<BMDDevicesModel>(new BMDDevicesModel);
            out->_init(context, settings);
            return out;
        }
    }
}
