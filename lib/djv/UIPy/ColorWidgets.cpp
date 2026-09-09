// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/UI/ColorWidgets.h>

#include <djv/Models/ColorModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/UI/CheckBox.h>
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
        void colorWidgets(nb::module_& m)
        {
            using namespace ui;

            nb::class_<OCIOWidget, ftk::IContainer>(m, "OCIOWidget")
                .def(
                    nb::new_(&OCIOWidget::create),
                    nb::arg("context"),
                    nb::arg("colorModel"),
                    nb::arg("parent") = nullptr)
                .def_prop_ro("enabledCheckBox", &OCIOWidget::getEnabledCheckBox);

            nb::class_<LUTWidget, ftk::IContainer>(m, "LUTWidget")
                .def(
                    nb::new_(&LUTWidget::create),
                    nb::arg("context"),
                    nb::arg("colorModel"),
                    nb::arg("parent") = nullptr)
                .def_prop_ro("enabledCheckBox", &LUTWidget::getEnabledCheckBox);

            nb::class_<ColorWidget, ftk::IContainer>(m, "ColorWidget")
                .def(
                    nb::new_(&ColorWidget::create),
                    nb::arg("context"),
                    nb::arg("viewportModel"),
                    nb::arg("parent") = nullptr)
                .def_prop_ro("enabledCheckBox", &ColorWidget::getEnabledCheckBox);

            nb::class_<LevelsWidget, ftk::IContainer>(m, "LevelsWidget")
                .def(
                    nb::new_(&LevelsWidget::create),
                    nb::arg("context"),
                    nb::arg("settings"),
                    nb::arg("viewportModel"),
                    nb::arg("parent") = nullptr)
                .def_prop_ro("enabledCheckBox", &LevelsWidget::getEnabledCheckBox);

            nb::class_<ExposureWidget, ftk::IContainer>(m, "ExposureWidget")
                .def(
                    nb::new_(&ExposureWidget::create),
                    nb::arg("context"),
                    nb::arg("viewportModel"),
                    nb::arg("parent") = nullptr)
                .def_prop_ro("enabledCheckBox", &ExposureWidget::getEnabledCheckBox);

            nb::class_<SoftClipWidget, ftk::IContainer>(m, "SoftClipWidget")
                .def(
                    nb::new_(&SoftClipWidget::create),
                    nb::arg("context"),
                    nb::arg("viewportModel"),
                    nb::arg("parent") = nullptr)
                .def_prop_ro("enabledCheckBox", &SoftClipWidget::getEnabledCheckBox);

            nb::class_<ClipWarningWidget, ftk::IContainer>(m, "ClipWarningWidget")
                .def(
                    nb::new_(&ClipWarningWidget::create),
                    nb::arg("context"),
                    nb::arg("viewportModel"),
                    nb::arg("parent") = nullptr)
                .def_prop_ro("enabledCheckBox", &ClipWarningWidget::getEnabledCheckBox);
        }
    }
}
