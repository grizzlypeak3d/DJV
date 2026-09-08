// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/DrawModel.h>

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
        void drawModel(nb::module_& m)
        {
            using namespace models;

            nb::enum_<DrawTool>(m, "DrawTool")
                .value("Pen", DrawTool::Pen)
                .value("Eraser", DrawTool::Eraser);

            ftk::python::observable<DrawTool>(m, "DrawTool");
            ftk::python::observable<ftk::Color4F>(m, "Color4F");

            nb::class_<DrawModel>(m, "DrawModel")
                .def(
                    nb::new_(&DrawModel::create),
                    nb::arg("settings"))
                .def_prop_rw("enabled", &DrawModel::isEnabled, &DrawModel::setEnabled)
                .def_prop_ro("observeEnabled", &DrawModel::observeEnabled)
                .def_prop_rw("tool", &DrawModel::getTool, &DrawModel::setTool)
                .def_prop_ro("observeTool", &DrawModel::observeTool)
                .def_prop_rw("color", &DrawModel::getColor, &DrawModel::setColor, nb::rv_policy::copy)
                .def_prop_ro("observeColor", &DrawModel::observeColor)
                .def_prop_rw("size", &DrawModel::getSize, &DrawModel::setSize)
                .def_prop_ro("observeSize", &DrawModel::observeSize);
        }
    }
}
