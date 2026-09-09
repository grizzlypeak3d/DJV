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
# libdecor gives window decorations on GNOME; Rocky 8 may not carry it,
# and the driver builds without it.
dnf install -y \
    wayland-devel \
    wayland-protocols-devel \
    libxkbcommon-devel \
    mesa-libEGL-devel
dnf install -y libdecor-devel || echo "libdecor-devel is not available"

# Install ALSA and PulseAudio support
dnf install -y \
    alsa-lib-devel \
    pulseaudio-libs-devel
