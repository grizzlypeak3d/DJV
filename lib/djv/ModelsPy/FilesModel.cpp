// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/FilesModel.h>

#include <ftk/UI/Settings.h>

#include <ftk/CorePy/Bindings.h>

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
        void filesModel(nb::module_& m)
        {
            using namespace models;

            nb::class_<FilesModelItem>(m, "FilesModelItem")
                .def(nb::init<>())
                .def_rw("id", &FilesModelItem::id)
                .def_rw("path", &FilesModelItem::path)
                .def_rw("audioPath", &FilesModelItem::audioPath)
                .def_rw("videoLayers", &FilesModelItem::videoLayers)
                .def_rw("videoLayer", &FilesModelItem::videoLayer)
                .def_rw("speed", &FilesModelItem::speed)
                .def_rw("currentTime", &FilesModelItem::currentTime)
                .def_rw("inOutRange", &FilesModelItem::inOutRange)
                .def_rw("timeRange", &FilesModelItem::timeRange)
                .def_rw("framesStated", &FilesModelItem::framesStated)
                .def_rw("newFile", &FilesModelItem::newFile);

            ftk::python::observable<std::shared_ptr<FilesModelItem> >(m, "FilesModelItem");
            ftk::python::observableList<std::shared_ptr<FilesModelItem> >(m, "FilesModelItem");
            ftk::python::observable<tl::CompareOptions>(m, "CompareOptions");
            ftk::python::observable<tl::CompareTime>(m, "CompareTime");

            nb::class_<FilesModel>(m, "FilesModel")
                .def(
                    nb::new_(&FilesModel::create),
                    nb::arg("settings"))

                .def("save", &FilesModel::save)

                .def_prop_ro("files", &FilesModel::getFiles)
                .def_prop_ro("observeFiles", &FilesModel::observeFiles)
                .def_prop_ro("a", &FilesModel::getA)
                .def_prop_ro("observeA", &FilesModel::observeA)
                .def_prop_ro("aIndex", &FilesModel::getAIndex)
                .def_prop_ro("observeAIndex", &FilesModel::observeAIndex)
                .def_prop_ro("b", &FilesModel::getB)
                .def_prop_ro("observeB", &FilesModel::observeB)
                .def_prop_ro("bIndexes", &FilesModel::getBIndexes)
                .def_prop_ro("observeBIndexes", &FilesModel::observeBIndexes)
                .def_prop_ro("active", &FilesModel::getActive)
                .def_prop_ro("observeActive", &FilesModel::observeActive)

                .def("add", nb::overload_cast<
                    const std::shared_ptr<FilesModelItem>&>(&FilesModel::add))
                .def("add", nb::overload_cast<
                    const std::vector<std::shared_ptr<FilesModelItem> >&>(&FilesModel::add))
                .def("move", &FilesModel::move, nb::arg("fromIndex"), nb::arg("toIndex"))
                .def("close", nb::overload_cast<>(&FilesModel::close))
                .def("close", nb::overload_cast<int>(&FilesModel::close), nb::arg("index"))
                .def("closeAll", &FilesModel::closeAll)
                .def("setA", &FilesModel::setA, nb::arg("index"))
                .def("setB", &FilesModel::setB, nb::arg("index"), nb::arg("value"))
                .def("toggleB", &FilesModel::toggleB, nb::arg("index"))
                .def("clearB", &FilesModel::clearB)
                .def("first", &FilesModel::first)
                .def("last", &FilesModel::last)
                .def("next", &FilesModel::next)
                .def("prev", &FilesModel::prev)
                .def("firstB", &FilesModel::firstB)
                .def("lastB", &FilesModel::lastB)
                .def("nextB", &FilesModel::nextB)
                .def("prevB", &FilesModel::prevB)

                .def_prop_ro("observeLayers", &FilesModel::observeLayers)
                .def("setLayer", &FilesModel::setLayer, nb::arg("item"), nb::arg("layer"))
                .def("setFrames", &FilesModel::setFrames, nb::arg("item"), nb::arg("range"))
                .def_prop_ro("observeReload", &FilesModel::observeReload)
                .def("refresh", &FilesModel::refresh)
                .def("nextLayer", &FilesModel::nextLayer)
                .def("prevLayer", &FilesModel::prevLayer)

                .def_prop_rw(
                    "compareOptions",
                    &FilesModel::getCompareOptions,
                    &FilesModel::setCompareOptions,
                    nb::rv_policy::copy)
                .def_prop_ro("observeCompareOptions", &FilesModel::observeCompareOptions)
                .def_prop_rw(
                    "compareTime",
                    &FilesModel::getCompareTime,
                    &FilesModel::setCompareTime)
                .def_prop_ro("observeCompareTime", &FilesModel::observeCompareTime);

            m.def("getCompareTimeLabels", &getCompareTimeLabels);
        }
    }
}
