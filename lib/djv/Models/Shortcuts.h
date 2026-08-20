// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <ftk/UI/Action.h>

namespace djv
{
    namespace models
    {
        //! Keyboard shortcuts.
        struct DJV_API_TYPE Shortcut
        {
            Shortcut() = default;
            DJV_API Shortcut(
                const std::string& name,
                const std::string& text,
                const ftk::KeyShortcut& primary = ftk::KeyShortcut(),
                const ftk::KeyShortcut& secondary = ftk::KeyShortcut());

            std::string      name;
            std::string      text;
            ftk::KeyShortcut primary;
            ftk::KeyShortcut secondary;

            DJV_API bool operator == (const Shortcut&) const;
            DJV_API bool operator != (const Shortcut&) const;
        };

        //! \name Serialize
        ///@{

        DJV_API void to_json(nlohmann::json&, const Shortcut&);

        DJV_API void from_json(const nlohmann::json&, Shortcut&);

        ///@}
    }
}
