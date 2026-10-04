# GPU renderer spike

Branch `gpu-spike` in DJV, tlRender and feather-tk. A second renderer, on SDL's
GPU API (Metal on macOS, Vulkan elsewhere), beside the OpenGL one. OpenGL and
OpenGL ES stay: the Raspberry Pi and the web have nothing else. What this is
for is what OpenGL cannot give on Linux and Windows: an HDR swapchain.

## State

Works on macOS/Metal, checked against the OpenGL renderer by screenshot:
the feather-tk user interface, and DJV with images, EXR, video, timelines,
every compare mode, OCIO, LUTs, playback and export.

Works on Linux/Vulkan, on one machine: Mesa 26.0's RADV on a Radeon RX 480,
GNOME 50 on Wayland, an LG C2 with HDR on. `ftk-gpu-test`, `tl-gpu-ocio-test`
and `tl-gpu-hdr-test` pass, and DJV matches the OpenGL renderer by screenshot
with images, EXR, video (sixteen bit included), timelines, every compare mode,
OCIO and a LUT: the same 0.2% of channels at the edges of glyphs as on macOS.
The user interface and video look right on the display, in an HDR10 swapchain.
The API's validation (`vulkan-validationlayers`) has nothing to say of any of
it but one warning, below.

## Building

In `etc/Config/local.cmake`:

    set(ftk_GPU ON CACHE BOOL "")
    set(TLRENDER_GPU ON CACHE BOOL "")

and the super build as usual. On this branch it builds SDL with its Vulkan
and Metal drivers, and glslang (not on macOS, where nothing needs it; set
`ftk_glslang` to check the GLSL there). An existing build tree rebuilds SDL.

An existing build tree also has the two options in its cache already, as
OFF, and `local.cmake` does not overwrite what is there. Say them to it once:

    cmake -S DJV -B build-Debug -Dftk_GPU=ON -DTLRENDER_GPU=ON

`ftk_TESTS` and `TLRENDER_TESTS` build the test programs below, and
`TLRENDER_PROGRAMS` builds `tlbake`.

At run time Vulkan wants the system's loader, `libvulkan.so.1`, and a driver.

## Running

Nothing changes unless asked for by name:

| Variable | |
|---|---|
| `FTK_RENDER=gpu` | Draw windows with the GPU renderer. |
| `FTK_GPU_SWAPCHAIN=sdr`, `hdr` or `hdr10` | Ask for a swapchain by name: SDR, extended linear or HDR10. Without it the swapchain follows the display: where the display is showing HDR, extended linear if the window can have it and HDR10 if it cannot. The log says what was got. |
| `FTK_GPU_HDR_TEST=1` | Draw patches at one, two, four and eight times white along the top. |
| `FTK_GPU_DEBUG=1` | Turn on the API's validation. |
| `FTK_GPU_VALIDATE=1` | Compile every shader's GLSL as it is made, whatever the driver. |
| `SDL_GPU_DRIVER=vulkan` | SDL's own: which driver. |

With `-log`, look for `GPU driver:`, `GLSL compiler:`, `Texture formats:`
and `Swapchain:`.

## Checking it

- `ftk-gpu-test [dir]` draws one scene with both renderers and compares them,
  checks the presenter's HDR arithmetic, and presents into each kind of
  swapchain the desktop offers, on a hidden window (a shown one on Vulkan,
  where it also says how fast frames are presented). It reads a buffer back
  as every type of image a file is written from, in every layout.
  `ftk-gpu-test -compare a.png b.png [diff.png]` compares two screenshots.
- `tl-gpu-ocio-test` runs OCIO through a pipeline against OCIO's CPU
  processor: Metal's shader on Metal, and OCIO's Vulkan GLSL through glslang
  elsewhere.
- `tl-gpu-hdr-test`: see HDR, below.
- The Diagnostics tool has `ftk GPU Objects` and `ftk GPU Memory`, beside
  the OpenGL ones, which read zero while the GPU renderer draws.
- Any application, with and without `FTK_RENDER=gpu`, `-screenshot` each,
  and compare. The two are the same but for the edges of glyphs, where
  OpenGL's sixteen bit texture coordinates show: about 0.2% of channels off
  by more than two, none by more than about 32.

      djv file.exr -hideSetup -settingsFile /tmp/a.json -screenshot gl.png
      FTK_RENDER=gpu djv file.exr -hideSetup -settingsFile /tmp/b.json -screenshot gpu.png

## Where it is

- `ftk/GPU`: `System` (the device), `Texture`, `OffscreenBuffer`, `Render`
  (an `IRender`), `Present` (to a swapchain, SDR or HDR), `Shader` (the two
  languages, and glslang).
- `ftk/UI/WindowGL.cpp`: `_updateGPU()`, and a window with no OpenGL context.
- `tlRender/GPU`: `tl::gpu::Render`, a port of `tl::gl::Render`.
- `tlRender/Timeline`: what the two timeline renderers do the same way is
  here once. `IRender::_drawBackground()` and `_drawForeground()` are the
  background and foreground, drawn through `ftk::IRender`, and
  `RenderPrivate.h` has the OpenColorIO configuration, the processors a set
  of options or a LUT comes to, and the keys they are kept by. What a
  renderer makes of a processor, its shader and textures, is its own.
- `tlRender/UI/Viewport.cpp`: `_drawGPU()`.
- `djv/UI/ExportWidget.cpp` and `tlRender/BakeApp`: the export and `tlbake`
  draw with the GPU renderer when the windows do, which is `FTK_RENDER=gpu`
  for both, and with OpenGL otherwise.
- `tlplay` needed nothing: it is feather-tk's windows and tlRender's
  viewport, and with `FTK_RENDER=gpu` it draws as it does with OpenGL.

## How it differs from the OpenGL renderer

- Draws are recorded into a command buffer. Vertices are gathered and sent
  when the frame ends, in a command buffer submitted first; textures are sent
  in command buffers of their own. A texture reused within a frame is
  "cycled" so an earlier draw keeps what it was given.
- There is no bound frame buffer. `pushTarget()` and `popTarget()` draw into
  another buffer for a while; each ends a render pass.
- A buffer's first row is the top one. `drawTexture()` takes its "mirror"
  flag as callers mean it for OpenGL, and turns it around.
- There are no three channel textures: interleaved RGB is given a fourth
  channel as it is sent. Planar RGB and YUV are not, being planes.
- The wipe cuts the picture's rectangle along the line rather than using
  the stencil buffer. There are no depth or stencil targets yet.
- OCIO's Metal shader takes its textures as arguments, so the entry point is
  written from its signature. Its Vulkan GLSL declares them itself, in the
  set and at the bindings asked for.

## Vulkan

What was found on the machine above.

- The window is claimed, and what is drawn is the right way up with the
  right faces. The uniform blocks agree, and so do OCIO's texture bindings.
- Compared with OpenGL by screenshot, beyond what State lists: dissolves, the
  clipping warning, the magnifier, the color picker's tool, the backgrounds,
  the grid and center marker, the HUD, the channels, mirroring, negative,
  exposure, levels, soft clip and the color controls, video levels, alpha
  blending, each color buffer, and each image filter. One thing was wrong
  and is fixed: a picture enlarged with Nearest was drawn with Linear unless
  the filter for reducing was Nearest as well. What is left is in Known
  differences, below.
- Presenting waits for the display: `ftk-gpu-test` presents sixty frames to
  each kind of swapchain at about 62 a second on a 60 Hz display.
- SDL's Vulkan driver has no swapchain texture for a hidden window, so
  `ftk-gpu-test` shows its window there, and a kind of swapchain the desktop
  does not offer is not a failure.
- With `FTK_GPU_DEBUG=1`, one warning, once, when OCIO or a LUT has a three
  dimensional table: `WARNING-VkImageSubresourceRange-layerCount-compatibility`,
  of a barrier SDL's driver makes for the texture. It is SDL's, and about a
  Vulkan feature that is not turned on.

Still to look at:

- Texture formats that Vulkan leaves optional: sixteen bit normalized
  (`R16_UNORM` and its siblings, which video over eight bits uses), and
  linear filtering of thirty-two bit float, which OCIO's tables want. RADV
  has them all. `Texture formats:` in the log says which a device lacks;
  nothing falls back from one, so a driver without it fails. SDL has no way
  to ask about the filtering.
- Other drivers: NVIDIA, Intel, and Windows.

## Known differences from OpenGL

- The edges of glyphs, as above.
- An edge that falls exactly on the centers of a row of pixels: OpenGL draws
  that row at the bottom of a rectangle and not at the top, and Metal the
  other way around, since OpenGL's buffers are the other way up. Framing a
  picture to the view can put its bottom edge there, and the picture is then
  one row shorter than OpenGL draws it. Vulkan does as Metal does: stacked
  vertically, the two pictures of a comparison meet one row away, and the
  center marker is one row higher.
- The same with Nearest: where the center of a pixel falls exactly between
  two texels the two renderers take different ones, which shows as single
  rows and columns a regular distance apart.
- The clipping warning's outline is a pixel away in places. It is a
  threshold, which makes a whole color of a difference too small to see.

## HDR

A window holds what it always has: display encoded for sRGB, with one as
the white of the user interface. Where its swapchain is HDR, what is drawn
above one is brighter than white by the same curve, and `ftk::gpu::Present`
writes it into the swapchain in the swapchain's terms. `ftk::IWindow::getHDR()`
says whether, how far above white the display goes, and what white is in
nits where the system says (macOS does not).

A picture is HDR when it is said to be: **HDR picture** in the View tool,
SDR or PQ, for when the OCIO display is an HDR one. Nothing in OCIO says a
display is PQ, so it is a setting, as the Blackmagic output's is. A PQ
picture is taken into what the window holds as it is drawn there
(`tl::gpu::Render::drawTextureHDR`), so the viewport's buffer keeps the
picture's own code values for the color picker. **HDR white** is the
luminance the window's white stands for where the system does not say; 203
nits, the reference white, by default.

The Color Picker says what a color stands for: **Luminance**, in nits, is
what a PQ picture's code values say, or what an HDR window makes of an SDR
picture, and is a dash for an SDR picture in an SDR window. Its swatch, and
the HUD's, is the color as the picture shows it
(`tl::ui::Viewport::getColorSampleNits()` and `getColorSampleDisplay()`). The
color is read from the viewport's buffer, so with the half float color buffer
a luminance is within about a third of a percent of the code value's.

`tl-gpu-hdr-test` checks the arithmetic: PQ code values drawn into a window
and presented into an HDR10 swapchain come out as they went in, colors
outside Rec. 709 included. It passes on Vulkan. A PQ picture has been looked
at on an HDR display, the LG C2 above in an HDR10 swapchain: Sol Levante,
which is sixteen bit 4:4:4 PQ at 1000 nits, looks as HDR should. **Nothing
has been measured.**

On Linux, as found on the machine above:

- HDR is the desktop's to give, and only a Wayland session gives it: the
  log's `Video driver:` has to say `wayland` (`SDL_VIDEO_DRIVER=wayland`
  asks for it), not `x11`. XWayland offers no HDR swapchain of either kind.
- SDL does report `SDL_PROP_WINDOW_HDR_ENABLED_BOOLEAN` for a Wayland window
  on an HDR display. What Wayland offers is an HDR10 swapchain and no
  extended linear one, so a window that follows the display takes extended
  linear where it can and HDR10 where it cannot. It used to ask for extended
  linear alone, be given SDR, and ask again every frame.
- White: SDL has no answer on Wayland, and says one for every window, which
  is eighty nits. A PQ surface there has its white at 203 nits, which the
  compositor takes to the white of everything else on the display, so that
  is where the presenter puts it: `ftk::gpu::getSDRWhiteLevel()`. The user
  interface is then as bright as the windows beside it.
- The HDR headroom SDL reports there is the compositor's 10000 nits over its
  reference white, about 49: the range of PQ, not what the display can do.
- `FTK_GPU_HDR_TEST=1`: four patches, each visibly brighter than the last,
  the first the white of the interface. Not yet looked at.

The export writes a PQ picture's code values as they are, and says what they
are: the description of the OCIO display where it has a name for it, or the
source's without color management. Sol Levante exported to ProRes 4444 reads
back within a couple of percent of the source's luminance, which is the codec.

Not done: HLG, HDR metadata for the swapchain, anything for the display's own
limits (what is brighter than the display goes is left to it). In the export,
which is waiting: nothing is taken from **HDR picture**, so a PQ picture whose
source describes nothing, or whose OCIO display has no name here, is written
without a description; the matrix of a Rec. 2020 picture is written as the
writer's guess from its size, Rec. 709 or 601, where HDR10 wants Rec. 2020's,
and mastering display and light level metadata are not carried from the
source.

## Writing files

The export and `tlbake` draw with the renderer the windows are drawn with,
so with `FTK_RENDER=gpu` there is no OpenGL in either: no hidden window, and
no context. What OpenGL's `glReadPixels` did for the writers,
`ftk::gpu::OffscreenBuffer::read(const ImageInfo&)` does: the buffer is
drawn into a texture with the components wanted and read back, then laid
out as the file's image is, three channels of four, ten bits packed, rows
from the bottom, aligned, and in the byte order asked for.

Against OpenGL, with `tlbake` and with `Export/Movie`: a picture that was
RGB to begin with is written as the same file, byte for byte, in eight, ten
and sixteen bits, half and float, with OCIO and with a LUT. A TIFF or DPX
differs in the time or the file name it carries and nowhere else. Where the
renderers do arithmetic, they round apart by one code value in places: a YUV
source, a dissolve, a picture that is scaled. Sol Levante to sixteen bit
PNG differs by one in 65535 in 0.05% of components.

Without a desktop both renderers want `SDL_VIDEODRIVER=offscreen`, and both
then work.

A movie is written from sixteen bits where the picture has more than eight.
The FFmpeg writer takes eight and sixteen bit RGB, and any other picture,
which is every YUV one and every floating point one, was written from eight
bit RGBA whatever it held: Sol Levante's twelve bits went to APV through
eight. The PNG and OpenImageIO writers did the same with what they do not
take, and now do as it does (`tl::getWriteType()`). And sixteen bit RGB is brought down by 255/256 on its way to YUV,
since FFmpeg's scaler takes its white for 65280: it came out 0.39% high in
code value, about 3% in luminance for PQ, with white over the top of the
range. One frame of Sol Levante written losslessly at ten bits is now 30 of
65535 from the source on average, where it was 70 from eight bits and 86,
all of it one way, from sixteen uncorrected.

The same scaler read YUV with alpha, and ten bit and deeper YUV when the
conversion is asked for on the CPU, to sixteen bit RGB that was 0.39% low,
and widened ten and twelve bit alpha by shifting it, so that opaque was
65472. The reader brings both up to where they belong. A picture written
from sixteen bits with alpha used to read back as it was written only
because the two were wrong by the same amount.

`Export/Movie` from the command line used to fail with the OpenGL renderer,
"Cannot create color texture": the window's context is current once the
window has been drawn, and a command given at startup can come first. The
window now makes it current for an export it is asked for.

## Not done

- Direct3D 12, which SDL also has, and which would want HLSL.
