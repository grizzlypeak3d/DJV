// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/NotesModel.h>

#include <ftk/CorePy/Bindings.h>

#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void notesModel(py::module_& m)
        {
            using namespace models;

            py::class_<ReviewNote>(m, "ReviewNote")
                .def(py::init())
                .def_readwrite("id", &ReviewNote::id)
                .def_readwrite("time", &ReviewNote::time)
                .def_readwrite("created", &ReviewNote::created)
                .def_readwrite("author", &ReviewNote::author)
                .def_readwrite("text", &ReviewNote::text)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            ftk::python::observableList<ReviewNote>(m, "ReviewNote");

            py::class_<NotesModel, std::shared_ptr<NotesModel> >(m, "NotesModel")
                .def(py::init(&NotesModel::create))
                .def_property_readonly("notes", &NotesModel::getNotes, py::return_value_policy::copy)
                .def_property_readonly("observeNotes", &NotesModel::observeNotes)
                .def("setNotes", &NotesModel::setNotes, py::arg("notes"))
                .def("add", &NotesModel::add, py::arg("time"), py::arg("text"))
                .def("update", &NotesModel::update, py::arg("id"), py::arg("text"))
                .def("remove", &NotesModel::remove, py::arg("id"))
                .def("clear", &NotesModel::clear);
        }
    }
}
