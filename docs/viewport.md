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

Viewport controls can be remapped in the **Mouse** section of the **Settings**
tool.

## Framing and zoom

The view can be framed (fit to the viewport), reset to 1:1, or zoomed in and
out from the **View** menu and the zoom control on the **View** tool bar.

Locations: **View** menu, **View** tool bar, **View** tool

## Color channels

Individual color channels can be isolated to inspect them.

Locations: **View** menu

Shortcuts:

- **Red** — <kbd>R</kbd>
- **Green** — <kbd>G</kbd>
- **Blue** — <kbd>B</kbd>
- **Alpha** — <kbd>A</kbd>

## Mirror

The image can be mirrored horizontally or vertically.

Locations: **View** menu

Shortcuts:

- **Mirror horizontal** — <kbd>H</kbd>
- **Mirror vertical** — <kbd>V</kbd>

## Position and zoom

The **Position and Zoom** section of the **View** tool shows the exact view
position and zoom level, and lets you set them numerically.

Locations: **View** tool

## Options

The **Options** section of the **View** tool controls how the image is sampled
and displayed:

- **Minify** — Filter used when the image is scaled down (**Nearest** or
  **Linear**).
- **Magnify** — Filter used when the image is scaled up. Use **Nearest** to see
  individual pixels without smoothing.
- **Video levels** — How video levels are interpreted: from the file, full
  range, or legal range.
- **Alpha blend** — How the alpha channel is blended: none, straight, or
  pre-multiplied.
- **Color buffer** — The viewport's bit depth. The default, **RGBA F32**, is
  recommended because it preserves the full range of color values without
  clamping. Lower bit-depth options can be faster, but they can clamp colors —
  choose them only when performance is more important than precision.

## Aspect ratio

The pixel aspect ratio can be left at the file's default or overridden. Up to
three custom aspect ratios can be defined.

Locations: **View** menu, **View** tool

## Background

The viewport background color can be customized.

Locations: **View** tool

## Grid

A grid overlay can be shown to identify areas in the viewport.

Locations: **View** menu, **View** tool

Shortcut: <kbd>Ctrl+G</kbd>

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

An outline can be drawn around the image to make transparent regions easier to
distinguish from the background.

Locations: **View** tool

## Center marker

A marker can be shown at the center of the viewport to help with alignment and
framing. Its size, line width, and color can be customized.

Locations: **View** menu, **View** tool

## HUD

The heads-up display (HUD) overlays useful information on top of the viewport.

Locations: **View** menu

Shortcut: <kbd>Ctrl+H</kbd>

![Viewport HUD]({{ '/assets/ViewHUDAnnotated.svg' | relative_url }})

1. Current file name
2. Current frame
3. Actual playback speed (the rate DJV is achieving, which may differ from the requested rate)
4. Number of frames dropped during playback
5. Color picker
6. Video cache fill (%)
7. Audio cache fill (%)
