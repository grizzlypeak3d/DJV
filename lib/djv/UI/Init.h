// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <memory>

namespace ftk
{
    class Context;
}

namespace djv
{
    namespace ui
    {
        //! Register the application's icons with the icon system: every
        //! icon compiled into the resource library, generated from the
        //! files in etc/Icons.
        DJV_UI_API void initIcons(const std::shared_ptr<ftk::Context>&);
    }
}
