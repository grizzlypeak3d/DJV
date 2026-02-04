#!/bin/sh

set -x

export JOBS=4
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

sh DJV/etc/Linux/linux-sbuild.sh Release
sh DJV/etc/Linux/linux-package.sh Release
