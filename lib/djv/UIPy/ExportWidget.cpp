// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/ExportWidget.h>

#include <djv/Models/ColorModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/Core/Context.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void exportWidget(py::module_& m)
        {
            using namespace ui;

            py::class_<ExportWidget, ftk::IContainer, std::shared_ptr<ExportWidget> >(m, "ExportWidget")
                .def(
                    py::init(&ExportWidget::create),
                    py::arg("context"),
                    py::arg("filesModel"),
                    py::arg("colorModel"),
                    py::arg("viewportModel"),
                    py::arg("settingsModel"),
                    py::arg("timeUnitsModel"),
                    py::arg("parent") = nullptr)
                .def("setPlayer", &ExportWidget::setPlayer, py::arg("player"));
        }
    }
}
