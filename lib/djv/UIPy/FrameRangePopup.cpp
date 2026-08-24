// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/FrameRangePopup.h>

#include <ftk/Core/Context.h>

#include <pybind11/functional.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void frameRangePopup(py::module_& m)
        {
            using namespace ui;

            py::class_<FrameRangePopup, ftk::IWidgetPopup, std::shared_ptr<FrameRangePopup> >(m, "FrameRangePopup")
                .def(
                    py::init(&FrameRangePopup::create),
                    py::arg("context"),
                    py::arg("range"),
                    py::arg("parent") = nullptr)
                .def("setCallback", &FrameRangePopup::setCallback);
        }
    }
}
