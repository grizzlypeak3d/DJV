---
title: Settings
layout: default
nav_order: 13
---

# Settings

Locations: **Tools** menu, **Tools** toolbar

Settings are stored as a JSON file in the **Documents/DJV** directory.

The **Settings** tool is divided into collapsible sections:

- **Cache** — Memory cache sizes for video, audio, and read-behind (see
  [Files]({{ '/files' | relative_url }})).
- **File Browser** — Whether to use the native or built-in file browser, and
  whether hidden files are shown.
- **Image Sequences** — How audio is paired with image sequences (see
  [Files]({{ '/files' | relative_url }})).
- **Mouse** — Mouse button bindings and scaling (see below).
- **Playback** — Options such as whether playback starts automatically when a
  file is opened.
- **Keyboard Shortcuts** — All keyboard shortcuts (see below).
- **Style** — Interface scale and custom fonts (see below).
- **Time** — Default time units (frames or timecode).
- **FFmpeg** / **FFmpeg Command** — Built-in FFmpeg options, and the external
  FFmpeg command (see [Files]({{ '/files' | relative_url }})).
- **USD** — Options for the experimental USD renderer.
- **Advanced** / **Miscellaneous** — Less common options.

Use the button at the bottom of the tool to save the settings manually.

## Mouse

![Keyboard shortcuts]({{ '/assets/mouse-settings.svg' | relative_url }})

The **Mouse** section maps mouse buttons (and optional modifier keys) to
different actions.

## Keyboard shortcuts

![Keyboard shortcuts]({{ '/assets/keyboard-shortcuts.svg' | relative_url }})

To set a shortcut, click its widget and press the new key combination. The
widget turns red if the chosen combination conflicts with an existing shortcut.
Each action can have a primary and a secondary shortcut, and the search box
helps find an action by name.

{: .note }
On macOS some shortcuts use the **Command** key instead of **Ctrl**.

## Font support

Custom fonts can be added in the **Style** section of the **Settings** tool.

To use custom fonts, first add the font files and then select them for use.

DJV includes **NotoSansCJKsc-Regular** for basic CJK rendering.

![Custom fonts]({{ '/assets/custom-fonts.svg' | relative_url }})
