# GPU renderer spike

Branch `gpu-spike` in DJV, tlRender and feather-tk. A second renderer, on SDL's
GPU API (Metal on macOS, Vulkan elsewhere), beside the OpenGL one. OpenGL and
OpenGL ES stay: the Raspberry Pi and the web have nothing else. What this is
for is what OpenGL cannot give on Linux and Windows: an HDR swapchain.

## State

Works on macOS/Metal, checked against the OpenGL renderer by screenshot:
the feather-tk user interface, and DJV with images, EXR, video, timelines,
every compare mode, OCIO, LUTs, playback and export.

**Vulkan has never run.** Every shader has a GLSL version and all of them
compile to SPIR-V with glslang, OpenColorIO's included. Nothing past that has
been tried: there is no Vulkan on the machine this was written on.

## Building

In `etc/Config/local.cmake`:

    set(ftk_GPU ON CACHE BOOL "")
    set(TLRENDER_GPU ON CACHE BOOL "")

and the super build as usual. On this branch it builds SDL with its Vulkan
and Metal drivers, and glslang (not on macOS, where nothing needs it; set
`ftk_glslang` to check the GLSL there). An existing build tree rebuilds SDL.

At run time Vulkan wants the system's loader, `libvulkan.so.1`, and a driver.

## Running

Nothing changes unless asked for by name:

| Variable | |
|---|---|
| `FTK_RENDER=gpu` | Draw windows with the GPU renderer. |
| `FTK_GPU_SWAPCHAIN=sdr`, `hdr` or `hdr10` | Ask for a swapchain by name: SDR, extended linear or HDR10. Without it the swapchain follows the display, extended linear where the display is showing HDR. The log says what was got. |
| `FTK_GPU_HDR_TEST=1` | Draw patches at one, two, four and eight times white along the top. |
| `FTK_GPU_DEBUG=1` | Turn on the API's validation. |
| `FTK_GPU_VALIDATE=1` | Compile every shader's GLSL as it is made, whatever the driver. |
| `SDL_GPU_DRIVER=vulkan` | SDL's own: which driver. |

With `-log`, look for `GPU driver:`, `GLSL compiler:` and `Swapchain:`.

## Checking it

- `ftk-gpu-test [dir]` draws one scene with both renderers and compares them,
  checks the presenter's HDR arithmetic, and presents into each kind of
  swapchain on a hidden window. `ftk-gpu-test -compare a.png b.png [diff.png]`
  compares two screenshots.
- `tl-gpu-ocio-test` runs OCIO through a pipeline against OCIO's CPU
  processor. It is Metal only as written.
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
- `tlRender/UI/Viewport.cpp`: `_drawGPU()`.
- `djv/UI/ExportWidget.cpp`: the export keeps drawing with OpenGL, in a
  context of its own when the window has none.

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

## What to look at first on Vulkan

None of these has been seen to work or to fail.

- Whether a window is claimed: it is made with `SDL_WINDOW_VULKAN` when the
  driver is Vulkan, on the expectation that SDL wants it.
- Which way up, and which faces are culled. SDL says the conventions are the
  same on every driver; the pipelines cull back faces, counter clockwise
  being the front.
- The uniform blocks, which are C structs laid out for Metal and declared
  std140 in the GLSL. They should agree; `DisplayUniforms` is the one with
  the most in it.
- Texture formats that Vulkan leaves optional: sixteen bit normalized
  (`R16_UNORM` and its siblings, which video over eight bits uses), and
  linear filtering of thirty-two bit float, which OCIO's tables want.
- OCIO's texture bindings: the display shader has up to three stages, each
  numbered on from the last.
- The swapchain: what `Swapchain:` says the desktop offers, and whether
  presenting waits for the display as it should.

## Known differences from OpenGL

- The edges of glyphs, as above.
- An edge that falls exactly on the centers of a row of pixels: OpenGL draws
  that row at the bottom of a rectangle and not at the top, and Metal the
  other way around, since OpenGL's buffers are the other way up. Framing a
  picture to the view can put its bottom edge there, and the picture is then
  one row shorter than OpenGL draws it.

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

`tl-gpu-hdr-test` checks the arithmetic: PQ code values drawn into a window
and presented into an HDR10 swapchain come out as they went in, colors
outside Rec. 709 included. **Nobody has yet looked at it on an HDR display.**

On Linux, none of which has been tried:

- HDR is the desktop's to give, and only a Wayland session gives it: the
  log's `Video driver:` has to say `wayland` (`SDL_VIDEO_DRIVER=wayland`
  asks for it), not `x11`.
- The swapchain follows `SDL_PROP_WINDOW_HDR_ENABLED_BOOLEAN`. If SDL does
  not report that for a Wayland window, `Swapchain:` will say SDR on an HDR
  desktop: `FTK_GPU_SWAPCHAIN=hdr10` or `hdr` asks by name, and whether that
  is supported is the driver's answer, which wants a recent Mesa or NVIDIA
  driver.
- White: off macOS the window's white is taken from SDL's SDR white level,
  in units of eighty nits. A PQ picture goes through an HDR10 swapchain
  unchanged whatever that says, since it is divided by it and multiplied
  back, but the user interface is drawn at it, so if SDL has no answer and
  says one the interface is at eighty nits and looks dim.
- `FTK_GPU_HDR_TEST=1` with any ftk example is the first thing to look at:
  four patches, each visibly brighter than the last, the first the white of
  the interface.

Not done: HLG, HDR metadata for the swapchain, anything for the display's
own limits (what is brighter than the display goes is left to it), and the
color picker and export know nothing of it.

## Not done

- `tlbake` and `tlplay`, and the export on the GPU renderer itself.
- The OCIO configuration code, and the background and foreground drawing,
  are copies of the OpenGL renderer's and want to be shared.
- Direct3D 12, which SDL also has, and which would want HLSL.
