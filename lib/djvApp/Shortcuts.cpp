// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Shortcuts.h>

namespace djv
{
    namespace app
    {
        Shortcut::Shortcut(
            const std::string& name,
            const std::string& text,
            const ftk::KeyShortcut& shortcut) :
            name(name),
            text(text),
            shortcuts({ shortcut })
        {}

        bool Shortcut::operator == (const Shortcut& other) const
        {
            return
                name == other.name &&
                text == other.text &&
                shortcuts == other.shortcuts;
        }

        bool Shortcut::operator != (const Shortcut& other) const
        {
            return !(*this == other);
        }

        void to_json(nlohmann::json& json, const Shortcut& in)
        {
            json["Name"] = in.name;
            json["Text"] = in.text;
            nlohmann::json shortcuts;
            for (const auto& i : in.shortcuts)
            {
                nlohmann::json shortcut;
                shortcut.push_back(to_string(i.key));
                shortcut.push_back(i.modifiers);
                shortcuts.push_back(shortcut);
            }
            json["Shortcuts"] = shortcuts;
        }

        void from_json(const nlohmann::json& json, Shortcut& out)
        {
            json.at("Name").get_to(out.name);
            json.at("Text").get_to(out.text);
            auto& shortcuts = json.at("Shortcuts");
            if (shortcuts.is_array())
            {
                for (auto i = shortcuts.begin(); i != shortcuts.end(); ++i)
                {
                    if (i->is_array() && 2 == i->size())
                    {
                        ftk::KeyShortcut shortcut;
                        from_string((*i)[0].get<std::string>(), shortcut.key);
                        (*i)[1].get_to(shortcut.modifiers);
                        out.shortcuts.push_back(shortcut);
                    }
                }
            }
        }
    }
}
