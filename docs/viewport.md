---
title: Viewport
layout: default
nav_order: 5
---

# Viewport

The viewport displays the current file. You can pan, zoom, or *frame* the view
so the image fills the available space.

Default viewport controls:

- **Pan** — Middle mouse button
- **Zoom** — Mouse wheel, or the **-** and **=** keys
- **Frame view** (fit image to viewport) — **Backspace**
- **Wipe** (in compare mode) — **Alt** + left mouse button
- **Color picker** — **Ctrl** + left mouse button
- **Frame shuttle** (scrub frames by dragging) — Left mouse button

Viewport controls can be remapped in the **Settings** tool.

## Color buffer

The viewport's bit depth can be configured, though rhe default, **RGBA F32**,
is recommended because it preserves the full range of color values without
clamping. Lower bit-depth options can be faster, but they can clamp colors —
choose them only when performance is more important than precision.

Locations: **View** tool

## Background

The viewport background color can be customized.

Locations: **View** tool

## Grid

A grid overlay can be shown to identify areas in the viewport.

Locations: **View** tool

Grid options:

- **Enabled** — Toggle the grid on or off
- **Cell mode** - Grid cell modes
- **Cell size** — Cell size in pixels
- **Cell count** - Number of cells
- **Line width** — Thickness of the grid lines
- **Color** — Color of the grid lines
- **Labels** — How cells are labeled:
  - **None** — No labels
  - **Pixels** — Pixel positions
  - **Alphanumeric** — Letters along X, numbers along Y (e.g., "B 12")
- **Text color** — Label text color
- **Overlay color** — Label background color

![View pixel grid]({{ '/assets/ViewPixelGridAnnotated.svg' | relative_url }})

The grid can also help you inspect individual pixels. To do so:

1. Set **Magnify** to **Nearest**
2. Enable the grid
3. Set the grid size to `1`

## Outline

An outline can be draw around the image to make transparent regions easier to
distinguish from the background.

Locations: **View** tool

## HUD

The heads-up display (HUD) overlays useful information on top of the viewport.

Locations: **View** menu

![Viewport HUD]({{ '/assets/ViewHUDAnnotated.svg' | relative_url }})

1. Current file name
2. Current frame
3. Actual playback speed (the rate DJV is achieving, which may differ from the requested rate)
4. Number of frames dropped during playback
5. Color picker
6. Video cache fill (%)
7. Audio cache fill (%)
