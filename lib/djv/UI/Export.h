// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

// The platform layer -- DJV_EXPORT and the rest -- is shared, and lives with
// the models library.
#include <djv/Models/Export.h>

// The macros for the user interface library. Each library in this project has its own set: one
// shared between them would be defined for whichever library is being built,
// so a library compiling a sibling's headers would read the sibling's API as
// dllexport where it wants dllimport. Functions survive that -- the linker
// takes them from the import library -- and data does not.
#if defined(DJV_UI_STATIC)
#    define DJV_UI_API
#    define DJV_UI_API_TYPE
#    define DJV_UI_API_TEMPLATE_CLASS(...)
#    define DJV_UI_API_TEMPLATE_STRUCT(...)
#    define DJV_UI_LOCAL
#else
#    if defined(DJV_UI_EXPORTS)
#        define DJV_UI_API DJV_EXPORT
#        define DJV_UI_API_TYPE DJV_EXPORT_TYPE
#        define DJV_UI_API_TEMPLATE_CLASS(...)                                             DJV_EXPORT_TEMPLATE(class, __VA_ARGS__)
#        define DJV_UI_API_TEMPLATE_STRUCT(...)                                            DJV_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#    else
#        define DJV_UI_API DJV_IMPORT
#        define DJV_UI_API_TYPE DJV_IMPORT_TYPE
#        define DJV_UI_API_TEMPLATE_CLASS(...)                                             DJV_IMPORT_TEMPLATE(class, __VA_ARGS__)
#        define DJV_UI_API_TEMPLATE_STRUCT(...)                                            DJV_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#    endif
#    define DJV_UI_LOCAL DJV_HIDDEN
#endif
