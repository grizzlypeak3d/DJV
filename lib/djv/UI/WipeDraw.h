// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <djv/Models/Wipe.h>

#include <ftk/Core/IRender.h>

namespace djv
{
    namespace ui
    {
        //! \name Wipe Drawing
        ///@{

        //! The radius of the disk that turns the wipe, for a pointer and
        //! for a finger, and how near each has to be to take the dot, in
        //! the view's pixels at a display scale of one.
        constexpr float wipeDiskRadius = 24.F;
        constexpr float wipeTouchDiskRadius = 36.F;
        constexpr float wipePointerReach = 8.F;
        constexpr float wipeTouchReach = 22.F;

        //! Draw the wipe's handles so that they show on any picture: a
        //! translucent dark band with faint light edges and two ticks
        //! where the seam crosses it, which turns it,
        //! and a light dot with a dark outline in the clear middle, which
        //! moves it from anywhere in the middle (see
        //! models::wipeMoveFraction). Like a widget, the part pointed at is
        //! lighter, and the part pressed lighter still. The seam itself
        //! shows the wipe's angle, through the middle. The opacity fades
        //! the whole of it.
        DJV_UI_API void drawWipe(
            const std::shared_ptr<ftk::IRender>&,
            const models::WipeWidget&,
            float displayScale,
            models::WipeHandle hover = models::WipeHandle::None,
            models::WipeHandle pressed = models::WipeHandle::None,
            float opacity = 1.F);

        ///@}
    }
}
