// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <tlRender/Timeline/DisplayOptions.h>

#include <ftk/Core/Image.h>
#include <ftk/Core/Util.h>

#include <functional>
#include <memory>

namespace tl
{
    namespace ui
    {
        class Viewport;
    }
}

namespace djv
{
    namespace models
    {
        class ColorModel;
        class FilesModel;
        class ViewportModel;
    }

    namespace ui
    {
        //! Keeps a viewport showing what the models say: the comparison,
        //! the color pipeline (the OCIO and LUT options, and the input
        //! color space resolved for each file), the image and display
        //! options, the background, and the color buffer.
        //!
        //! The main viewport and the magnifier show the same picture, and
        //! this is what makes them agree: an option that reaches the
        //! viewport through here reaches both, rather than each of them
        //! observing the models on its own.
        class DJV_UI_API_TYPE ViewportBinding :
            public std::enable_shared_from_this<ViewportBinding>
        {
            FTK_NON_COPYABLE(ViewportBinding);

        protected:
            void _init(
                const std::shared_ptr<models::FilesModel>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<tl::ui::Viewport>&);

            ViewportBinding();

        public:
            DJV_UI_API ~ViewportBinding();

            DJV_UI_API static std::shared_ptr<ViewportBinding> create(
                const std::shared_ptr<models::FilesModel>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<tl::ui::Viewport>&);

            //! How many video layers the viewport is showing. The image and
            //! display options are given per layer, each with the input
            //! color space resolved for its file, so a comparison of files
            //! in different color spaces shows each of them correctly.
            DJV_UI_API void setVideoFramesSize(size_t);

            DJV_UI_API const ftk::ImageOptions& getImageOptions() const;
            DJV_UI_API const tl::DisplayOptions& getDisplayOptions() const;

            //! Set a function called after any of the options changed and
            //! reached the viewport, and once right away. For what a
            //! viewport shows beside the picture -- a heads up display that
            //! names the color buffer, a message about the comparison.
            DJV_UI_API void setChangedCallback(const std::function<void(void)>&);

        private:
            void _videoUpdate();
            void _changed();

            FTK_PRIVATE();
        };
    }
}
