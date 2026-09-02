// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/MarkersModel.h>

#include <ftk/CorePy/Bindings.h>

#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void markersModel(py::module_& m)
        {
            using namespace models;

            m.def("reviewMarkerColor", &reviewMarkerColor);

            py::class_<ReviewMarker>(m, "ReviewMarker")
                .def(py::init())
                .def_readwrite("id", &ReviewMarker::id)
                .def_readwrite("name", &ReviewMarker::name)
                .def_readwrite("range", &ReviewMarker::range)
                .def_readwrite("color", &ReviewMarker::color)
                .def_readwrite("text", &ReviewMarker::text)
                .def_readwrite("author", &ReviewMarker::author)
                .def_readwrite("created", &ReviewMarker::created)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            ftk::python::observableList<ReviewMarker>(m, "ReviewMarker");

            py::class_<MarkersModel, std::shared_ptr<MarkersModel> >(m, "MarkersModel")
                .def(py::init(&MarkersModel::create))
                .def_property_readonly("markers", &MarkersModel::getMarkers, py::return_value_policy::copy)
                .def_property_readonly("observeMarkers", &MarkersModel::observeMarkers)
                .def("setMarkers", &MarkersModel::setMarkers, py::arg("markers"))
                .def("add", &MarkersModel::add,
                    py::arg("range"), py::arg("name"), py::arg("text"))
                .def("update", &MarkersModel::update, py::arg("id"), py::arg("text"))
                .def("remove", &MarkersModel::remove, py::arg("id"))
                .def("clear", &MarkersModel::clear);
        }
    }
}
