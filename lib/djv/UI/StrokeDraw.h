// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <djv/Models/Review.h>

#include <ftk/Core/FontSystem.h>
#include <ftk/Core/IRender.h>

#include <functional>

namespace djv
{
    namespace ui
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
        DJV_UI_API void drawStroke(
            const std::shared_ptr<ftk::IRender>&,
            const std::shared_ptr<ftk::FontSystem>&,
            const models::ReviewStroke&,
            const std::function<ftk::V2F(const ftk::V2F&)>& toScreen,
            float scale,
            float alpha = 1.F,
            bool caret = false);

        ///@}
    }
}
