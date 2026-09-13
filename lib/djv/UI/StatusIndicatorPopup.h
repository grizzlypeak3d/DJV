// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>
#include <djv/Models/Export.h>

#include <ftk/UI/IWidgetPopup.h>

namespace djv
{
    namespace ui
    {
        //! Status indicator popup.
        class DJV_UI_API_TYPE StatusIndicatorPopup : public ftk::IWidgetPopup
        {
            FTK_NON_COPYABLE(StatusIndicatorPopup);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::vector<std::pair<std::string, std::string> >&,
                const std::shared_ptr<IWidget>& parent);

            StatusIndicatorPopup();

        public:
            DJV_UI_API virtual ~StatusIndicatorPopup();

            DJV_UI_API static std::shared_ptr<StatusIndicatorPopup> create(
                const std::shared_ptr<ftk::Context>&,
                const std::vector<std::pair<std::string, std::string> >&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_UI_API void setIndicators(const std::map<std::string, bool>&);

            //! Set the callback for turning an option off.
            DJV_UI_API void setOffCallback(const std::function<void(const std::string&)>&);

            //! Set the callback for showing an option's tool.
            DJV_UI_API void setToolCallback(const std::function<void(const std::string&)>&);

        private:
            FTK_PRIVATE();
        };
    }
}
