// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/InfoWidget.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/Settings.h>
#include <ftk/Core/Context.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void infoWidget(py::module_& m)
        {
            using namespace ui;

            py::class_<InfoWidget, ftk::IContainer, std::shared_ptr<InfoWidget> >(m, "InfoWidget")
                .def(
                    py::init(&InfoWidget::create),
                    py::arg("context"),
                    py::arg("settings"),
                    py::arg("parent") = nullptr)
                .def("setPlayer", &InfoWidget::setPlayer, py::arg("player"));
        }
    }
}
