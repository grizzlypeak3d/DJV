// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <ftk/UI/DoubleModel.h>
#include <ftk/UI/IWidgetPopup.h>

namespace djv
{
    namespace ui
    {
        //! Speed popup.
        class DJV_API_TYPE SpeedPopup : public ftk::IWidgetPopup
        {
            FTK_NON_COPYABLE(SpeedPopup);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::DoubleModel>&,
                double defaultSpeed,
                const std::shared_ptr<IWidget>& parent);

            SpeedPopup();

        public:
            DJV_API virtual ~SpeedPopup();

            DJV_API static std::shared_ptr<SpeedPopup> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::DoubleModel>&,
                double defaultSpeed,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_API void setCallback(const std::function<void(double)>&);

            DJV_API void open(
                const std::shared_ptr<ftk::IWindow>&,
                const ftk::Box2I& buttonGeometry,
                const std::optional<ftk::Box2I>& widgetGeometry = std::optional<ftk::Box2I>()) override;

        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };
    }
}
