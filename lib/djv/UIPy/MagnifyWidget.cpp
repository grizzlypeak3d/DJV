// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/MagnifyWidget.h>

#include <djv/Models/ColorModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/UI/Settings.h>

#include <ftk/Core/Context.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void magnifyWidget(py::module_& m)
        {
            using namespace ui;

            py::class_<MagnifyWidget, ftk::IContainer, std::shared_ptr<MagnifyWidget> >(m, "MagnifyWidget")
                .def(
                    py::init(&MagnifyWidget::create),
                    py::arg("context"),
                    py::arg("settings"),
                    py::arg("viewport"),
                    py::arg("filesModel"),
                    py::arg("colorModel"),
                    py::arg("viewportModel"),
                    py::arg("settingsModel"),
                    py::arg("parent") = nullptr)
                .def("setPlayer", &MagnifyWidget::setPlayer, py::arg("player"));
        }
    }
}
