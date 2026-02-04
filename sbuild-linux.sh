#!/bin/sh

set -x

BUILD_TYPE=Release
if [ "$#" -eq 1 ]; then
    BUILD_TYPE=$1
fi

export JOBS=4
export TLRENDER_NET=OFF
export TLRENDER_OCIO=ON
export TLRENDER_JPEG=ON
export TLRENDER_TIFF=ON
export TLRENDER_EXR=ON
export TLRENDER_FFMPEG=ON
export TLRENDER_FFMPEG_MINIMAL=OFF
export TLRENDER_NASM=ON
export TLRENDER_OIIO=ON
export TLRENDER_USD=OFF
export TLRENDER_BMD=OFF
export TLRENDER_BMD_SDK=
export FTK_API=GL_4_1
export BUILD_SHARED_LIBS=OFF

sh DJV/etc/Linux/linux-sbuild.sh $BUILD_TYPE
sh DJV/etc/Linux/linux-build.sh $BUILD_TYPE
