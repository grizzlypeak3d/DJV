// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

//! \file
//! The std::function caster: feather-tk's own where it has one, which it
//! uses in place of nanobind's and which can't be included beside it, and
//! otherwise nanobind's. The wheel builds against the feather-tk release
//! tlRender pins, which may come from before feather-tk had one; once the
//! pin is past it, this can include <ftk/CorePy/Function.h> and nothing
//! else. The same as tlRender/CorePy/Function.h, which a tlRender wheel
//! from before it does not have.

#if __has_include(<ftk/CorePy/Function.h>)
#include <ftk/CorePy/Function.h>
#else
#include <nanobind/stl/function.h>
#endif
