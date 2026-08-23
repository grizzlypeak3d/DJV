// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

// For an explanation of how these export defines work, see:
// https://github.com/PixarAnimationStudios/OpenUSD/blob/dev/pxr/base/arch/export.h
#if defined(_WINDOWS)
#    if defined(__GNUC__) && __GNUC__ >= 4 || defined(__clang__)
#        define DJV_EXPORT __attribute__((dllexport))
#        define DJV_IMPORT __attribute__((dllimport))
#        define DJV_HIDDEN
#        define DJV_EXPORT_TYPE
#        define DJV_IMPORT_TYPE
#    else
#        define DJV_EXPORT __declspec(dllexport)
#        define DJV_IMPORT __declspec(dllimport)
#        define DJV_HIDDEN
#        define DJV_EXPORT_TYPE
#        define DJV_IMPORT_TYPE
#    endif
#elif defined(__GNUC__) && __GNUC__ >= 4 || defined(__clang__)
#    define DJV_EXPORT __attribute__((visibility("default")))
#    define DJV_IMPORT
#    define DJV_HIDDEN __attribute__((visibility("hidden")))
#    if defined(__clang__)
#        define DJV_EXPORT_TYPE                                     \
            __attribute__((type_visibility("default")))
#    else
#        define DJV_EXPORT_TYPE                                     \
            __attribute__((visibility("default")))
#    endif
#    define DJV_IMPORT_TYPE
#else
#    define DJV_EXPORT
#    define DJV_IMPORT
#    define DJV_HIDDEN
#    define DJV_EXPORT_TYPE
#    define DJV_IMPORT_TYPE
#endif
#define DJV_EXPORT_TEMPLATE(type, ...)
#define DJV_IMPORT_TEMPLATE(type, ...)                              \
    extern template type DJV_IMPORT __VA_ARGS__

// The macros for the models library. Each library in this project has its own set: one
// shared between them would be defined for whichever library is being built,
// so a library compiling a sibling's headers would read the sibling's API as
// dllexport where it wants dllimport. Functions survive that -- the linker
// takes them from the import library -- and data does not.
#if defined(DJV_MODELS_STATIC)
#    define DJV_MODELS_API
#    define DJV_MODELS_API_TYPE
#    define DJV_MODELS_API_TEMPLATE_CLASS(...)
#    define DJV_MODELS_API_TEMPLATE_STRUCT(...)
#    define DJV_MODELS_LOCAL
#else
#    if defined(DJV_MODELS_EXPORTS)
#        define DJV_MODELS_API DJV_EXPORT
#        define DJV_MODELS_API_TYPE DJV_EXPORT_TYPE
#        define DJV_MODELS_API_TEMPLATE_CLASS(...)                                             DJV_EXPORT_TEMPLATE(class, __VA_ARGS__)
#        define DJV_MODELS_API_TEMPLATE_STRUCT(...)                                            DJV_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#    else
#        define DJV_MODELS_API DJV_IMPORT
#        define DJV_MODELS_API_TYPE DJV_IMPORT_TYPE
#        define DJV_MODELS_API_TEMPLATE_CLASS(...)                                             DJV_IMPORT_TEMPLATE(class, __VA_ARGS__)
#        define DJV_MODELS_API_TEMPLATE_STRUCT(...)                                            DJV_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#    endif
#    define DJV_MODELS_LOCAL DJV_HIDDEN
#endif
