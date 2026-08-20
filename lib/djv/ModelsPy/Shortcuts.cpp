// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/Shortcuts.h>

#include <pybind11/operators.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void shortcuts(py::module_& m)
        {
            using namespace models;

            py::class_<Shortcut>(m, "Shortcut")
                .def(py::init())
                .def(
                    py::init<
                        const std::string&,
                        const std::string&,
                        const ftk::KeyShortcut&,
                        const ftk::KeyShortcut&>(),
                    py::arg("name"),
                    py::arg("text"),
                    py::arg("primary") = ftk::KeyShortcut(),
                    py::arg("secondary") = ftk::KeyShortcut())
                .def_readwrite("name", &Shortcut::name)
                .def_readwrite("text", &Shortcut::text)
                .def_readwrite("primary", &Shortcut::primary)
                .def_readwrite("secondary", &Shortcut::secondary)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);
        }
    }
}
