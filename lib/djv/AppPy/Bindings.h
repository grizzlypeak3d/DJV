// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <nanobind/nanobind.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/function.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace djv
{
    namespace python
    {
        void app(nanobind::module_&);
        void mainWindow(nanobind::module_&);
        void toolWidget(nanobind::module_&);

        void appBind(nanobind::module_&);
    }
}
