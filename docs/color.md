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
