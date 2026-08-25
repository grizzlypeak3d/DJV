// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/AudioPopup.h>

#include <djv/Models/AudioModel.h>

#include <ftk/Core/Context.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void audioPopup(py::module_& m)
        {
            using namespace ui;

            py::class_<AudioPopup, ftk::IWidgetPopup, std::shared_ptr<AudioPopup> >(m, "AudioPopup")
                .def(
                    py::init(&AudioPopup::create),
                    py::arg("context"),
                    py::arg("audioModel"),
                    py::arg("parent") = nullptr);
        }
    }
}
