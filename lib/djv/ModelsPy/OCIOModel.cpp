// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/OCIOModel.h>

#include <ftk/CorePy/Bindings.h>
#include <ftk/Core/Context.h>

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void ocioModel(py::module_& m)
        {
            using namespace models;

            py::class_<OCIOModelData>(m, "OCIOModelData")
                .def(py::init())
                .def_readwrite("enabled", &OCIOModelData::enabled)
                .def_readwrite("config", &OCIOModelData::config)
                .def_readwrite("fileName", &OCIOModelData::fileName)
                .def_readwrite("name", &OCIOModelData::name)
                .def_readwrite("inputs", &OCIOModelData::inputs)
                .def_readwrite("inputIndex", &OCIOModelData::inputIndex)
                .def_readwrite("displays", &OCIOModelData::displays)
                .def_readwrite("displayIndex", &OCIOModelData::displayIndex)
                .def_readwrite("views", &OCIOModelData::views)
                .def_readwrite("viewIndex", &OCIOModelData::viewIndex)
                .def_readwrite("looks", &OCIOModelData::looks)
                .def_readwrite("lookIndex", &OCIOModelData::lookIndex)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            ftk::python::observable<OCIOModelData>(m, "OCIOModelData");

            py::class_<OCIOModel, std::shared_ptr<OCIOModel> >(m, "OCIOModel")
                .def(
                    py::init(&OCIOModel::create),
                    py::arg("context"))
                .def("setOptions", &OCIOModel::setOptions)
                .def_property_readonly("observeOptions", &OCIOModel::observeOptions)
                .def_property_readonly("observeData", &OCIOModel::observeData)
                .def("setEnabled", &OCIOModel::setEnabled)
                .def("setConfig", &OCIOModel::setConfig)
                .def("setFileName", &OCIOModel::setFileName, py::arg("fileName"))
                .def("setInputIndex", &OCIOModel::setInputIndex)
                .def("setDisplayIndex", &OCIOModel::setDisplayIndex)
                .def("setViewIndex", &OCIOModel::setViewIndex)
                .def("setLookIndex", &OCIOModel::setLookIndex);
        }
    }
}
