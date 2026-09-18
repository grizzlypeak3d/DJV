// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>
#include <djv/UIPy/Bindings.h>

#include <djv/Models/Version.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

NB_MODULE(_djv, m)
{
    m.doc() = "DJV is an open source application for playback and review of image sequences.";

    nb::module_::import_("tlrender");

    m.attr("VERSION_MAJOR") = DJV_VERSION_MAJOR;
    m.attr("VERSION_MINOR") = DJV_VERSION_MINOR;
    m.attr("VERSION_PATCH") = DJV_VERSION_PATCH;
    m.attr("VERSION_DEV") = DJV_VERSION_DEV;
    m.attr("VERSION_FULL") = DJV_VERSION_FULL;

    djv::python::modelsBind(m);
    djv::python::uiBind(m);
}
