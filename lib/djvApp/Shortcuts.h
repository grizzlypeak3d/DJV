// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/Action.h>

namespace djv
{
    namespace app
    {
        //! Keyboard shortcuts.
        struct Shortcut
        {
            Shortcut() = default;
            Shortcut(
                const std::string& name,
                const std::string& text,
                const ftk::KeyShortcut& primary = ftk::KeyShortcut(),
                const ftk::KeyShortcut& secondary = ftk::KeyShortcut());

            std::string      name;
            std::string      text;
            ftk::KeyShortcut primary;
            ftk::KeyShortcut secondary;

            bool operator == (const Shortcut&) const;
            bool operator != (const Shortcut&) const;
        };

        //! \name Serialize
        ///@{

        void to_json(nlohmann::json&, const Shortcut&);

        void from_json(const nlohmann::json&, Shortcut&);

        ///@}
    }
}
