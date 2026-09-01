// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/Init.h>

#include <ftk/Core/Context.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void uiInit(py::module_& m)
        {
            m.def(
                "initIcons",
                &ui::initIcons,
                py::arg("context"));
        }
    }
}
