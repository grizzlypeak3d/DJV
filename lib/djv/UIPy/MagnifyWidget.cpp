// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/UI/MagnifyWidget.h>

#include <djv/Models/ColorModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/ViewportModel.h>

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
        void magnifyWidget(nb::module_& m)
        {
            using namespace ui;

            nb::class_<MagnifyWidget, ftk::IContainer>(m, "MagnifyWidget")
                .def(
                    nb::new_(&MagnifyWidget::create),
                    nb::arg("context"),
                    nb::arg("settings"),
                    nb::arg("viewport"),
                    nb::arg("filesModel"),
                    nb::arg("colorModel"),
                    nb::arg("viewportModel"),
                    nb::arg("settingsModel"),
                    nb::arg("parent") = nullptr)
                .def("setPlayer", &MagnifyWidget::setPlayer, nb::arg("player"));
        }
    }
}
