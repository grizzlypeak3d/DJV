#!/bin/sh

set -x

# Update packages
sudo apt-get update

# Install OpenGL support
sudo apt-get install xorg-dev libglu1-mesa-dev mesa-common-dev mesa-utils xvfb
xvfb-run glxinfo

# Install ALSA and PulseAudio support
sudo apt-get install libasound2-dev
sudo apt-get install libpulse-dev
