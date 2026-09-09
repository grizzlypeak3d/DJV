#!/bin/sh

# Install what the system-packages build needs on Ubuntu 26.04, for the
# continuous integration job that builds against the distribution's packages
# rather than the super build. Runs inside a container as root, so there is
# no sudo. The package list is the one in docs/building.html.

set -e
set -x

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y build-essential git cmake xorg-dev libglu1-mesa-dev mesa-common-dev mesa-utils libasound2-dev libpulse-dev libva-dev libdrm-dev libwayland-dev wayland-protocols libxkbcommon-dev libegl1-mesa-dev libdecor-0-dev pkg-config zlib1g-dev libpng-dev libfreetype-dev nlohmann-json3-dev libsdl3-dev libimath-dev libopenexr-dev libopencolorio-dev libopenimageio-dev libopentimelineio-dev libminizip-ng-dev libavcodec-dev libavdevice-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev xvfb libgl1-mesa-dri
xvfb-run glxinfo
