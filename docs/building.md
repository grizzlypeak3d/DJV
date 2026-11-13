# Building from source

DJV is built with CMake. A *super build* is provided that builds the dependencies and then DJV, driven by a script per platform; it is how the packages are made. The dependencies can also come from the system's packages, on the distributions that carry them.

Build options go in `etc/Config/local.cmake`, which is not tracked. For example:

```
# Enable full codec support.
set(TLRENDER_FFMPEG_MINIMAL OFF CACHE BOOL "")

# Enable USD (Universal Scene Description) support. Building USD also
# requires Python 3.
set(TLRENDER_USD ON CACHE BOOL "")
```

For the number of build jobs, export `CMAKE_BUILD_PARALLEL_LEVEL`.

## Linux

Requirements: Git and CMake 3.31.

### Debian and Ubuntu

Install the system packages:

```
sudo apt-get install build-essential git cmake xorg-dev libglu1-mesa-dev mesa-common-dev mesa-utils libasound2-dev libpulse-dev libva-dev libdrm-dev libwayland-dev wayland-protocols libxkbcommon-dev libegl1-mesa-dev libdecor-0-dev
```

The Wayland packages are optional; without them SDL is built with X11 only and runs through XWayland on a Wayland desktop.

### Rocky 9

```
sudo dnf install git libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel mesa-libGL-devel pipewire-devel libva-devel libdrm-devel wayland-devel wayland-protocols-devel libxkbcommon-devel mesa-libEGL-devel libdecor-devel
```

Rocky 9 packages CMake 3.26, and OpenTimelineIO needs 3.31. A newer one can be installed with pip; the cmake.org `.sh` file is an installer that takes a prefix, and either way the result has to be on the path ahead of the system's:

```
pip3 install --user "cmake<4"
export PATH=$HOME/.local/bin:$PATH
```

### Rocky 8

```
sudo dnf install git libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel mesa-libGL-devel pipewire-devel libva-devel libdrm-devel wayland-devel wayland-protocols-devel libxkbcommon-devel mesa-libEGL-devel
```

Rocky 8 also needs a newer compiler, installed and then enabled for the shell that builds:

```
sudo dnf install gcc-toolset-13
scl enable gcc-toolset-13 bash
```

NVIDIA hardware decoding loads the driver's CUDA library at run time. On RHEL family systems that is a separate package:

```
sudo dnf install nvidia-driver-cuda-libs
```

### Build

Clone the repository and run the super build script:

```
git clone https://github.com/grizzlypeak3d/DJV.git
sh DJV/sbuild-linux.sh
```

Run the application:

```
build-Release/bin/djv/djv DJV/etc/SampleData/BART_2021-02-07.0000.jpg
```

### Raspberry Pi

The Raspberry Pi 5 builds with the Debian packages above. Its graphics driver offers OpenGL ES 3.1 but only OpenGL 3.1 on the desktop, and DJV on OpenGL needs 4.1. The same build holds both: DJV tries OpenGL 4.1 when it starts and uses OpenGL ES 3.0 where that is not to be had, so nothing needs setting. The log, shown with `-log`, says which is in use.

The one tried first can be changed in `etc/Config/local.cmake`, which saves the attempt at the other:

```
set(ftk_API "GLES_3" CACHE STRING "")
```

The `FTK_GL_API` environment variable asks for one by name, `GL_4_1` or `GLES_3`, and the other is then not tried. It is how OpenGL ES is tested on a desktop:

```
FTK_GL_API=GLES_3 build-Release/bin/djv/djv
```

Some of the largest files take close to 1 GB of memory each to compile, so on an 8 GB board the build is limited to three jobs:

```
CMAKE_BUILD_PARALLEL_LEVEL=3 sh DJV/sbuild-linux.sh
```

The AV1 and APV libraries are among the slowest dependencies to build, and can be left out to save time. Without libaom there is no AV1 playback: FFmpeg's own AV1 decoder works only with hardware decoding.

```
set(TLRENDER_AOM OFF CACHE BOOL "")
set(TLRENDER_SVTAV1 OFF CACHE BOOL "")
set(TLRENDER_OPENAPV OFF CACHE BOOL "")
```

What differs from a desktop:

- OpenGL ES has no 16-bit textures, so images with 16 bits per channel, and video with more than 8, are not displayed.
- There is no hardware decoding: the Pi's only video decoder is for HEVC, and FFmpeg does not support it without Raspberry Pi's patches. Video is decoded on the CPU.
- The display scale is set from the monitor's size and resolution. On the Raspberry Pi Touch Display 2 it comes out at 2; `-displayScale` sets it on the command line.
- On a touch screen one finger acts as the mouse, and two fingers pan and pinch to zoom the view and the timeline.

## macOS

Requirements: Git, Xcode, and CMake 3.31.

Clone the repository and run the super build script:

```
git clone https://github.com/grizzlypeak3d/DJV.git
sh DJV/sbuild-macos.sh
```

Run the application:

```
build-Release/bin/djv/djv DJV/etc/SampleData/BART_2021-02-07.0000.jpg
```

These aliases are convenient for switching between architectures:

```
alias arm="env /usr/bin/arch -arm64 /bin/zsh --login"
alias intel="env /usr/bin/arch -x86_64 /bin/zsh --login"
```

## Windows

Requirements:

- Git ([git-scm.com](https://git-scm.com))
- Visual Studio 2022
- CMake 3.31
- NASM ([nasm.us](https://www.nasm.us)) for compiling FFmpeg and libjpeg-turbo
- MSYS2 ([msys2.org](https://www.msys2.org)) for compiling FFmpeg
- Strawberry Perl ([strawberryperl.com](https://strawberryperl.com/)) for compiling network support
- Python 3.11 for compiling USD
- NSIS ([nsis.sourceforge.io](https://nsis.sourceforge.io/Main_Page)) for packaging

Open the Visual Studio command console "x64 Native Tools Command Prompt for VS 2022", found in the Start menu in the "Visual Studio 2022" folder.

Clone the repository and run the super build script:

```
git clone https://github.com/grizzlypeak3d/DJV.git
DJV\sbuild-win.bat
```

Run the application:

```
set PATH=%CD%\install-Release\bin;%PATH%
build-Release\bin\djv\Release\djv DJV\etc\SampleData\BART_2021-02-07.0000.jpg
```

## The GPU renderer {#gpu-renderer}

Besides OpenGL, DJV is built with a second renderer, on Metal on macOS and Vulkan on Linux and Windows, which is what HDR needs. OpenGL draws unless the other is chosen: in the **Graphics** section of the **Settings** tool, or with `-renderer GPU` on the command line.

The build options are `ftk_GPU` and `TLRENDER_GPU`, which `etc/Config/default.cmake` turns on except for OpenGL ES. A build directory from before they existed has them in its cache as off, and a configuration file does not overwrite what is there; say them to it once:

```
cmake -S DJV -B build-Release -Dftk_GPU=ON -DTLRENDER_GPU=ON
```

The super build builds SDL with its Metal and Vulkan drivers and, for Vulkan, glslang, which compiles the shaders. At run time Vulkan needs the system's loader and a driver: on Debian and Ubuntu, `libvulkan1` and `mesa-vulkan-drivers`. Where it is chosen and cannot draw, the log says why and OpenGL draws.

How it works, its environment variables and its test programs are described beside the code: `lib/ftk/GPU/README.md` in feather-tk and `lib/tlRender/GPU/README.md` in tlRender.

## Updating a checkout {#updating}

The super build script checks out the submodules the first time it runs and does not move them afterwards. After pulling, update them to the commits DJV uses:

```
git -C DJV pull
git -C DJV submodule update --init --recursive
```

Then run the super build script again. Dependencies that are already built are not built again, and DJV is built from where it left off.

A clean build is needed only in some cases:

- When a dependency changes, for example a new version of FFmpeg, or when the compiler is updated: remove `ftk-Release`, `tl-Release`, `build-Release`, and `install-Release`, and build again.
- When an option in `etc/Config/local.cmake` changes: the same, since the options apply to the dependencies as well as to DJV.
- When a build that was working fails to link, or the application misbehaves after an update: remove `build-Release` and build again. feather-tk, tlRender, and DJV are all built there, so this takes much less time than the dependencies.

## System packages

On Linux the dependencies can come from the distribution instead of the super build. This is the build a distribution package would make. Ubuntu 26.04 is the first release with every package it needs, and it is the configuration continuous integration builds, so what follows is kept true there.

Install the system packages:

```
sudo apt-get install build-essential git cmake xorg-dev libglu1-mesa-dev mesa-common-dev mesa-utils libasound2-dev libpulse-dev libva-dev libdrm-dev libwayland-dev wayland-protocols libxkbcommon-dev libegl1-mesa-dev libdecor-0-dev pkg-config zlib1g-dev libpng-dev libfreetype-dev nlohmann-json3-dev libsdl3-dev libimath-dev libopenexr-dev libopencolorio-dev libopenimageio-dev openimageio-tools libopentimelineio-dev libminizip-ng-dev libavcodec-dev libavdevice-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev
```

The OpenImageIO tools are listed because the library's CMake configuration names them, and configuring fails without them. lunasvg has no package and is built from source, into a prefix beside the build:

```
git clone --branch v3.4.0 https://github.com/sammycage/lunasvg.git
cmake -S lunasvg -B lunasvg-build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/install-Release -DLUNASVG_BUILD_EXAMPLES=OFF
cmake --build lunasvg-build --target install
```

subprocess.h, which runs the FFmpeg command line tool, is a single header with no package, copied into the same prefix at the commit the super build uses:

```
git clone https://github.com/sheredom/subprocess.h.git subprocess
git -C subprocess checkout 0d76f78ff8b56d1240ffe6571d689c5e26299527
mkdir -p install-Release/include
cp subprocess/subprocess.h install-Release/include
```

Then DJV is configured directly, with its default options and that prefix on the search path, and built. Ubuntu installs OpenColorIO's CMake configuration in `/usr/share/cmake` itself, which CMake does not search, so that directory is named. The super build's libraries are left out of the package, since it gathers them out of the install prefix and a build against the system's packages has none there; the `package` target still makes a package, of DJV alone:

```
git clone --recursive https://github.com/grizzlypeak3d/DJV.git
cmake -S DJV -B build-Release -C DJV/etc/Config/default.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/install-Release -DCMAKE_PREFIX_PATH=$PWD/install-Release -DOpenColorIO_DIR=/usr/share/cmake -DDJV_PACKAGE_DEPS=OFF
cmake --build build-Release
```

The script `etc/Linux/build-system.sh` does the same, and is what continuous integration runs.

What differs from the super build:

- The distribution's OpenTimelineIO is 0.18.1. Markers carry an RGBA color from 0.19, so a review saved by this build writes marker colors as the default named color, and reads named colors.
- The super build patches FFmpeg so that media past 2 GB inside an OTIOZ bundle can be read; the distribution's FFmpeg is unpatched, and such an entry does not open.
- The codecs are whatever the distribution's FFmpeg carries, rather than the packages' minimal set.
