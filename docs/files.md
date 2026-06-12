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

## Opening files

You can open files and folders in three ways:

- From the **File** menu or **File** tool bar
- By dragging and dropping onto the main window
- From the command line

Opening a folder opens every supported file in that folder (non-recursively).

Image sequences can be opened from the command line by either specifying the
first frame or using the "#" wildcard. For example:

```
djv render.#.exr
```

The native file browser is enabled by default on Windows and macOS. To use DJV's
built-in file browser instead, change the option in the **Settings** tool.

To switch between open files, use the **File/Current** menu, the **Tab Bar**, or
the **Files** tool.

## FFmpeg command

To support additional formats and codecs, you can configure DJV to use an
external FFmpeg command. FFmpeg runs as a sub-process and streams decoded video
and audio to DJV for display.

The paths to the **ffmpeg** and **ffprobe** commands are set in the **Settings**
tool.

![FFmpeg command]({{ '/assets/FFmpegCommandAnnotated.svg' | relative_url }})

1. ffmpeg command location (click to show full path)
2. ffprobe command location (click to show full path)
3. Click to show file browser

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

## Files tool

The **Files** tool is the central place for managing open files: it sets the
current file, picks active layers, and configures comparison. Open it from the
**Tools** menu or the tool bar.

![Files tool]({{ '/assets/FilesToolAnnotated.svg' | relative_url }})

1. **Current file** (the **A** file)
2. **B files** (multiple **B** files can be set in tile mode)
3. Current layer
4. Compare mode
5. Compare time
6. Compare options

## Image sequences with audio

Audio can be paired with an image sequence either automatically or manually.

To pair audio automatically, open the **Image Sequences** section in the
**Settings** tool. You can either list the file extensions DJV should look for
(for example, `.wav .mp3`), or provide a specific file name to match.

To pair audio manually, use **File/Open With Audio**.

## USD

USD support is currently experimental. When a USD file is opened, DJV renders it
to an image sequence using the Hydra renderer.

DJV picks the rendering camera in this order:

1. The clip name, if the USD file is referenced from an OTIO file
2. The primary camera in the scene
3. The first camera found in the scene
4. A temporary camera generated to frame the scene
