// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/Viewport.h>

#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/UI/SysLogModel.h>

#include <ftk/Core/Context.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void viewport(py::module_& m)
        {
            using namespace ui;

            py::class_<Viewport, tl::ui::Viewport, std::shared_ptr<Viewport> >(m, "Viewport")
                .def(
                    py::init(&Viewport::create),
                    py::arg("context"),
                    py::arg("filesModel"),
                    py::arg("colorModel"),
                    py::arg("viewportModel"),
                    py::arg("timeUnitsModel"),
                    py::arg("settingsModel"),
                    py::arg("annotationsModel"),
                    py::arg("drawModel"),
                    py::arg("sysLogModel"),
                    py::arg("parent") = nullptr)
                .def("setToastActive", &Viewport::setToastActive)
                .def("setHUDActive", &Viewport::setHUDActive);
        }
    }
}
