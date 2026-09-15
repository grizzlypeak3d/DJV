// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <memory>

namespace ftk
{
    class Context;
}

namespace djv
{
    namespace app
    {
        //! Point the "Open with" entries Windows keeps for this program in the
        //! user's own registry hive back at the running executable, where they
        //! name a copy of it that has been removed. Does nothing on other
        //! platforms.
        void repairShellAssociations(const std::shared_ptr<ftk::Context>&);
    }
}
