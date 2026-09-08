// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/UI/StatusIndicator.h>

#include <djv/Models/AudioModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/Core/Context.h>

#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/filesystem.h>

namespace nb = nanobind;

namespace djv
{
    namespace python
    {
        void statusIndicator(nb::module_& m)
        {
            using namespace ui;

            nb::class_<StatusIndicator, ftk::IContainer>(m, "StatusIndicator")
                .def(
                    nb::new_(&StatusIndicator::create),
                    nb::arg("context"),
                    nb::arg("viewportModel"),
                    nb::arg("colorModel"),
                    nb::arg("audioModel"),
                    nb::arg("parent") = nullptr);
        }
    }
}
