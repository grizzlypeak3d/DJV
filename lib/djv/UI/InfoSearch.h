// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/Core/String.h>

#include <string>

namespace djv
{
    namespace ui
    {
        namespace detail
        {
            //! Existing single-query Information row filter. Keep this
            //! separate from #316's unresolved multi-term search semantics.
            inline bool matchesInfoRow(
                const std::string& name,
                const std::string& value,
                const std::string& search)
            {
                return search.empty() ||
                    ftk::contains(name, search, ftk::CaseCompare::Insensitive) ||
                    ftk::contains(value, search, ftk::CaseCompare::Insensitive);
            }
        }
    }
}
