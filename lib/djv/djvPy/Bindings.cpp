// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>
#include <djv/UIPy/Bindings.h>

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(djvPy, m)
{
    m.doc() = "DJV is an open source application for playback and review of image sequences.";

    py::module_::import("tlRenderPy");

    djv::python::modelsBind(m);
    djv::python::uiBind(m);
}
