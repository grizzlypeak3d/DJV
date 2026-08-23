// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#define DJV_VERSION_MAJOR 3
#define DJV_VERSION_MINOR 6
#define DJV_VERSION_PATCH 0
#define DJV_VERSION_DEV "-dev"
// Built from the parts above rather than written out beside them. The two
// were kept in step by hand and stopped being: the numbers said one release
// and the string another, and both are read -- AppInfoModel hands out all
// five. A release is one number to change now.
#define DJV_VERSION_STR_(x) #x
#define DJV_VERSION_STR(x) DJV_VERSION_STR_(x)
#define DJV_VERSION_FULL \
    DJV_VERSION_STR(DJV_VERSION_MAJOR) "." \
    DJV_VERSION_STR(DJV_VERSION_MINOR) "." \
    DJV_VERSION_STR(DJV_VERSION_PATCH) DJV_VERSION_DEV
