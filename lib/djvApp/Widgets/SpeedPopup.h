// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/DoubleModel.h>
#include <ftk/UI/IMenuPopup.h>

namespace djv
{
    namespace app
    {
        //! Speed popup.
        class SpeedPopup : public ftk::IMenuPopup
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
            virtual ~SpeedPopup();

            static std::shared_ptr<SpeedPopup> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::DoubleModel>&,
                double defaultSpeed,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setCallback(const std::function<void(double)>&);

            void open(
                const std::shared_ptr<ftk::IWindow>&,
                const ftk::Box2I& buttonGeometry) override;
        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };
    }
}
