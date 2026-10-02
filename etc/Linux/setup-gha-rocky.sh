#!/bin/sh

# The Rocky Linux equivalent of setup-gha.sh, for the packaging job. It runs
# inside a container as root, so there is no sudo, and the packages are named
# for RHEL rather than Debian.

set -e
set -x

# Install OpenGL support
dnf install -y \
    mesa-libGL-devel \
    mesa-libGLU-devel \
    libX11-devel \
    libXcursor-devel \
    libXext-devel \
    libXi-devel \
    libXinerama-devel \
    libXrandr-devel \
    xorg-x11-server-Xvfb \
    glx-utils
xvfb-run glxinfo

# Wayland: SDL only builds its Wayland driver when these are present, and
# loads the libraries at run time, so the package gains no dependency.
dnf install -y \
    wayland-devel \
    wayland-protocols-devel \
    libxkbcommon-devel \
    mesa-libEGL-devel

# libdecor, which SDL draws the window decorations with where the compositor
# leaves them to the application, as GNOME does: built without it, the
# package's windows there have no title bar or border. Rocky 8 has no
# libdecor, and going on without it is how the package came to be built that
# way, so it is built here, for its headers. It is not in the package: SDL
# loads the system's, and treats what is newer than libdecor 0.1 as optional.
# cairo and pango are for libdecor's own plugin, which its build requires.
if ! dnf install -y libdecor-devel; then
    dnf install -y cairo-devel pango-devel
    python -m pip install meson ninja
    export PATH="$(python -c 'import sysconfig; print(sysconfig.get_path("scripts"))'):$PATH"
    curl --fail --location --retry 3 --output /tmp/libdecor.tar.gz \
        https://gitlab.freedesktop.org/libdecor/libdecor/-/archive/0.2.2/libdecor-0.2.2.tar.gz
    tar -xzf /tmp/libdecor.tar.gz -C /tmp
    meson setup /tmp/libdecor-build /tmp/libdecor-0.2.2 \
        --prefix=/usr --libdir=lib64 --buildtype=release \
        -Ddemo=false -Ddbus=disabled -Dgtk=disabled
    ninja -C /tmp/libdecor-build install
fi
pkg-config --modversion libdecor-0

# Install ALSA and PulseAudio support
dnf install -y \
    alsa-lib-devel \
    pulseaudio-libs-devel

# And PipeWire, which SDL also loads at run time. Without it SDL plays through
# PulseAudio on a PipeWire system, whose relay adds two threads that are not
# real time between the player and the device, and the audio drops out under
# load. SDL needs 0.3.44 to build against and Rocky 8's is older; going on
# without it is how the package came to be built that way, so it is built
# here, for its headers, with everything that needs another library turned
# off. It is not in the package: SDL loads the system's.
dnf install -y pipewire-devel || echo "pipewire-devel is not available"
if ! pkg-config --atleast-version=0.3.44 libpipewire-0.3; then
    dnf remove -y pipewire-devel || true
    python -m pip install meson ninja
    export PATH="$(python -c 'import sysconfig; print(sysconfig.get_path("scripts"))'):$PATH"
    curl --fail --location --retry 3 --output /tmp/pipewire.tar.gz \
        https://gitlab.freedesktop.org/pipewire/pipewire/-/archive/0.3.48/pipewire-0.3.48.tar.gz
    tar -xzf /tmp/pipewire.tar.gz -C /tmp
    meson setup /tmp/pipewire-build /tmp/pipewire-0.3.48 \
        --prefix=/usr --libdir=lib64 --buildtype=release --auto-features=disabled \
        -Dexamples=disabled -Dtests=disabled -Dpipewire-jack=disabled \
        -Dpipewire-v4l2=disabled -Ddbus=disabled -Dsystemd-user-service=disabled \
        '-Dsession-managers=[]'
    ninja -C /tmp/pipewire-build install
fi
pkg-config --atleast-version=0.3.44 libpipewire-0.3
