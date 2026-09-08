// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/UI/Viewport.h>

#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/UI/SysLogModel.h>

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
        void viewport(nb::module_& m)
        {
            using namespace ui;

            nb::class_<Viewport, tl::ui::Viewport>(
                m, "Viewport",
                // The Python examples hold this through weakref, which
                // nanobind classes opt into.
                nb::is_weak_referenceable())
                .def(
                    nb::new_(&Viewport::create),
                    nb::arg("context"),
                    nb::arg("filesModel"),
                    nb::arg("colorModel"),
                    nb::arg("viewportModel"),
                    nb::arg("timeUnitsModel"),
                    nb::arg("settingsModel"),
                    nb::arg("annotationsModel"),
                    nb::arg("drawModel"),
                    nb::arg("sysLogModel"),
                    nb::arg("parent") = nullptr)
                .def("setToastActive", &Viewport::setToastActive)
                .def("setHUDActive", &Viewport::setHUDActive);
        }
    }
}
