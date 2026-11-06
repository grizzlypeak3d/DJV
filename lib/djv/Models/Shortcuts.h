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
        struct DJV_MODELS_API_TYPE Shortcut
        {
            Shortcut() = default;
            DJV_MODELS_API Shortcut(
                const std::string& name,
                const std::string& text,
                const ftk::KeyShortcut& primary = ftk::KeyShortcut(),
                const ftk::KeyShortcut& secondary = ftk::KeyShortcut());

            std::string      name;
            std::string      text;
            ftk::KeyShortcut primary;
            ftk::KeyShortcut secondary;

            bool operator == (const Shortcut&) const = default;
        };

        //! The default keys of the application's actions, by the action's
        //! name ("Playback/Toggle"): the desktop's actions take theirs from
        //! here, and the iPad's keyboard does too, so the two keep the same
        //! keys. The text is left to the actions, which show it.
        DJV_MODELS_API const std::vector<Shortcut>& getDefaultShortcuts();

        //! The default keys of one action: none when it has none.
        DJV_MODELS_API Shortcut getDefaultShortcut(const std::string& name);

        //! \name Serialize
        ///@{

        DJV_MODELS_API void to_json(nlohmann::json&, const Shortcut&);

        DJV_MODELS_API void from_json(const nlohmann::json&, Shortcut&);

        ///@}
    }
}
