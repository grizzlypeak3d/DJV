// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Core/Shortcuts.h>

namespace djv
{
    Shortcut::Shortcut(
        const std::string& name,
        const std::string& text,
        const ftk::KeyShortcut& primary,
        const ftk::KeyShortcut& secondary) :
        name(name),
        text(text),
        primary(primary),
        secondary(secondary)
    {
    }

    bool Shortcut::operator == (const Shortcut& other) const
    {
        return
            name == other.name &&
            text == other.text &&
            primary == other.primary &&
            secondary == other.secondary;
    }

    bool Shortcut::operator != (const Shortcut& other) const
    {
        return !(*this == other);
    }

    void to_json(nlohmann::json& json, const Shortcut& in)
    {
        json["Name"] = in.name;
        json["Text"] = in.text;
        json["Primary"] = in.primary;
        json["Secondary"] = in.secondary;
    }

    void from_json(const nlohmann::json& json, Shortcut& out)
    {
        json.at("Name").get_to(out.name);
        json.at("Text").get_to(out.text);
        json.at("Primary").get_to(out.primary);
        json.at("Secondary").get_to(out.secondary);
    }
}
