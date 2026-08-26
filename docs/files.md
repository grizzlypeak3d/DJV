---
title: Files
layout: default
nav_order: 4
---

# Files

DJV ships with support for the following formats:

- Image sequences: Cineon, DPX, JPEG, OpenEXR, PNG, PPM, SGI, TGA, BMP, TIFF
- Movie codecs: MJPEG, MPEG-2
- Audio codecs: FLAC, MP3, WAV
- Timelines: OTIO, OTIOZ
- Experimental: USD

Different formats may be available depending on how DJV was built, or by using
an external FFmpeg command.

{: .note }
These are the formats DJV reads. For the formats DJV writes, see
[Exporting files]({{ '/export' | relative_url }}).

## Opening files

You can open files and folders in three ways:

- From the **File** menu or **File** tool bar
- By dragging and dropping onto the main window
- From the command line

Opening a folder opens every supported file in that folder (non-recursively).

Shortcuts:

* Open: <kbd>Ctrl+O</kbd>
* Open with audio: <kbd>Ctrl+Shift+O</kbd>
* Reload: <kbd>Ctrl+R</kbd>
* Close: <kbd>Ctrl+E</kbd>
* Close all: <kbd>Ctrl+Shift+E</kbd>


Image sequences can be opened from the command line by either specifying the
first frame or using the "#" wildcard. For example:

```
djv render.#.exr
```

The native file browser is enabled by default on Windows and macOS. To use DJV's
built-in file browser instead, change the option in the **Settings** tool.

### The built-in file browser

The built-in file browser has a panel on the left for navigation and a file list
on the right:

- **Drives** — Mounted drives and volumes.
- **Shortcuts** — Common locations such as the home, Desktop, Documents, and
  Downloads folders.
- **Recent** — Recently visited directories.

Above the file list are buttons to go up a directory, navigate back and forward,
reload, and edit the current path, along with a row of *path buttons* for quickly
jumping to any parent directory. Below the list, a search box filters by name, an
extension menu filters by file type, and the sort menu and direction button order
the results.

Image sequences are shown as a single item with their frame range, so a
multi-thousand-frame render appears as one entry rather than thousands.

![File browser]({{ '/assets/file-browser.svg' | relative_url }})

## Switching files

To switch between open files, use the **File/Current** menu, the **Tab Bar**, or
the **Files** tool.

Locations: **File** menu, **Tab Bar**, **Files** tool

Shortcuts:

* Next file: <kbd>Ctrl+Page Down</kbd>
* Previous file: <kbd>Ctrl+Page Up</kbd>

## Reviews

A *review* is a saved session. Instead of re-opening each file and rebuilding a
comparison by hand, you can save the whole setup to a `.djvr` file and reopen it
later in one step. A review stores:

- The open files, including any separate audio, the active layer, and each
  file's speed, current frame, and in/out range.
- The active tab (the **A** file) and the comparison setup: mode, **B** files,
  wipe, overlay, and the relative/absolute time mode.
- The viewport state: framing, pan, and zoom.
- Color and display state: OCIO, LUT, display, background, foreground, aspect
  ratio, and HUD.
- The interface: the active tool panel and the window layout.

Opening a review replaces the current session. File paths are stored relative to
the `.djvr` when possible, so a review stays valid when moved together with its
media; any files that cannot be found are reported in the **Messages** tool.

A review written by a newer DJV than the one you are running is refused, with
the reason given in the **Messages** tool, rather than opened in part. Anything
else in a review that this version does not understand is left alone: it is
kept as it stands when you save, so passing a session between different
versions of DJV does not quietly strip it. The document itself is described in
[Review file format]({{ site.baseurl }}/review-format.html).

The window title shows the open review's path, with a trailing `*` while it has
unsaved changes. **Close Review** closes the review and returns DJV to its empty
startup state; if there are unsaved changes it prompts to save first, as does
quitting the application.

While a saved review has unsaved changes, DJV keeps a periodic autosave backup.
If the application does not exit cleanly, the next launch offers to recover those
changes.

### Building a review: make "A" the primary source

A review can hold any number of files, but **the "A" file is always the master**.
Set it to the source the review is *about* — the shot being reviewed — and use
the "B" files for what you compare it against.

This is not a stylistic preference: "A" drives playback and timing. The player is
built from "A", so its duration, frame rate and audio govern the session; the
timeline shows "A"; comparison times are resolved relative to it; and **Fit to A**
scales the other sources to match it. A review whose "A" is a reference still
plays, but every timing decision then follows the wrong source.

How many "B" files you may add depends on the comparison mode:

| Mode | "B" files |
|---|---|
| A, B, Wipe, Overlay, Difference, Horizontal, Vertical | one |
| Tile | any number |

Selecting a second "B" in a single-buffer mode replaces the first, and switching
from **Tile** to any other mode keeps only one "B". This is by design, and it is
why reviews are saved with the mode: reopening a tiled review with four sources
restores all of them, while the single-buffer modes restore one. In every mode
"A" is restored exactly as it was saved.

The recommended way to build a review:

1. Open the primary source first, so it becomes "A".
2. Open the sources to compare against, and mark them as "B".
3. Choose the comparison mode — **Tile** if you need more than two sources.
4. Save the review.

Locations: **File** menu

Shortcuts:

* Open review: <kbd>Ctrl+Shift+O</kbd>
* Save review: <kbd>Ctrl+Shift+S</kbd>

Reviews can also be opened from the command line:

```
djv session.djvr
```

Personal preferences — keyboard shortcuts, style, cache size, and the audio
device — are deliberately **not** stored in a review, so opening one received
from someone else does not reconfigure your installation.

Notes and drawings record who made them, taken from your account name on the
machine, so a session that has been passed around stays readable.

## Annotating a review

The **Review** tool holds everything you add on top of the footage: drawings and
notes, in one panel so that marking up a frame and commenting on it stay
visible together.

Locations: **Tools** menu, **Tools** toolbar

Shortcut: <kbd>F9</kbd>

### Drawing

Pick the **pen** or the **eraser** in the *Drawing* section to start drawing;
click the active tool again to stop. While drawing is on, dragging with the left
mouse button draws on the image instead of shuttling frames — panning (middle
button) and zooming (wheel) are unchanged.

* **Colour** — click the swatch to change it.
* **Size** — the stroke width, in pixels of the source image. A stroke therefore
  keeps its position and its weight whatever the zoom, the pan, or the
  comparison mode, and stays crisp when you zoom in.
* **Eraser** — removes the whole stroke it touches, not part of it.
* **Undo** / **Redo** — <kbd>Ctrl+Z</kbd> and <kbd>Ctrl+Shift+Z</kbd>, several
  levels deep. While you are writing a note, <kbd>Ctrl+Z</kbd> undoes your text
  instead.
* **Clear Frame** — removes every stroke on the current frame.

A drawing belongs to one source and appears on **one frame**. In a comparison
you can draw on either side, and each stroke stays attached to its own source.

### Notes

Write in the *Notes* section and press **Publish Note**: the note records the
frame you were on and the time you published it. Each card shows its frame as a
button that takes you back there, and a button to delete the note.

Frames carrying a note or a drawing are marked in the timeline ruler. The marks
are deliberately alike — they say *there is something here*, not what kind.

## FFmpeg plugin

The FFmpeg plugin provides support for movie and audio files. Only a limited
set of codecs is enabled in the open source packages, to enable additional
codecs use the FFmpeg command described below, or build from source.

![FFmpeg plugin]({{ '/assets/ffmpeg-plugin.svg' | relative_url }})

Changes take effect on newly opened files; reload (<kbd>Ctrl+R</kbd>) to apply
them to the current file.

* **Software YUV conversion** - Convert YUV to RGB on the CPU instead of on the
  GPU. Off by default: the decoded YUV frame is uploaded directly and converted
  in the display shader, which is faster and avoids an extra frame copy. Enable
  this to perform the conversion in software before display.
* **Hardware decoding** - Decode video on the GPU (VideoToolbox on macOS,
  Direct3D 11 on Windows) when the file is compatible. Off by default. Files
  that are not compatible will automatically fall back to software decoding.
* **Threads** - The number of threads used for decoding. The default, `0`, lets
  FFmpeg choose automatically based on the system.

## FFmpeg command

To support additional formats and codecs, you can configure DJV to use an
external FFmpeg command. FFmpeg runs as a sub-process and streams decoded video
and audio to DJV for display.

The paths to the **ffmpeg** and **ffprobe** commands are set in the **Settings**
tool.

![FFmpeg command]({{ '/assets/ffmpeg-command.svg' | relative_url }})

Changes take effect on newly opened files; reload (<kbd>Ctrl+R</kbd>) to apply
them to the current file.

* **ffmpeg**, **ffprobe** - set the command location (click to show full path or
click folder icon to open browser)

## Memory cache

DJV caches frames in memory for smooth playback and scrubbing. The cache is
configured in the **Settings** tool, with separate values for video, audio, and
*read-behind*. Read-behind is the number of seconds cached *before* the current
frame, which keeps scrubbing responsive when moving backward.

Only the current file is cached. Switching files clears the cache and reloads it
for the new file.

## Layers

For files with multiple layers (such as multi-part OpenEXR), the active layer
can be changed from the **File/Layers** menu or from the **Files** tool.

Locations: **File** menu, **Files** tool

Shortcuts:

* Next layer: <kbd>Ctrl+Equals</kbd>
* Previous layer: <kbd>Ctrl+Minus</kbd>

## Files tool

The **Files** tool is the central place for managing open files: it sets the
current file, picks active layers, and configures comparison.

Locations: **Tools** menu, **Tools** toolbar

Shortcut: <kbd>F1</kbd>

![Files tool]({{ '/assets/files-tool.svg' | relative_url }})

* Current file (the **A** file)
* **B** files (multiple **B** files can be set in tile mode)

## Image sequences with audio

Audio can be paired with an image sequence either automatically or manually.

To pair audio automatically, open the **Image Sequences** section in the
**Settings** tool. You can either list the file extensions DJV should look for
(for example, `.wav .mp3`), or provide a specific file name to match.

To pair audio manually, use the menu **File/Open With Audio**.

## OTIO spatial coordinates

OTIO files can give each clip a bounding box, called the
[spatial coordinates](https://opentimelineio.readthedocs.io/en/stable/tutorials/spatial-coordinates.html){:target="_blank"},
that describes the area the image occupies on a canvas shared by the whole
timeline. DJV uses these boxes to size and position the images.

The most common use is playing back clips that were rendered at different
resolutions. Give the clips the same box and they are all displayed at the same
size, so the view no longer jumps between them when playback moves from one clip
to the next. This also keeps the zoom and pan steady across a cut, which is
useful for checking continuity between shots.

The coordinates are unit-less, so boxes of `0, 0, 1920, 1080` and `0, 0, 16, 9`
describe the same area. DJV works out the size in pixels from the first clip
that has coordinates, together with the resolution the timeline is working at.
The Y axis points up, unlike image coordinates, so a clip with a larger Y value
is displayed higher.

{: .note }
Clips are not required to have spatial coordinates. In a timeline where only
some clips have them, the clips without them fill the canvas, and so are
displayed at the same size as the clips that do.

Clips can also be given different boxes to place them side by side, or to
position a smaller image within a larger frame.

The **OTIO** section in the **Settings** tool controls how the coordinates are
used:

- **None** — Ignore the spatial coordinates. Clips are sized from their own
  resolution.
- **Coordinates** — Use the spatial coordinates where clips provide them. This
  is the default.
- **Normalize** — Use the spatial coordinates, and display clips that do not
  have them at the size of the first clip. Use this to play clips of differing
  resolutions at the same size when the timeline was not authored with spatial
  coordinates.

{: .note }
The color picker and magnifier report positions in the original image, not the
canvas, so the pixel coordinates always refer to the media itself.

The authored coordinates are shown in the **Information** tool, along with the
canvas DJV derived from them.

## USD

USD support is currently experimental. When a USD file is opened, DJV renders it
to an image sequence using the Hydra renderer.

DJV picks the rendering camera in this order:

1. The clip name, if the USD file is referenced from an OTIO file
2. The primary camera in the scene
3. The first camera found in the scene
4. A temporary camera generated to frame the scene
