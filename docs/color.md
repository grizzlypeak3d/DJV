---
title: Color
layout: default
nav_order: 9
---

# Color

The **Color** tool covers everything color-related: OpenColorIO settings, LUTs
(look-up tables), and image adjustments such as brightness, contrast, levels,
and exposure.

Locations: **Tools** menu, **Tools** toolbar

Shortcut: <kbd>F4</kbd>

The tool is divided into collapsible sections — **OCIO**, **LUT**, **Color**,
**Levels**, **Exposure**, and **Soft Clip** — and each can be turned on or off
independently using the checkbox in its header. This makes it easy to compare
the image with and without a given adjustment.

{: .note }
When any of the color controls are active, a status bar indicator is shown so
the adjustment isn't forgotten about.

## OpenColorIO (OCIO)

![Color tool]({{ '/assets/ColorToolAnnotated.svg' | relative_url }})

1. Enable OpenColorIO
2. OpenColorIO configuration
3. Configuration name
4. Input color space
5. Display color space
6. View color space
7. Look color space

The OpenColorIO configuration can be set to a built-in configuration, the
**OCIO** environment variable, or a specific file path.

Shortcuts:

* Toggled enabled: <kbd>Ctrl+N</kbd>


## LUT

A LUT can be applied either before or after the OpenColorIO pass — set the LUT
**Order** option to **PreColorConfig** or **PostColorConfig** as needed. The LUT
can be toggled with <kbd>Ctrl+K</kbd>.

### Supported LUT formats

The following LUT formats are supported:

* flame: .3dl
* lustre: .3dl
* ColorCorrection: .cc
* ColorCorrectionCollection: .ccc
* ColorDecisionList: .cdl
* Academy/ASC Common LUT Format: .clf
* Color Transform Format: .ctf
* cinespace: .csp
* Discreet 1D LUT: .lut
* houdini: .lut
* International Color Consortium profile: .icc
* Image Color Matching profile: .icm
* ICC profile: .pf
* iridas_cube: .cube
* iridas_itx: .itx
* iridas_look: .look
* pandora_mga: .mga
* pandora_m3d: .m3d
* resolve_cube: .cube
* spi1d: .spi1d
* spi3d: .spi3d
* spimtx: .spimtx
* truelight: .cub
* nukevf: .vf

Different formats may be available depending on how DJV was built.

## Color

Basic color adjustments:

- **Add** — Add a constant value to each channel.
- **Brightness** — Scale the overall brightness.
- **Contrast** — Increase or decrease contrast around the midpoint.
- **Saturation** — Adjust color intensity, from grayscale up to oversaturated.
- **Hue** — Rotate the hue.
- **Invert** — Invert the image colors.

## Levels

Levels remap the input tonal range to an output range, similar to the levels
control in an image editor:

- **In low** / **In high** — The black and white points of the input.
- **Gamma** — A midtone adjustment applied between the input and output ranges.
- **Out low** / **Out high** — The black and white points of the output.

## Exposure

Exposure controls mimic film response and are useful for high-dynamic-range
images:

- **Exposure** — Overall exposure, in stops.
- **Defog** — Subtract a fog (haze) value.
- **Knee low** / **Knee high** — The range over which the highlight roll-off (the
  "knee") is applied.
- **Gamma** — A final gamma adjustment.

## Soft clip

**Soft clip** gently rolls off values approaching white instead of clipping them
hard, which helps preserve detail in bright highlights.
