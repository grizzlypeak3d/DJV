---
title: Exporting files
layout: default
nav_order: 12
---

# Exporting files

The **Export** tool writes out the current file as an image sequence, a movie, or
a single still image.

Locations: **Tools** menu, **Tools** toolbar

Shortcut: <kbd>F2</kbd>

![Export tool]({{ '/assets/export-tool.svg' | relative_url }})

* Render size — The resolution of the export. **Default** uses the source
  resolution; a custom size can also be set.

## File types

Set **File type** to choose what is exported:

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
