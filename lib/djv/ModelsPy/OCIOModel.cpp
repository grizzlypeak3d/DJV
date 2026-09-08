// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/OCIOModel.h>

#include <ftk/CorePy/Bindings.h>
#include <ftk/Core/Context.h>

#include <nanobind/stl/function.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/filesystem.h>

namespace nb = nanobind;

namespace djv
{
    namespace python
    {
        void ocioModel(nb::module_& m)
        {
            using namespace models;

            nb::class_<OCIOModelData>(m, "OCIOModelData")
                .def(nb::init())
                .def_rw("enabled", &OCIOModelData::enabled)
                .def_rw("config", &OCIOModelData::config)
                .def_rw("fileName", &OCIOModelData::fileName)
                .def_rw("name", &OCIOModelData::name)
                .def_rw("inputs", &OCIOModelData::inputs)
                .def_rw("inputIndex", &OCIOModelData::inputIndex)
                .def_rw("displays", &OCIOModelData::displays)
                .def_rw("displayIndex", &OCIOModelData::displayIndex)
                .def_rw("views", &OCIOModelData::views)
                .def_rw("viewIndex", &OCIOModelData::viewIndex)
                .def_rw("looks", &OCIOModelData::looks)
                .def_rw("lookIndex", &OCIOModelData::lookIndex)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            ftk::python::observable<OCIOModelData>(m, "OCIOModelData");

            nb::class_<OCIOModel>(m, "OCIOModel")
                .def(
                    nb::new_(&OCIOModel::create),
                    nb::arg("context"))
                .def("setOptions", &OCIOModel::setOptions)
                .def_prop_ro("observeOptions", &OCIOModel::observeOptions)
                .def_prop_ro("observeData", &OCIOModel::observeData)
                .def("setEnabled", &OCIOModel::setEnabled)
                .def("setConfig", &OCIOModel::setConfig)
                .def("setFileName", &OCIOModel::setFileName, nb::arg("fileName"))
                .def("setInputIndex", &OCIOModel::setInputIndex)
                .def("setDisplayIndex", &OCIOModel::setDisplayIndex)
                .def("setViewIndex", &OCIOModel::setViewIndex)
                .def("setLookIndex", &OCIOModel::setLookIndex);
        }
    }
}
