---
title: A/B comparison
layout: default
nav_order: 8
---

# A/B comparison

A/B comparison lets you view two files (or layers) at once — useful for checking
revisions, matching color between shots, or spotting differences between renders.

## Setting up a comparison

1. Open the files in DJV.
2. The current file is the **A** file. Set the **B** file from the **Compare/B**
   menu or the **Files** tool.
3. Pick a compare mode (see below).

Locations: **Compare** menu, **Compare** tool bar, **Files** tool

Shortcuts:

* Next B file: <kbd>Shift+Page Down</kbd>
* Previous B file: <kbd>Shift+Page Up</kbd>

{: .note }
Drag with **Alt** + left mouse button to move the wipe between **A** and **B**.

## Compare modes

| Mode           | What it shows                                                          | Shortcut          |
| -------------- | --------------------------------------------------------------------- | ----------------- |
| **A**          | Only the **A** file                                                    | <kbd>Ctrl+A</kbd> |
| **B**          | Only the **B** file                                                    | <kbd>Ctrl+B</kbd> |
| **Wipe**       | A wipe between **A** and **B** (drag with **Alt** + left mouse button) | <kbd>Ctrl+W</kbd> |
| **Overlay**    | **B** layered on top of **A**                                          |                   |
| **Difference** | The pixel difference between **A** and **B**                           |                   |
| **Horizontal** | **A** and **B** side by side                                           |                   |
| **Vertical**   | **A** above **B**                                                      |                   |
| **Tile**       | **A** and **B** as tiles (supports multiple **B** files)               | <kbd>Ctrl+T</kbd> |

Use <kbd>Alt+A</kbd> to toggle between the **A** and **B** modes.

## Compare time

Files can be compared in either *relative* or *absolute* time:

- **Relative** — The **B** file is offset so its start aligns with the start of **A**.
- **Absolute** — **A** and **B** play at the same timeline time.

## Comparing multiple layers

Tile mode supports multiple **B** files, which makes it handy for viewing several
layers of a single file at once. Open the file multiple times and set a different
active layer for each instance, then enable **Tile** compare mode and add the
other instances as **B** files.

![Tile mode]({{ '/assets/compare-tile.svg' | relative_url }})

1. Set the compare mode to **Tile**
2. Set the current file and its layer
3. Add the **B** files and pick a layer for each
