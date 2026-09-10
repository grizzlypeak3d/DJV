// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <tlRender/Timeline/ColorOptions.h>
#include <tlRender/Timeline/DisplayOptions.h>
#include <tlRender/Timeline/Player.h>
#include <tlRender/Timeline/TimeUnits.h>

#include <ftk/UI/IPopup.h>

namespace djv
{
    namespace ui
    {
        //! Timeline preview popup.
        //!
        //! Shows the frame at a time on the timeline, drawn the way the view
        //! would draw it: the player's own timeline renders it, under the
        //! same image, display, OCIO and LUT options, so a comparison,
        //! a transition or a color transform in the view is in the preview.
        class DJV_UI_API_TYPE TimelinePreview : public ftk::IPopup
        {
            FTK_NON_COPYABLE(TimelinePreview);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<tl::Player>&,
                const std::shared_ptr<tl::ITimeUnitsModel>&,
                const std::shared_ptr<IWidget>& window);

            TimelinePreview();

        public:
            DJV_UI_API virtual ~TimelinePreview();

            //! Create a new popup, as a child of the window.
            DJV_UI_API static std::shared_ptr<TimelinePreview> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<tl::Player>&,
                const std::shared_ptr<tl::ITimeUnitsModel>&,
                const std::shared_ptr<IWidget>& window);

            //! Set the position the preview is shown above: the cursor's X
            //! and the top of the timeline, so it does not ride up and down
            //! with the cursor.
            DJV_UI_API void setPos(const ftk::V2I&);

            //! Set the time to preview. The frame is asked for and shown when
            //! it arrives.
            DJV_UI_API void setTime(const OTIO_NS::RationalTime&);

            //! Set the image options.
            DJV_UI_API void setImageOptions(const ftk::ImageOptions&);

            //! Set the display options.
            DJV_UI_API void setDisplayOptions(const tl::DisplayOptions&);

            //! Set the OCIO options.
            DJV_UI_API void setOCIOOptions(const tl::OCIOOptions&);

            //! Set the OCIO input color space resolver.
            DJV_UI_API void setOCIOInputResolver(
                const std::function<std::string(
                    const std::string& path,
                    const ftk::ImageTags&)>&);

            //! Set the LUT options.
            DJV_UI_API void setLUTOptions(const tl::LUTOptions&);

            DJV_UI_API void close() override;

            DJV_UI_API void setGeometry(const ftk::Box2I&) override;
            DJV_UI_API void sizeHintEvent(const ftk::SizeHintEvent&) override;
            DJV_UI_API void tickEvent(
                bool parentsVisible,
                bool parentsEnabled,
                const ftk::TickEvent&) override;
            DJV_UI_API void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;

        private:
            FTK_PRIVATE();
        };
    }
}
