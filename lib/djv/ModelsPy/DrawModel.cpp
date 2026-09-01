// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/DrawModel.h>

#include <ftk/UI/Settings.h>

#include <ftk/CorePy/Bindings.h>

#include <pybind11/stl.h>
#include <pybind11/functional.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void drawModel(py::module_& m)
        {
            using namespace models;

            py::enum_<DrawTool>(m, "DrawTool")
                .value("Pen", DrawTool::Pen)
                .value("Eraser", DrawTool::Eraser);

            ftk::python::observable<DrawTool>(m, "DrawTool");
            ftk::python::observable<ftk::Color4F>(m, "Color4F");

            py::class_<DrawModel, std::shared_ptr<DrawModel> >(m, "DrawModel")
                .def(
                    py::init(&DrawModel::create),
                    py::arg("settings"))
                .def_property("enabled", &DrawModel::isEnabled, &DrawModel::setEnabled)
                .def_property_readonly("observeEnabled", &DrawModel::observeEnabled)
                .def_property("tool", &DrawModel::getTool, &DrawModel::setTool)
                .def_property_readonly("observeTool", &DrawModel::observeTool)
                .def_property("color", &DrawModel::getColor, &DrawModel::setColor, py::return_value_policy::copy)
                .def_property_readonly("observeColor", &DrawModel::observeColor)
                .def_property("size", &DrawModel::getSize, &DrawModel::setSize)
                .def_property_readonly("observeSize", &DrawModel::observeSize);
        }
    }
}
