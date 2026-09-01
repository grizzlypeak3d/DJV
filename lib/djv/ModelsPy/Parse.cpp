// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/Parse.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void parse(py::module_& m)
        {
            using namespace models;

            m.def("parseFrameRange", &parseFrameRange, py::arg("value"));
            m.def(
                "parseTime",
                &parseTime,
                py::arg("name"),
                py::arg("value"),
                py::arg("speed"),
                py::arg("units"));
        }
    }
}
