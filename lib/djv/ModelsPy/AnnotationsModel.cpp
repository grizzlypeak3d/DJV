// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/AnnotationsModel.h>

#include <ftk/CorePy/Bindings.h>

#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/function.h>

namespace nb = nanobind;

namespace djv
{
    namespace python
    {
        void annotationsModel(nb::module_& m)
        {
            using namespace models;

            nb::class_<ReviewStroke>(m, "ReviewStroke")
                .def(nb::init())
                .def_rw("color", &ReviewStroke::color)
                .def_rw("width", &ReviewStroke::width)
                .def_rw("points", &ReviewStroke::points)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            nb::class_<ReviewAnnotation>(m, "ReviewAnnotation")
                .def(nb::init())
                .def_rw("id", &ReviewAnnotation::id)
                .def_rw("sourceId", &ReviewAnnotation::sourceId)
                .def_rw("time", &ReviewAnnotation::time)
                .def_rw("author", &ReviewAnnotation::author)
                .def_rw("created", &ReviewAnnotation::created)
                .def_rw("strokes", &ReviewAnnotation::strokes)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            ftk::python::observableList<ReviewAnnotation>(m, "ReviewAnnotation");

            nb::class_<AnnotationsModel>(m, "AnnotationsModel")
                .def(nb::new_(&AnnotationsModel::create))
                .def_prop_ro("annotations", &AnnotationsModel::getAnnotations, nb::rv_policy::copy)
                .def_prop_ro("observeAnnotations", &AnnotationsModel::observeAnnotations)
                .def("getStrokes", &AnnotationsModel::getStrokes, nb::arg("sourceId"), nb::arg("time"))
                .def("setAnnotations", &AnnotationsModel::setAnnotations, nb::arg("annotations"))
                .def("addStroke", &AnnotationsModel::addStroke, nb::arg("sourceId"), nb::arg("time"), nb::arg("stroke"))
                .def("eraseStrokes", &AnnotationsModel::eraseStrokes, nb::arg("sourceId"), nb::arg("time"), nb::arg("pos"), nb::arg("radius"))
                .def("clearFrame", &AnnotationsModel::clearFrame, nb::arg("sourceIds"), nb::arg("time"))
                .def("clear", &AnnotationsModel::clear)
                .def_prop_ro("observeHasUndo", &AnnotationsModel::observeHasUndo)
                .def_prop_ro("observeHasRedo", &AnnotationsModel::observeHasRedo)
                .def("undo", &AnnotationsModel::undo)
                .def("redo", &AnnotationsModel::redo);
        }
    }
}
