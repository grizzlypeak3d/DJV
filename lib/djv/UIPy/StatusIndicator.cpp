// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/StatusIndicator.h>

#include <djv/Models/AudioModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/Core/Context.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void statusIndicator(py::module_& m)
        {
            using namespace ui;

            py::class_<StatusIndicator, ftk::IContainer, std::shared_ptr<StatusIndicator> >(m, "StatusIndicator")
                .def(
                    py::init(&StatusIndicator::create),
                    py::arg("context"),
                    py::arg("viewportModel"),
                    py::arg("colorModel"),
                    py::arg("audioModel"),
                    py::arg("parent") = nullptr);
        }
    }
}
