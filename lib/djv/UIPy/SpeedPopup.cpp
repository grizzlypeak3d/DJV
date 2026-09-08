// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/UI/SpeedPopup.h>

#include <ftk/UI/DoubleModel.h>
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
        void speedPopup(nb::module_& m)
        {
            using namespace ui;

            nb::class_<SpeedPopup, ftk::IWidgetPopup>(m, "SpeedPopup")
                .def(
                    nb::new_(&SpeedPopup::create),
                    nb::arg("context"),
                    nb::arg("model"),
                    nb::arg("defaultSpeed"),
                    nb::arg("parent") = nullptr)
                .def("setCallback", &SpeedPopup::setCallback);
        }
    }
}
