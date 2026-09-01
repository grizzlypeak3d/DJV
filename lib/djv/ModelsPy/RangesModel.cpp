// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/RangesModel.h>

#include <ftk/CorePy/Bindings.h>

#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void rangesModel(py::module_& m)
        {
            using namespace models;

            py::class_<ReviewRange>(m, "ReviewRange")
                .def(py::init())
                .def_readwrite("id", &ReviewRange::id)
                .def_readwrite("name", &ReviewRange::name)
                .def_readwrite("range", &ReviewRange::range)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            ftk::python::observableList<ReviewRange>(m, "ReviewRange");

            py::class_<RangesModel, std::shared_ptr<RangesModel> >(m, "RangesModel")
                .def(py::init(&RangesModel::create))
                .def_property_readonly("ranges", &RangesModel::getRanges, py::return_value_policy::copy)
                .def_property_readonly("observeRanges", &RangesModel::observeRanges)
                .def("setRanges", &RangesModel::setRanges, py::arg("ranges"))
                .def("add", &RangesModel::add, py::arg("range"), py::arg("name"))
                .def("remove", &RangesModel::remove, py::arg("id"))
                .def("clear", &RangesModel::clear);
        }
    }
}
