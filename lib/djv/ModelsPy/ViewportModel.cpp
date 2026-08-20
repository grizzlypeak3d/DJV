// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/ViewportModel.h>

#include <ftk/CorePy/Bindings.h>
#include <ftk/UI/Settings.h>
#include <ftk/Core/Context.h>

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void viewportModel(py::module_& m)
        {
            using namespace models;

            py::class_<AspectRatioOptions>(m, "AspectRatioOptions")
                .def(py::init())
                .def_readwrite("index", &AspectRatioOptions::index)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            py::enum_<HUDItem>(m, "HUDItem")
                .value("FileName", HUDItem::FileName)
                .value("Info", HUDItem::Info)
                .value("Cache", HUDItem::Cache)
                .value("Time", HUDItem::Time)
                .value("ViewZoom", HUDItem::ViewZoom)
                .value("ColorPicker", HUDItem::ColorPicker)
                .value("Render", HUDItem::Render);
            FTK_ENUM_BIND(m, HUDItem);

            py::enum_<HUDPos>(m, "HUDPos")
                .value("None", HUDPos::None)
                .value("TopLeft", HUDPos::TopLeft)
                .value("TopRight", HUDPos::TopRight)
                .value("BottomLeft", HUDPos::BottomLeft)
                .value("BottomRight", HUDPos::BottomRight);
            FTK_ENUM_BIND(m, HUDPos);

            py::class_<HUDOptions>(m, "HUDOptions")
                .def(py::init())
                .def_readwrite("enabled", &HUDOptions::enabled)
                .def_readwrite("items", &HUDOptions::items)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            ftk::python::observable<ftk::ImageOptions>(m, "ImageOptions");
            ftk::python::observable<tl::DisplayOptions>(m, "DisplayOptions");
            ftk::python::observable<AspectRatioOptions>(m, "AspectRatioOptions");
            ftk::python::observable<tl::BackgroundOptions>(m, "BackgroundOptions");
            ftk::python::observable<tl::ForegroundOptions>(m, "ForegroundOptions");
            ftk::python::observable<HUDOptions>(m, "HUDOptions");

            py::class_<ViewportModel, std::shared_ptr<ViewportModel> >(m, "ViewportModel")
                .def(
                    py::init(&ViewportModel::create),
                    py::arg("context"),
                    py::arg("settings"))
                .def_property("imageOptions", &ViewportModel::getImageOptions, &ViewportModel::setImageOptions, py::return_value_policy::copy)
                .def_property_readonly("observeImageOptions", &ViewportModel::observeImageOptions)
                .def_property("displayOptions", &ViewportModel::getDisplayOptions, &ViewportModel::setDisplayOptions, py::return_value_policy::copy)
                .def_property_readonly("observeDisplayOptions", &ViewportModel::observeDisplayOptions)
                .def_property("aspectRatioOptions", &ViewportModel::getAspectRatioOptions, &ViewportModel::setAspectRatioOptions, py::return_value_policy::copy)
                .def_property_readonly("observeAspectRatioOptions", &ViewportModel::observeAspectRatioOptions)
                .def_property("backgroundOptions", &ViewportModel::getBackgroundOptions, &ViewportModel::setBackgroundOptions, py::return_value_policy::copy)
                .def_property_readonly("observeBackgroundOptions", &ViewportModel::observeBackgroundOptions)
                .def_property("foregroundOptions", &ViewportModel::getForegroundOptions, &ViewportModel::setForegroundOptions, py::return_value_policy::copy)
                .def_property_readonly("observeForegroundOptions", &ViewportModel::observeForegroundOptions)
                .def_property("hudOptions", &ViewportModel::getHUDOptions, &ViewportModel::setHUDOptions, py::return_value_policy::copy)
                .def_property_readonly("observeHUDOptions", &ViewportModel::observeHUDOptions);
        }
    }
}
