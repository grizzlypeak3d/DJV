// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/ToolsModel.h>

#include <ftk/UI/Settings.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void toolsModel(py::module_& m)
        {
            using namespace models;

            py::class_<ToolInfo>(m, "ToolInfo")
                .def(py::init())
                .def_readwrite("name", &ToolInfo::name)
                .def_readwrite("icon", &ToolInfo::icon)
                .def_readwrite("sort", &ToolInfo::sort)
                .def_readwrite("toolBar", &ToolInfo::toolBar)
                .def_readwrite("shortcut", &ToolInfo::shortcut);

            py::class_<ToolsModel, std::shared_ptr<ToolsModel> >(m, "ToolsModel")
                .def(
                    py::init(&ToolsModel::create),
                    py::arg("settings"))
                .def("save", &ToolsModel::save)
                .def_property_readonly("tools", &ToolsModel::getTools, py::return_value_policy::copy)
                .def("addTool", &ToolsModel::addTool, py::arg("tool"))
                .def_property_readonly("openTools", &ToolsModel::getOpenTools)
                .def_property_readonly("observeOpenTools", &ToolsModel::observeOpenTools)
                .def("isToolOpen", &ToolsModel::isToolOpen, py::arg("name"))
                .def("setToolOpen", &ToolsModel::setToolOpen, py::arg("name"), py::arg("open"))
                .def("closeTools", &ToolsModel::closeTools);
        }
    }
}
