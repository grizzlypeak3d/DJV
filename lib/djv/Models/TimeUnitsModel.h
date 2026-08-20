// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <tlRender/Timeline/TimeUnits.h>

namespace ftk
{
    class Settings;
}

namespace djv
{
    namespace models
    {
        //! Time units model.
        class DJV_API_TYPE TimeUnitsModel : public tl::TimeUnitsModel
        {
            FTK_NON_COPYABLE(TimeUnitsModel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            TimeUnitsModel();

        public:
            DJV_API ~TimeUnitsModel();

            //! Create a new model.
            DJV_API static std::shared_ptr<TimeUnitsModel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

        private:
            FTK_PRIVATE();
        };
    }
}
