// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <tlRender/Timeline/TimeUnits.h>

namespace ftk
{
    class Settings;
}

namespace djv
{
    namespace app
    {
        //! Time units model.
        class TimeUnitsModel : public tl::TimeUnitsModel
        {
            FTK_NON_COPYABLE(TimeUnitsModel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            TimeUnitsModel();

        public:
            ~TimeUnitsModel();

            //! Create a new model.
            static std::shared_ptr<TimeUnitsModel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

        private:
            FTK_PRIVATE();
        };
    }
}
