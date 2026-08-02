// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IWidgetPopup.h>

#include <ftk/Core/Range.h>

namespace djv
{
    namespace ui
    {
        //! Frame range popup.
        //!
        //! The range a sequence is meant to cover is set rarely, and two edits
        //! in every row of the files tool cost more width than the tool has.
        class FrameRangePopup : public ftk::IWidgetPopup
        {
            FTK_NON_COPYABLE(FrameRangePopup);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const ftk::RangeI64&,
                const std::shared_ptr<IWidget>& parent);

            FrameRangePopup();

        public:
            virtual ~FrameRangePopup();

            //! Create a new popup.
            static std::shared_ptr<FrameRangePopup> create(
                const std::shared_ptr<ftk::Context>&,
                const ftk::RangeI64&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the callback.
            void setCallback(const std::function<void(const ftk::RangeI64&)>&);

        private:
            FTK_PRIVATE();
        };
    }
}
