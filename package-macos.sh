#!/bin/sh

set -x

export JOBS=4
export DJV_MACOS_PACKAGE=ON
export TLRENDER_NET=OFF
export TLRENDER_OCIO=ON
export TLRENDER_JPEG=ON
export TLRENDER_TIFF=ON
export TLRENDER_EXR=ON
export TLRENDER_FFMPEG=ON
export TLRENDER_FFMPEG_MINIMAL=ON
export TLRENDER_NASM=ON
export TLRENDER_OIIO=ON
export TLRENDER_USD=ON
export TLRENDER_BMD=OFF
export TLRENDER_BMD_SDK=
export FTK_API=GL_4_1
export CMAKE_OSX_DEPLOYMENT_TARGET=10.15
export CMAKE_OSX_ARCHITECTURES=arm64

sh DJV/etc/macOS/macos-sbuild.sh Release
sh DJV/etc/macOS/macos-package.sh Release
