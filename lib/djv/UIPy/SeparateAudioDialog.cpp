// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/SeparateAudioDialog.h>

#include <ftk/Core/Context.h>

#include <pybind11/functional.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void separateAudioDialog(py::module_& m)
        {
            using namespace ui;

            py::class_<SeparateAudioDialog, ftk::IDialog, std::shared_ptr<SeparateAudioDialog> >(m, "SeparateAudioDialog")
                .def(
                    py::init(&SeparateAudioDialog::create),
                    py::arg("context"),
                    py::arg("parent") = nullptr)
                .def("setCallback", &SeparateAudioDialog::setCallback);
        }
    }
}
