---
title: Color
layout: default
nav_order: 9
---

# Color

The **Color** tool covers everything color-related: OpenColorIO settings, LUTs
(look-up tables), and basic adjustments such as brightness, contrast, and levels.

Open it from the **Tools** menu or the tool bar.

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

A LUT can be applied either before or after the OpenColorIO pass — set the LUT
**Order** option to **PreColorConfig** or **PostColorConfig** as needed.

## LUTs

Supported LUT formats:
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
