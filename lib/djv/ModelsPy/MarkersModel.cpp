// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/MarkersModel.h>

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
        void markersModel(nb::module_& m)
        {
            using namespace models;

            m.def("reviewMarkerColor", &reviewMarkerColor);

            nb::class_<ReviewMarker>(m, "ReviewMarker")
                .def(nb::init())
                .def_rw("id", &ReviewMarker::id)
                .def_rw("name", &ReviewMarker::name)
                .def_rw("range", &ReviewMarker::range)
                .def_rw("color", &ReviewMarker::color)
                .def_rw("text", &ReviewMarker::text)
                .def_rw("author", &ReviewMarker::author)
                .def_rw("created", &ReviewMarker::created)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            ftk::python::observableList<ReviewMarker>(m, "ReviewMarker");

            nb::class_<MarkersModel>(m, "MarkersModel")
                .def(nb::new_(&MarkersModel::create))
                .def_prop_ro("markers", &MarkersModel::getMarkers, nb::rv_policy::copy)
                .def_prop_ro("observeMarkers", &MarkersModel::observeMarkers)
                .def("setMarkers", &MarkersModel::setMarkers, nb::arg("markers"))
                .def("add", &MarkersModel::add,
                    nb::arg("range"), nb::arg("name"), nb::arg("text"))
                .def("update", &MarkersModel::update, nb::arg("id"), nb::arg("text"))
                .def("updateColor", &MarkersModel::updateColor, nb::arg("id"), nb::arg("color"))
                .def("remove", &MarkersModel::remove, nb::arg("id"))
                .def("clear", &MarkersModel::clear);
        }
    }
}
