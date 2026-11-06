// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <djv/Models/Review.h>

#include <ftk/Core/FontSystem.h>
#include <ftk/Core/IRender.h>

#include <functional>

namespace djv
{
    namespace models
    {
        //! \name Annotation Drawing
        ///@{

        //! Draw one stroke of an annotation.
        //!
        //! The stroke is in the pixels of its source image; toScreen maps a
        //! point of those to where it is drawn, and scale is how many drawn
        //! pixels one image pixel is, which sizes the width and the text.
        //! The geometry is built after the mapping, so a curve is smooth
        //! and a width round at any zoom. The alpha fades a stroke, as the
        //! onion skin does; the caret marks text being typed.
        DJV_MODELS_API void drawStroke(
            const std::shared_ptr<ftk::IRender>&,
            const std::shared_ptr<ftk::FontSystem>&,
            const models::ReviewStroke&,
            const std::function<ftk::V2F(const ftk::V2F&)>& toScreen,
            float scale,
            float alpha = 1.F,
            bool caret = false);

        //! Draw the frame of a selected stroke: a thin line a little
        //! outside what it covers, dark under light so that it reads on
        //! any picture, and the handles of a shape's two points. The
        //! mapping and the scale are drawStroke()'s; the display scale
        //! sizes the handles.
        DJV_MODELS_API void drawSelection(
            const std::shared_ptr<ftk::IRender>&,
            const std::shared_ptr<ftk::FontSystem>&,
            const models::ReviewStroke&,
            const std::function<ftk::V2F(const ftk::V2F&)>& toScreen,
            float scale,
            float displayScale);

        //! The size of a selected shape's handles on the screen, in pixels
        //! at a display scale of one.
        const float selectionHandleSize = 8.F;

        ///@}
    }
}
