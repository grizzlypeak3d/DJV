// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/SettingsKeys.h>

#include <ftk/UI/Settings.h>

namespace djv
{
    namespace models
    {
        struct TimeUnitsModel::Private
        {
            std::shared_ptr<ftk::Settings> settings;
        };

        void TimeUnitsModel::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            tl::TimeUnitsModel::_init(context);
            FTK_P();

            p.settings = settings;

            tl::TimeUnits units = tl::TimeUnits::Timecode;
            std::string s = tl::to_string(units);
            p.settings->get(settingsKeys::timeUnits, s);
            tl::from_string(s, units);
            setTimeUnits(units);
        }

        TimeUnitsModel::TimeUnitsModel() :
            _p(new Private)
        {}

        TimeUnitsModel::~TimeUnitsModel()
        {
            save();
        }

        void TimeUnitsModel::save()
        {
            FTK_P();
            p.settings->set(settingsKeys::timeUnits, tl::to_string(getTimeUnits()));
        }

        std::shared_ptr<TimeUnitsModel> TimeUnitsModel::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            auto out = std::shared_ptr<TimeUnitsModel>(new TimeUnitsModel);
            out->_init(context, settings);
            return out;
        }
    }
}
