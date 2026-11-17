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
        //! and the iPad to draw and drag the same way: a dot at its center
        //! that moves it from anywhere within half the disk, and the band
        //! around that which turns it, from anywhere on it -- a second dot
        //! to turn it did not read as turning, and a ring with ticks was
        //! more than it needed (#957). Turning starts no nearer the center
        //! than the band: the angle swings fast there, and a press just off
        //! the dot turned the wipe when it was meant to move it (#966).
        //! There is no line across the picture, which would hide the seam
        //! being looked at; the seam shows the angle. All in the view's
        //! pixels.
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

            //! The disk's radius.
            float radius = 0.F;

            //! Where the wipe crosses the disk's edge, up the wipe from the
            //! center when it is upright.
            ftk::V2F rotate;

            bool operator == (const WipeWidget&) const = default;
        };

        //! The wipe's handles, for the compare options, the first
        //! box (which the wipe is drawn across, see tl::getWipeCenter()),
        //! the view's position and zoom, and the disk's radius.
        DJV_MODELS_API WipeWidget getWipeWidget(
            const tl::CompareOptions&,
            const ftk::Box2I& firstBox,
            const ftk::V2F& viewPos,
            float zoom,
            float rotateDistance);

        //! The fraction of the disk's radius, from the center, that moves
        //! the wipe rather than turning it.
        constexpr float wipeMoveFraction = .5F;

        //! The handle within reach of a point: the dot, from anywhere in
        //! the middle of the disk (see wipeMoveFraction) or within reach of
        //! the dot, whichever is larger; or else the band around it,
        //! anywhere on it or within reach of its edge.
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

        //! The wipe's rotation as the disk is dragged: the rotation it had
        //! when the disk was taken, turned by the angle from where it was
        //! taken to where the pointer is, so taking it anywhere does not
        //! make it jump. Snapped (see snapWipeRotation()).
        DJV_MODELS_API float getWipeRotationDrag(
            float pressRotation,
            const ftk::V2F& center,
            const ftk::V2F& grab,
            const ftk::V2F& pos,
            bool step = false);

        //! Whether a turn holds its angle with the pointer at a point: in
        //! the middle of the disk, where the angle around the center swings
        //! fast and, at the center, is not there at all. A drag through the
        //! middle holds the wipe still, then turns it to point at the
        //! pointer on the far side.
        DJV_MODELS_API bool isWipeTurnHeld(
            const WipeWidget&,
            const ftk::V2F& pos);

        //! Snap a rotation, from zero to 360: to the nearest multiple of 45
        //! degrees with step (Shift, as for a shape held square), and
        //! otherwise to level or plumb when it is within two degrees of it,
        //! which is what a wipe is almost always meant to be.
        DJV_MODELS_API float snapWipeRotation(float, bool step = false);

        ///@}
    }
}
