// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <tlRender/Device/BMDDevicesModel.h>

namespace ftk
{
    class Settings;
}

namespace djv
{
    namespace app
    {
        //! BMD devices model.
        class BMDDevicesModel : public tl::bmd::DevicesModel
        {
            FTK_NON_COPYABLE(BMDDevicesModel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            BMDDevicesModel();

        public:
            ~BMDDevicesModel();

            //! Create a new model.
            static std::shared_ptr<BMDDevicesModel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

        private:
            FTK_PRIVATE();
        };
    }
}
