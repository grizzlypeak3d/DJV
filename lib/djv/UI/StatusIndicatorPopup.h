// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>
#include <djv/Models/Export.h>
#include <djv/Models/Status.h>

#include <ftk/UI/IWidgetPopup.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace djv
{
    namespace ui
    {
        //! Status indicator popup: the options by group, each group by its
        //! letter, in the order the indicator shows the letters.
        class DJV_UI_API_TYPE StatusIndicatorPopup : public ftk::IWidgetPopup
        {
            FTK_NON_COPYABLE(StatusIndicatorPopup);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::vector<models::StatusItem>&,
                const std::vector<models::StatusGroup>&,
                const std::shared_ptr<IWidget>& parent);

            StatusIndicatorPopup();

        public:
            DJV_UI_API virtual ~StatusIndicatorPopup();

            DJV_UI_API static std::shared_ptr<StatusIndicatorPopup> create(
                const std::shared_ptr<ftk::Context>&,
                const std::vector<models::StatusItem>&,
                const std::vector<models::StatusGroup>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set which options are in use.
            DJV_UI_API void setIndicators(const std::map<std::string, bool>&);

            //! Set the callback for an option being clicked.
            DJV_UI_API void setCallback(const std::function<void(const std::string&)>&);

        private:
            FTK_PRIVATE();
        };
    }
}
