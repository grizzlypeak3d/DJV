// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/SpeedPopup.h>

#include <ftk/UI/DoubleModel.h>
#include <ftk/Core/Context.h>

#include <pybind11/functional.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void speedPopup(py::module_& m)
        {
            using namespace ui;

            py::class_<SpeedPopup, ftk::IWidgetPopup, std::shared_ptr<SpeedPopup> >(m, "SpeedPopup")
                .def(
                    py::init(&SpeedPopup::create),
                    py::arg("context"),
                    py::arg("model"),
                    py::arg("defaultSpeed"),
                    py::arg("parent") = nullptr)
                .def("setCallback", &SpeedPopup::setCallback);
        }
    }
}
