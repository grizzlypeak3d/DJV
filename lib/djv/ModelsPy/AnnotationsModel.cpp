// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/AnnotationsModel.h>

#include <ftk/CorePy/Bindings.h>

#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void annotationsModel(py::module_& m)
        {
            using namespace models;

            py::class_<ReviewStroke>(m, "ReviewStroke")
                .def(py::init())
                .def_readwrite("color", &ReviewStroke::color)
                .def_readwrite("width", &ReviewStroke::width)
                .def_readwrite("points", &ReviewStroke::points)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            py::class_<ReviewAnnotation>(m, "ReviewAnnotation")
                .def(py::init())
                .def_readwrite("id", &ReviewAnnotation::id)
                .def_readwrite("sourceId", &ReviewAnnotation::sourceId)
                .def_readwrite("time", &ReviewAnnotation::time)
                .def_readwrite("author", &ReviewAnnotation::author)
                .def_readwrite("created", &ReviewAnnotation::created)
                .def_readwrite("strokes", &ReviewAnnotation::strokes)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            ftk::python::observableList<ReviewAnnotation>(m, "ReviewAnnotation");

            py::class_<AnnotationsModel, std::shared_ptr<AnnotationsModel> >(m, "AnnotationsModel")
                .def(py::init(&AnnotationsModel::create))
                .def_property_readonly("annotations", &AnnotationsModel::getAnnotations, py::return_value_policy::copy)
                .def_property_readonly("observeAnnotations", &AnnotationsModel::observeAnnotations)
                .def("getStrokes", &AnnotationsModel::getStrokes, py::arg("sourceId"), py::arg("time"))
                .def("setAnnotations", &AnnotationsModel::setAnnotations, py::arg("annotations"))
                .def("addStroke", &AnnotationsModel::addStroke, py::arg("sourceId"), py::arg("time"), py::arg("stroke"))
                .def("eraseStrokes", &AnnotationsModel::eraseStrokes, py::arg("sourceId"), py::arg("time"), py::arg("pos"), py::arg("radius"))
                .def("clearFrame", &AnnotationsModel::clearFrame, py::arg("sourceId"), py::arg("time"))
                .def("clear", &AnnotationsModel::clear)
                .def_property_readonly("observeHasUndo", &AnnotationsModel::observeHasUndo)
                .def_property_readonly("observeHasRedo", &AnnotationsModel::observeHasRedo)
                .def("undo", &AnnotationsModel::undo)
                .def("redo", &AnnotationsModel::redo);
        }
    }
}
