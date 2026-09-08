// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/UI/InfoWidget.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/Settings.h>
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
        void infoWidget(nb::module_& m)
        {
            using namespace ui;

            nb::class_<InfoWidget, ftk::IContainer>(m, "InfoWidget")
                .def(
                    nb::new_(&InfoWidget::create),
                    nb::arg("context"),
                    nb::arg("settings"),
                    nb::arg("parent") = nullptr)
                .def("setPlayer", &InfoWidget::setPlayer, nb::arg("player"));
        }
    }
}
