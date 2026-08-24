// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/ColorWidgets.h>

#include <djv/Models/ColorModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/UI/CheckBox.h>
#include <ftk/UI/Settings.h>

#include <ftk/Core/Context.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void colorWidgets(py::module_& m)
        {
            using namespace ui;

            py::class_<OCIOWidget, ftk::IContainer, std::shared_ptr<OCIOWidget> >(m, "OCIOWidget")
                .def(
                    py::init(&OCIOWidget::create),
                    py::arg("context"),
                    py::arg("colorModel"),
                    py::arg("parent") = nullptr)
                .def_property_readonly("enabledCheckBox", &OCIOWidget::getEnabledCheckBox);

            py::class_<LUTWidget, ftk::IContainer, std::shared_ptr<LUTWidget> >(m, "LUTWidget")
                .def(
                    py::init(&LUTWidget::create),
                    py::arg("context"),
                    py::arg("colorModel"),
                    py::arg("parent") = nullptr)
                .def_property_readonly("enabledCheckBox", &LUTWidget::getEnabledCheckBox);

            py::class_<ColorWidget, ftk::IContainer, std::shared_ptr<ColorWidget> >(m, "ColorWidget")
                .def(
                    py::init(&ColorWidget::create),
                    py::arg("context"),
                    py::arg("viewportModel"),
                    py::arg("parent") = nullptr)
                .def_property_readonly("enabledCheckBox", &ColorWidget::getEnabledCheckBox);

            py::class_<LevelsWidget, ftk::IContainer, std::shared_ptr<LevelsWidget> >(m, "LevelsWidget")
                .def(
                    py::init(&LevelsWidget::create),
                    py::arg("context"),
                    py::arg("settings"),
                    py::arg("viewportModel"),
                    py::arg("parent") = nullptr)
                .def_property_readonly("enabledCheckBox", &LevelsWidget::getEnabledCheckBox);

            py::class_<ExposureWidget, ftk::IContainer, std::shared_ptr<ExposureWidget> >(m, "ExposureWidget")
                .def(
                    py::init(&ExposureWidget::create),
                    py::arg("context"),
                    py::arg("viewportModel"),
                    py::arg("parent") = nullptr)
                .def_property_readonly("enabledCheckBox", &ExposureWidget::getEnabledCheckBox);

            py::class_<SoftClipWidget, ftk::IContainer, std::shared_ptr<SoftClipWidget> >(m, "SoftClipWidget")
                .def(
                    py::init(&SoftClipWidget::create),
                    py::arg("context"),
                    py::arg("viewportModel"),
                    py::arg("parent") = nullptr)
                .def_property_readonly("enabledCheckBox", &SoftClipWidget::getEnabledCheckBox);
        }
    }
}
