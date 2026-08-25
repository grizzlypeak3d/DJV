// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/FilesModel.h>

#include <ftk/UI/Settings.h>

#include <ftk/CorePy/Bindings.h>

#include <pybind11/stl.h>
#include <pybind11/functional.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void filesModel(py::module_& m)
        {
            using namespace models;

            py::class_<FilesModelItem, std::shared_ptr<FilesModelItem> >(m, "FilesModelItem")
                .def(py::init<>())
                .def_readwrite("path", &FilesModelItem::path)
                .def_readwrite("audioPath", &FilesModelItem::audioPath)
                .def_readwrite("videoLayers", &FilesModelItem::videoLayers)
                .def_readwrite("videoLayer", &FilesModelItem::videoLayer)
                .def_readwrite("speed", &FilesModelItem::speed)
                .def_readwrite("currentTime", &FilesModelItem::currentTime)
                .def_readwrite("inOutRange", &FilesModelItem::inOutRange)
                .def_readwrite("timeRange", &FilesModelItem::timeRange)
                .def_readwrite("framesStated", &FilesModelItem::framesStated)
                .def_readwrite("newFile", &FilesModelItem::newFile);

            ftk::python::observable<std::shared_ptr<FilesModelItem> >(m, "FilesModelItem");
            ftk::python::observableList<std::shared_ptr<FilesModelItem> >(m, "FilesModelItem");
            ftk::python::observable<tl::CompareOptions>(m, "CompareOptions");
            ftk::python::observable<tl::CompareTime>(m, "CompareTime");

            py::class_<FilesModel, std::shared_ptr<FilesModel> >(m, "FilesModel")
                .def(
                    py::init(&FilesModel::create),
                    py::arg("settings"))

                .def("save", &FilesModel::save)

                .def_property_readonly("files", &FilesModel::getFiles)
                .def_property_readonly("observeFiles", &FilesModel::observeFiles)
                .def_property_readonly("a", &FilesModel::getA)
                .def_property_readonly("observeA", &FilesModel::observeA)
                .def_property_readonly("aIndex", &FilesModel::getAIndex)
                .def_property_readonly("observeAIndex", &FilesModel::observeAIndex)
                .def_property_readonly("b", &FilesModel::getB)
                .def_property_readonly("observeB", &FilesModel::observeB)
                .def_property_readonly("bIndexes", &FilesModel::getBIndexes)
                .def_property_readonly("observeBIndexes", &FilesModel::observeBIndexes)
                .def_property_readonly("active", &FilesModel::getActive)
                .def_property_readonly("observeActive", &FilesModel::observeActive)

                .def("add", &FilesModel::add)
                .def("close", py::overload_cast<>(&FilesModel::close))
                .def("close", py::overload_cast<int>(&FilesModel::close), py::arg("index"))
                .def("closeAll", &FilesModel::closeAll)
                .def("setA", &FilesModel::setA, py::arg("index"))
                .def("setB", &FilesModel::setB, py::arg("index"), py::arg("value"))
                .def("toggleB", &FilesModel::toggleB, py::arg("index"))
                .def("clearB", &FilesModel::clearB)
                .def("first", &FilesModel::first)
                .def("last", &FilesModel::last)
                .def("next", &FilesModel::next)
                .def("prev", &FilesModel::prev)
                .def("firstB", &FilesModel::firstB)
                .def("lastB", &FilesModel::lastB)
                .def("nextB", &FilesModel::nextB)
                .def("prevB", &FilesModel::prevB)

                .def_property_readonly("observeLayers", &FilesModel::observeLayers)
                .def("setLayer", &FilesModel::setLayer, py::arg("item"), py::arg("layer"))
                .def("setFrames", &FilesModel::setFrames, py::arg("item"), py::arg("range"))
                .def_property_readonly("observeReload", &FilesModel::observeReload)
                .def("refresh", &FilesModel::refresh)
                .def("nextLayer", &FilesModel::nextLayer)
                .def("prevLayer", &FilesModel::prevLayer)

                .def_property(
                    "compareOptions",
                    &FilesModel::getCompareOptions,
                    &FilesModel::setCompareOptions,
                    py::return_value_policy::copy)
                .def_property_readonly("observeCompareOptions", &FilesModel::observeCompareOptions)
                .def_property(
                    "compareTime",
                    &FilesModel::getCompareTime,
                    &FilesModel::setCompareTime)
                .def_property_readonly("observeCompareTime", &FilesModel::observeCompareTime);

            m.def("getCompareTimeLabels", &getCompareTimeLabels);
        }
    }
}
