// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/AppPy/Bindings.h>

namespace nb = nanobind;

namespace djv
{
    namespace python
    {
        void appBind(nb::module_& m)
        {
            nb::module_ sm = m.def_submodule(
                "app",
                "The application: the C++ one, which a Python program "
                "starts and extends with tools and commands of its own.");
            mainWindow(sm);
            toolWidget(sm);
            app(sm);
        }
    }
}
