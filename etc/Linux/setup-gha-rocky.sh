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

# Install ALSA and PulseAudio support
dnf install -y \
    alsa-lib-devel \
    pulseaudio-libs-devel
