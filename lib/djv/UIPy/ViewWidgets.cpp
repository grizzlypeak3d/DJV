// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/ViewWidgets.h>

#include <djv/Models/ViewportModel.h>

#include <ftk/Core/Context.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        namespace
        {
            // The view widgets share one shape: created from the context
            // and the viewport model, with nothing else to bind.
            template<typename T>
            void viewWidget(py::module_& m, const char* name)
            {
                py::class_<T, ftk::IContainer, std::shared_ptr<T> >(m, name)
                    .def(
                        py::init(&T::create),
                        py::arg("context"),
                        py::arg("viewportModel"),
                        py::arg("parent") = nullptr);
            }
        }

        void viewWidgets(py::module_& m)
        {
            using namespace ui;

            viewWidget<ViewOptionsWidget>(m, "ViewOptionsWidget");
            viewWidget<ViewAspectRatioWidget>(m, "ViewAspectRatioWidget");
            viewWidget<ViewBackgroundWidget>(m, "ViewBackgroundWidget");
            viewWidget<ViewOutlineWidget>(m, "ViewOutlineWidget");
            viewWidget<ViewGridWidget>(m, "ViewGridWidget");
            viewWidget<ViewCenterMarkerWidget>(m, "ViewCenterMarkerWidget");
            viewWidget<ViewHUDWidget>(m, "ViewHUDWidget");
        }
    }
}
