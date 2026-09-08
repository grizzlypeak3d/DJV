// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/UI/FrameRangePopup.h>

#include <ftk/Core/Context.h>

#include <nanobind/stl/function.h>
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
        void frameRangePopup(nb::module_& m)
        {
            using namespace ui;

            nb::class_<FrameRangePopup, ftk::IWidgetPopup>(m, "FrameRangePopup")
                .def(
                    nb::new_(&FrameRangePopup::create),
                    nb::arg("context"),
                    nb::arg("range"),
                    nb::arg("parent") = nullptr)
                .def("setCallback", &FrameRangePopup::setCallback);
        }
    }
}
