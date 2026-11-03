// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <tlRender/Timeline/CompareOptions.h>

#include <ftk/Core/Box.h>

namespace djv
{
    namespace models
    {
        //! \name Wipe
        //! The wipe's handles over the picture, for the desktop's viewport
        //! and the iPad to draw and drag the same way: one at its center
        //! that moves it, and one along the wipe that turns it. There is no
        //! line across the picture, which would hide the seam being looked
        //! at. All in the view's pixels.
        ///@{

        //! A part of the wipe.
        enum class DJV_MODELS_API_TYPE WipeHandle
        {
            None,
            Center,
            Rotate
        };

        //! Where the wipe's handles are.
        struct DJV_MODELS_API_TYPE WipeWidget
        {
            //! The center, which the wipe goes through.
            ftk::V2F center;

            //! The handle that turns it, along the wipe from the center.
            ftk::V2F rotate;

            bool operator == (const WipeWidget&) const = default;
        };

        //! The wipe's handles, for the compare options, the first
        //! box (which the wipe is drawn across, see tl::getWipeCenter()),
        //! the view's position and zoom, and the rotation handle's distance
        //! from the center.
        DJV_MODELS_API WipeWidget getWipeWidget(
            const tl::CompareOptions&,
            const ftk::Box2I& firstBox,
            const ftk::V2F& viewPos,
            float zoom,
            float rotateDistance);

        //! The handle within reach of a point, the rotation handle first.
        DJV_MODELS_API WipeHandle getWipeHandle(
            const WipeWidget&,
            const ftk::V2F& pos,
            float reach);

        //! The wipe's center at a point in the view.
        DJV_MODELS_API ftk::V2F getWipeCenter(
            const ftk::V2F& pos,
            const ftk::Box2I& firstBox,
            const ftk::V2F& viewPos,
            float zoom);

        //! The wipe's rotation, in degrees from zero to 360, with the
        //! rotation handle at a point: the wipe turned to point from the
        //! center to it.
        DJV_MODELS_API float getWipeRotation(
            const ftk::V2F& center,
            const ftk::V2F& pos);

        ///@}
    }
}
