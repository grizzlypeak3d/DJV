// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/ToolsModel.h>

#include <ftk/UI/Settings.h>

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
        void toolsModel(nb::module_& m)
        {
            using namespace models;

            nb::class_<ToolInfo>(m, "ToolInfo")
                .def(nb::init())
                .def_rw("name", &ToolInfo::name)
                .def_rw("icon", &ToolInfo::icon)
                .def_rw("sort", &ToolInfo::sort)
                .def_rw("toolBar", &ToolInfo::toolBar)
                .def_rw("shortcut", &ToolInfo::shortcut);

            nb::class_<ToolsModel>(m, "ToolsModel")
                .def(
                    nb::new_(&ToolsModel::create),
                    nb::arg("settings"))
                .def("save", &ToolsModel::save)
                .def_prop_ro("tools", &ToolsModel::getTools, nb::rv_policy::copy)
                .def("addTool", &ToolsModel::addTool, nb::arg("tool"))
                .def_prop_ro("openTools", &ToolsModel::getOpenTools)
                .def_prop_ro("observeOpenTools", &ToolsModel::observeOpenTools)
                .def("isToolOpen", &ToolsModel::isToolOpen, nb::arg("name"))
                .def("setToolOpen", &ToolsModel::setToolOpen, nb::arg("name"), nb::arg("open"))
                .def("closeTools", &ToolsModel::closeTools);
        }
    }
}
