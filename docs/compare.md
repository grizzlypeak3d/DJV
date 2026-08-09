---
title: A/B comparison
layout: default
nav_order: 8
---

# A/B comparison

A/B comparison lets you view two files (or layers) at once — useful for checking
revisions, matching color between shots, or spotting differences between renders.

It is for comparing *pixels*. To compare what two files are — their resolution,
format or timing — open them and switch between them with **File/Next** and
**File/Previous** instead, which makes each one current in turn. See
[Comparing file information](#comparing-file-information).

## Setting up a comparison

1. Open the files in DJV.
2. The current file is the **A** file. Mark one or more of the others as **B**
   with the **B** button in the **Files** tool, or step through them with
   **Compare/Next** and **Compare/Previous**.
3. Turn on a compare mode (see below).

Locations: **Compare** menu, **Compare** tool bar, **Files** tool

Shortcuts:

* Next B file: <kbd>Shift+Page Down</kbd>
* Previous B file: <kbd>Shift+Page Up</kbd>

{: .note }
Drag with **Alt** + left mouse button to move the wipe between **A** and **B**.

## Compare modes

The modes are toggles rather than a list to choose from: turning one on turns
off any other, and turning it off again leaves the **A** file on its own. There
is no separate mode for showing **A** — that is what no comparison looks like.

| Mode           | What it shows                                                          | Shortcut          |
| -------------- | --------------------------------------------------------------------- | ----------------- |
| **B**          | Only the **B** file                                                    | <kbd>Ctrl+B</kbd> |
| **Wipe**       | A wipe between **A** and **B** (drag with **Alt** + left mouse button) | <kbd>Ctrl+W</kbd> |
| **Overlay**    | **B** layered on top of **A**                                          |                   |
| **Difference** | The pixel difference between **A** and **B**                           |                   |
| **Horizontal** | **A** and **B** side by side                                           |                   |
| **Vertical**   | **A** above **B**                                                      |                   |
| **Tile**       | **A** and **B** as tiles (supports multiple **B** files)               | <kbd>Ctrl+T</kbd> |

The modes without a shortcut can be given one in the **Keyboard Shortcuts**
section of the **Settings** tool.

## Difference gain

**Difference** shows how far apart the two files are at each pixel, and the
differences worth looking for are often very small — a compressed version
differs from its source by a code value or two, which is not distinguishable
from black. **Gain** multiplies the difference so that it can be seen; at 16 a
difference of one code value in eight bits reads as a clear grey.

The brightness is the size of the difference and the colour is which channels
it is in, so a gain high enough to make the small differences visible drives
the large ones to white. Work down from a high gain to find where something
differs, then back off to judge by how much.

## Same size

Files of different resolutions are drawn at the size of the current file, so a
smaller one is not shown tiny beside it. Turn **Same size** off to draw each file
at its own size.

This is what the view is describing while it is on. With a 1920x1080 **A** and a
1280x720 **B**, the frame really is 1920x1080 whichever file is on screen, and
the **View Zoom** and **Render** items in the HUD report that frame rather than
either file.

Locations: **Files** tool

## Sync by

Which frame of each file is shown together:

- **Start of File** — The **B** file is offset so its start aligns with the start of **A**.
- **Timecode** — **A** and **B** play at the same timeline time.

Files that start at different timecodes show black in **B** until this is set to
**Start of File**.

Locations: **Compare** menu, **Files** tool

## Comparing multiple layers

Tile mode supports multiple **B** files, which makes it handy for viewing several
layers of a single file at once. Open the file multiple times and set a different
active layer for each instance, then enable **Tile** compare mode and add the
other instances as **B** files.

![Tile mode]({{ '/assets/compare-tile.svg' | relative_url }})

1. Set the compare mode to **Tile**
2. Set the current file and its layer
3. Add the **B** files and pick a layer for each

## Comparing file information

The **Information** tool and the HUD describe the **A** file — the current one —
whatever a comparison is showing. A comparison changes which pixels are drawn,
not which file is current, so switching between **B** and a wipe does not change
what they report.

To compare what two files are rather than how they look, open them and switch
between them with **File/Next** and **File/Previous** (<kbd>Ctrl+Page Down</kbd>
and <kbd>Ctrl+Page Up</kbd>), or click their tabs. Each becomes the current file
in turn, and every display follows it.
