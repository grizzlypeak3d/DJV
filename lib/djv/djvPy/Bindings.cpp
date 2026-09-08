// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <djv/UIPy/Bindings.h>

#include <nanobind/nanobind.h>

namespace nb = nanobind;

NB_MODULE(djvPy, m)
{
    m.doc() = "DJV is an open source application for playback and review of image sequences.";

    nb::module_::import_("tlRenderPy");

    djv::python::modelsBind(m);
    djv::python::uiBind(m);
}
