---
title: Exporting files
layout: default
nav_order: 12
---

# Exporting files

The **Export** tool writes out the current file as an image sequence, a movie, or
a single still image.

Open it from the **Tools** menu or the tool bar.

![Export tool]({{ '/assets/export-tool.svg' | relative_url }})

1. Output directory
2. Render size — the resolution of the export. **Default** uses the source
   resolution; a custom size can also be set.
3. File type
4. Base file name
5. File extension
6. Movie codec
7. Export button

## File types

Set **File type** to choose what is written:

- **Image** — Just the current frame, as a single still image.
- **Sequence** — The in/out range as a numbered image sequence. Set **Zero
  padding** to control the number of digits in the frame numbers (for example, a
  padding of `4` produces `render.0001.exr`).
- **Movie** — The in/out range as a movie file, encoded with the selected
  **Codec**.

Available extensions depend on how DJV was built. Image and sequence exports
typically support `.exr`, `.png`, `.tif`, and `.tiff`; movie exports typically
support `.mov`, `.mp4`, and `.m4v`.

Exports respect the current layer, playback speed, in/out range, and color
settings.

{: .note }
Audio export is not yet supported.
