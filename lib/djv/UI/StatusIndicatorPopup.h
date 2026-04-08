// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IWidgetPopup.h>

namespace djv
{
    namespace ui
    {
        //! Status indicator popup.
        class StatusIndicatorPopup : public ftk::IWidgetPopup
        {
            FTK_NON_COPYABLE(StatusIndicatorPopup);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            StatusIndicatorPopup();

        public:
            virtual ~StatusIndicatorPopup();

            static std::shared_ptr<StatusIndicatorPopup> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setOCIO(bool);
            void setLUT(bool);
            void setColor(bool);
            void setAudioOffset(bool);
            void setOutputDevice(bool);

        private:
            FTK_PRIVATE();
        };
    }
}
