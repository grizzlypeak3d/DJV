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

        //! The rotation handle's distance from the center, and how near a
        //! pointer has to be to take a handle, in the view's pixels at a
        //! display scale of one: a fingertip's reach for a finger.
        constexpr float wipeRotateDistance = 64.F;
        constexpr float wipePointerReach = 10.F;
        constexpr float wipeTouchReach = 22.F;

        //! Draw the wipe's handles, light over dark so that they show on
        //! any picture, the one being dragged or pointed at larger.
        DJV_UI_API void drawWipe(
            const std::shared_ptr<ftk::IRender>&,
            const models::WipeWidget&,
            float displayScale,
            models::WipeHandle highlight = models::WipeHandle::None);

        ///@}
    }
}
