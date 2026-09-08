// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/UI/ViewWidgets.h>

#include <djv/Models/ViewportModel.h>

#include <ftk/UI/CheckBox.h>
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
        namespace
        {
            // The view widgets share one shape: created from the context
            // and the viewport model.
            template<typename T>
            void viewWidget(nb::module_& m, const char* name)
            {
                nb::class_<T, ftk::IContainer>(m, name)
                    .def(
                        nb::new_(&T::create),
                        nb::arg("context"),
                        nb::arg("viewportModel"),
                        nb::arg("parent") = nullptr);
            }

            // The toggled view widgets also expose the enabled check box,
            // for placing on a bellows.
            template<typename T>
            void viewToggleWidget(nb::module_& m, const char* name)
            {
                nb::class_<T, ftk::IContainer>(m, name)
                    .def(
                        nb::new_(&T::create),
                        nb::arg("context"),
                        nb::arg("viewportModel"),
                        nb::arg("parent") = nullptr)
                    .def_prop_ro("enabledCheckBox", &T::getEnabledCheckBox);
            }
        }

        void viewWidgets(nb::module_& m)
        {
            using namespace ui;

            viewWidget<ViewOptionsWidget>(m, "ViewOptionsWidget");
            viewWidget<ViewAspectRatioWidget>(m, "ViewAspectRatioWidget");
            viewWidget<ViewBackgroundWidget>(m, "ViewBackgroundWidget");
            viewToggleWidget<ViewOutlineWidget>(m, "ViewOutlineWidget");
            viewToggleWidget<ViewGridWidget>(m, "ViewGridWidget");
            viewToggleWidget<ViewCenterMarkerWidget>(m, "ViewCenterMarkerWidget");
            viewToggleWidget<ViewHUDWidget>(m, "ViewHUDWidget");
        }
    }
}
