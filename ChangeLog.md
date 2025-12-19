## 3.3.0

Features:
* The built-in file browser can now display image sequences as a single item
  with frame range.
* The command line now accepts file sequences like "render.#.exr".
* The tab key can now be assigned to shortcuts (e.g., Ctrl+Tab and
  Ctrl+Shit+Tab for changing the current file).
* Add the "Extra 1" and "Extra 2" buttons to the mouse settings.
* Library updates:
    - SDL2 release-2.32.10

Fixes:
* Fix compare overlay and difference flipping the image.
* Fix middle and right mouse buttons that were swapped in the settings.
* Fix the mouse settings to only show the "Command" key on macOS.
* Fix for the clipboard that was causing excessive CPU usage on Gnome.
* Fix for compiling on Rocky 8.


## 3.2.1

Fixes:
* macOS package fixes.


## 3.2.0

Features:
* SDL2 support for window creation and input handling.
* Support for OpenImageIO.
* Signed macOS packages.
* Library updates:
    - tlRender 0.13.0
    - feather-tk 0.5.0
    - OpenTimelineIO v0.18.0
    - OpenColorIO v2.5.0
    - OpenEXR v3.3.6
    - Imath 3.2.1
    - FFmpeg 8.0
    - USD v25.11
    - libpng v1.6.50
    - libjpeg-turbo 3.1.1
    - OpenSSL 3.5.2
    - curl 8_15_0
    - freetype 2-14-1
    - minizip 4.0.10

Fixes:
* Fix application closing when window is minimized.
* Fix outline with alpha blend modes.
* Fix outline rendering underneath grid.
* Fix display scale values.
* Add the units to the audio sync offset label.
* Fix opening file associations on macOS.
* Fixes for keyboard mapping and number pad.
* Fixes for building with CMake 4.x versions.
* Fix for cached frames wrapping around the timeline.


## 3.1.1

Features:
* Make the timeline minimized by default.
* Add sample data for testing.

Fixes:
* Make the default color buffer RGBA_F32.
* More fixes for OpenColorIO settings.


## 3.1.0

Features:
* Add support for the OCIO environment variable and built-in OCIO configurations.
* Add more speed options.
* Add buttons to clear keyboard shortcuts.
* Add a setting for whether the file browser shows hidden files.
* Add position and zoom controls to the view tool, and a zoom control to the tool bar.
* Make the timeline ticks more visible.
* Add a keyboard shortcut (Ctrl+G) for enabling the grid.
* Add a keyboard shortcut (Ctrl+N) for toggling whether OCIO is enabled.
* Add a keyboard shortcut (Ctrl+K) for toggling whether the LUT is enabled.
* Add a Windows NSIS package.
* Use the current playback speed and layer when exporting.
* Add a setting for which mouse button is used for panning, compare wipe, color picker, and frame shuttle.
* Add a setting for scaling the mouse wheel zoom in the viewport and timeline.
* Add a button to manually save the settings.

Fixes:
* Fix for image sequences with uppercase file extensions.
* Fix for building with BMD support.
* Fix for OCIO settings not saving.
* Fix the timeline duration on the bottom tool bar to show the in/out range.
