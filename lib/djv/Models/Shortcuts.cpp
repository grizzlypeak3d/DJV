// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/Shortcuts.h>

#include <ftk/UI/Event.h>

#include <algorithm>
#include <sstream>

#include <nlohmann/json.hpp>

namespace djv
{
    namespace models
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
        {}

        namespace
        {
            //! The documentation's stand-in for the command modifier: a bit
            //! no key modifier uses.
            const int docsCommandModifier = 1 << 12;

            //! The documentation's mark on a key that macOS does not have,
            //! so that the reference made on any platform lists it, and
            //! says so, rather than each platform writing its own.
            const int docsNotMacOS = 1 << 13;

            //! The table, with the modifier that is Command on macOS and
            //! Control elsewhere given: the application's own, or the
            //! documentation's stand-in for it, which on Linux and Windows
            //! could not be told from a shortcut that is Control everywhere.
            std::vector<Shortcut> getDefaultShortcutsTable(int cmd)
            {
                return
            {
                // Audio
                Shortcut("Audio/VolumeUp", "", ftk::Key::Period),
                Shortcut("Audio/VolumeDown", "", ftk::Key::Comma),
                Shortcut("Audio/Mute", "", ftk::Key::M),

                // Color
                Shortcut("Color/OCIO", "", ftk::KeyShortcut(ftk::Key::N, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Color/LUT", "", ftk::KeyShortcut(ftk::Key::K, static_cast<int>(ftk::KeyModifier::Control))),

                // Compare
                Shortcut("Compare/Next", "", ftk::KeyShortcut(ftk::Key::PageDown, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Compare/Prev", "", ftk::KeyShortcut(ftk::Key::PageUp, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Compare/B", "", ftk::KeyShortcut(ftk::Key::B, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Compare/Wipe", "", ftk::KeyShortcut(ftk::Key::W, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Compare/Tile", "", ftk::KeyShortcut(ftk::Key::T, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Compare/HoldFrame", "", ftk::KeyShortcut(ftk::Key::H, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Compare/ClearHeld", "", ftk::KeyShortcut(ftk::Key::H, static_cast<int>(ftk::KeyModifier::Shift) | static_cast<int>(ftk::KeyModifier::Alt))),

                // File
                Shortcut("File/Open", "", ftk::KeyShortcut(ftk::Key::O, cmd)),
                Shortcut("File/OpenAudio", "", ftk::KeyShortcut(ftk::Key::O, static_cast<int>(ftk::KeyModifier::Shift) | cmd)),
                Shortcut("File/RecentFiles", "", ftk::KeyShortcut(ftk::Key::R, static_cast<int>(ftk::KeyModifier::Shift) | cmd)),
                Shortcut("File/Close", "", ftk::KeyShortcut(ftk::Key::E, cmd)),
                Shortcut("File/CloseAll", "", ftk::KeyShortcut(ftk::Key::E, static_cast<int>(ftk::KeyModifier::Shift) | cmd)),
                Shortcut("File/Reload", "", ftk::KeyShortcut(ftk::Key::R, cmd)),
                Shortcut("File/ExportStill", "", ftk::KeyShortcut(ftk::Key::S, static_cast<int>(ftk::KeyModifier::Shift) | cmd)),
                Shortcut("File/Next", "", ftk::KeyShortcut(ftk::Key::PageDown, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("File/Prev", "", ftk::KeyShortcut(ftk::Key::PageUp, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("File/NextMediaReference", "", ftk::KeyShortcut(ftk::Key::M, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("File/NextLayer", "", ftk::KeyShortcut(ftk::Key::Equals, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("File/PrevLayer", "", ftk::KeyShortcut(ftk::Key::Minus, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("File/Exit", "", ftk::KeyShortcut(ftk::Key::Q, cmd)),

                // Frame
                Shortcut("Frame/Start", "", ftk::Key::Home),
                Shortcut("Frame/End", "", ftk::Key::End),
                Shortcut("Frame/Prev", "", ftk::Key::Left),
                Shortcut("Frame/PrevX10", "", ftk::KeyShortcut(ftk::Key::Left, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Frame/PrevX100", "", ftk::KeyShortcut(ftk::Key::Left, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Frame/Next", "", ftk::Key::Right),
                Shortcut("Frame/NextX10", "", ftk::KeyShortcut(ftk::Key::Right, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Frame/NextX100", "", ftk::KeyShortcut(ftk::Key::Right, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Frame/FocusCurrent", "", ftk::KeyShortcut(ftk::Key::F, static_cast<int>(ftk::KeyModifier::Control))),

                // Playback
                Shortcut("Playback/Stop", "", ftk::Key::K),
                Shortcut("Playback/Forward", "", ftk::Key::L),
                Shortcut("Playback/Reverse", "", ftk::Key::J),
                Shortcut("Playback/Toggle", "", ftk::Key::Space),
                Shortcut("Playback/JumpBack1s", "", ftk::KeyShortcut(ftk::Key::J, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Playback/JumpBack10s", "", ftk::KeyShortcut(ftk::Key::J, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Playback/JumpForward1s", "", ftk::KeyShortcut(ftk::Key::L, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Playback/JumpForward10s", "", ftk::KeyShortcut(ftk::Key::L, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Playback/SetInPoint", "", ftk::Key::I),
                Shortcut("Playback/ResetInPoint", "", ftk::KeyShortcut(ftk::Key::I, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Playback/SetOutPoint", "", ftk::Key::O),
                Shortcut("Playback/ResetOutPoint", "", ftk::KeyShortcut(ftk::Key::O, static_cast<int>(ftk::KeyModifier::Shift))),

                // Review
                // Alt rather than Shift on the command modifier: Shift+Ctrl+O is
                // "Open with audio", and Shift+Ctrl+S is free but keeping the pair
                // symmetrical is worth more than reusing it.
                Shortcut("Review/Open", "", ftk::KeyShortcut(ftk::Key::O, static_cast<int>(ftk::KeyModifier::Alt) | cmd)),
                Shortcut("Review/Save", "", ftk::KeyShortcut(ftk::Key::S, static_cast<int>(ftk::KeyModifier::Alt) | cmd)),
                Shortcut("Review/Undo", "", ftk::KeyShortcut(ftk::Key::Z, cmd)),
                // Ctrl+Y as well where that is the convention; macOS has only the
                // one.
                Shortcut(
                    "Review/Redo",
                    "",
                    ftk::KeyShortcut(ftk::Key::Z, static_cast<int>(ftk::KeyModifier::Shift) | cmd),
                    docsCommandModifier == cmd ?
                        ftk::KeyShortcut(ftk::Key::Y, static_cast<int>(ftk::KeyModifier::Control) | docsNotMacOS) :
#if defined(__APPLE__)
                        ftk::KeyShortcut()),
#else // __APPLE__
                        ftk::KeyShortcut(ftk::Key::Y, static_cast<int>(ftk::KeyModifier::Control))),
#endif // __APPLE__
                // Shift and Control on the arrows are already taken by the X10 and
                // X100 frame steps.
                Shortcut("Review/PrevFrame", "", ftk::KeyShortcut(ftk::Key::Left, static_cast<int>(ftk::KeyModifier::Alt))),
                Shortcut("Review/NextFrame", "", ftk::KeyShortcut(ftk::Key::Right, static_cast<int>(ftk::KeyModifier::Alt))),

                // Timeline
                Shortcut("Timeline/TrackMedia", "", ftk::Key::C),

                // View
                Shortcut("View/Frame", "", ftk::Key::Backspace),
                Shortcut("View/ZoomReset", "", ftk::Key::_0),
                Shortcut("View/ZoomIn", "", ftk::Key::Equals),
                Shortcut("View/ZoomOut", "", ftk::Key::Minus),
                Shortcut("View/Center", "", ftk::Key::Backslash),
                Shortcut("View/Red", "", ftk::Key::R),
                Shortcut("View/Green", "", ftk::Key::G),
                Shortcut("View/Blue", "", ftk::Key::B),
                Shortcut("View/Alpha", "", ftk::Key::A),
                Shortcut("View/Negative", "", ftk::KeyShortcut(ftk::Key::I, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("View/MirrorHorizontal", "", ftk::Key::H),
                Shortcut("View/MirrorVertical", "", ftk::Key::V),
                Shortcut("View/Grid", "", ftk::KeyShortcut(ftk::Key::G, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("View/HUD", "", ftk::KeyShortcut(ftk::Key::H, static_cast<int>(ftk::KeyModifier::Control))),

                // Window
                Shortcut("Window/FullScreen", "", ftk::Key::U),
                Shortcut("Window/PresentMode", "", ftk::KeyShortcut(ftk::Key::P, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Window/Secondary", "", ftk::Key::Y),
            };
            }

            std::string getKeyLabel(ftk::Key key)
            {
                switch (key)
                {
                case ftk::Key::Period: return ".";
                case ftk::Key::Comma: return ",";
                case ftk::Key::Minus: return "-";
                case ftk::Key::Equals: return "=";
                case ftk::Key::Slash: return "/";
                case ftk::Key::Backslash: return "\\";
                case ftk::Key::Semicolon: return ";";
                case ftk::Key::SingleQuote: return "'";
                case ftk::Key::LeftBracket: return "[";
                case ftk::Key::RightBracket: return "]";
                case ftk::Key::BackQuote: return "`";
                default: break;
                }
                std::stringstream ss;
                ss << key;
                return ss.str();
            }

            std::string getDocsLabel(const ftk::KeyShortcut& shortcut)
            {
                std::string out;
                if (shortcut.key != ftk::Key::Unknown)
                {
                    std::vector<std::string> parts;
                    if (shortcut.modifiers & docsCommandModifier)
                        parts.push_back("Ctrl/Cmd");
                    if (shortcut.modifiers & static_cast<int>(ftk::KeyModifier::Control))
                        parts.push_back("Ctrl");
                    if (shortcut.modifiers & static_cast<int>(ftk::KeyModifier::Alt))
                        parts.push_back("Alt");
                    if (shortcut.modifiers & static_cast<int>(ftk::KeyModifier::Super))
                        parts.push_back("Super");
                    if (shortcut.modifiers & static_cast<int>(ftk::KeyModifier::Shift))
                        parts.push_back("Shift");
                    parts.push_back(getKeyLabel(shortcut.key));
                    for (size_t i = 0; i < parts.size(); ++i)
                    {
                        out += (i > 0 ? "+" : "") + parts[i];
                    }
                    if (shortcut.modifiers & docsNotMacOS)
                        out += " (not macOS)";
                }
                return out;
            }
        }

        const std::vector<Shortcut>& getDefaultShortcuts()
        {
            static const std::vector<Shortcut> out =
                getDefaultShortcutsTable(static_cast<int>(ftk::commandKeyModifier));
            return out;
        }

        std::vector<std::string> getDefaultShortcutLabels(const std::string& name)
        {
            std::vector<std::string> out;
            static const std::vector<Shortcut> docs =
                getDefaultShortcutsTable(docsCommandModifier);
            const auto i = std::find_if(
                docs.begin(),
                docs.end(),
                [&name](const Shortcut& value) { return value.name == name; });
            if (i != docs.end())
            {
                for (const auto& shortcut : { i->primary, i->secondary })
                {
                    const std::string label = getDocsLabel(shortcut);
                    if (!label.empty())
                    {
                        out.push_back(label);
                    }
                }
            }
            return out;
        }

        Shortcut getDefaultShortcut(const std::string& name)
        {
            const auto& defaults = getDefaultShortcuts();
            const auto i = std::find_if(
                defaults.begin(),
                defaults.end(),
                [&name](const Shortcut& value) { return value.name == name; });
            return i != defaults.end() ? *i : Shortcut(name, "");
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
}

