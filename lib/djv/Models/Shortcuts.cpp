// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/Shortcuts.h>

#include <ftk/UI/Event.h>

#include <algorithm>

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

        const std::vector<Shortcut>& getDefaultShortcuts()
        {
            static const std::vector<Shortcut> out =
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

                // File
                Shortcut("File/Open", "", ftk::KeyShortcut(ftk::Key::O, static_cast<int>(ftk::commandKeyModifier))),
                Shortcut("File/OpenAudio", "", ftk::KeyShortcut(ftk::Key::O, static_cast<int>(ftk::KeyModifier::Shift) | static_cast<int>(ftk::commandKeyModifier))),
                Shortcut("File/RecentFiles", "", ftk::KeyShortcut(ftk::Key::R, static_cast<int>(ftk::KeyModifier::Shift) | static_cast<int>(ftk::commandKeyModifier))),
                Shortcut("File/Close", "", ftk::KeyShortcut(ftk::Key::E, static_cast<int>(ftk::commandKeyModifier))),
                Shortcut("File/CloseAll", "", ftk::KeyShortcut(ftk::Key::E, static_cast<int>(ftk::KeyModifier::Shift) | static_cast<int>(ftk::commandKeyModifier))),
                Shortcut("File/Reload", "", ftk::KeyShortcut(ftk::Key::R, static_cast<int>(ftk::commandKeyModifier))),
                Shortcut("File/ExportStill", "", ftk::KeyShortcut(ftk::Key::S, static_cast<int>(ftk::KeyModifier::Shift) | static_cast<int>(ftk::commandKeyModifier))),
                Shortcut("File/Next", "", ftk::KeyShortcut(ftk::Key::PageDown, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("File/Prev", "", ftk::KeyShortcut(ftk::Key::PageUp, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("File/NextMediaReference", "", ftk::KeyShortcut(ftk::Key::M, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("File/NextLayer", "", ftk::KeyShortcut(ftk::Key::Equals, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("File/PrevLayer", "", ftk::KeyShortcut(ftk::Key::Minus, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("File/Exit", "", ftk::KeyShortcut(ftk::Key::Q, static_cast<int>(ftk::commandKeyModifier))),

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
                Shortcut("Review/Open", "", ftk::KeyShortcut(ftk::Key::O, static_cast<int>(ftk::KeyModifier::Alt) | static_cast<int>(ftk::commandKeyModifier))),
                Shortcut("Review/Save", "", ftk::KeyShortcut(ftk::Key::S, static_cast<int>(ftk::KeyModifier::Alt) | static_cast<int>(ftk::commandKeyModifier))),
                Shortcut("Review/Undo", "", ftk::KeyShortcut(ftk::Key::Z, static_cast<int>(ftk::commandKeyModifier))),
                // Ctrl+Y as well where that is the convention; macOS has only the
                // one.
                Shortcut(
                    "Review/Redo",
                    "",
                    ftk::KeyShortcut(ftk::Key::Z, static_cast<int>(ftk::KeyModifier::Shift) | static_cast<int>(ftk::commandKeyModifier)),
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

