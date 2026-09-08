// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/UI/Init.h>

#include <ftk/Core/Context.h>

namespace nb = nanobind;

namespace djv
{
    namespace python
    {
        void uiInit(nb::module_& m)
        {
            m.def(
                "initIcons",
                &ui::initIcons,
                nb::arg("context"));
        }
    }
}
