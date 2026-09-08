// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/ViewportModel.h>

#include <ftk/CorePy/Bindings.h>
#include <ftk/UI/Settings.h>
#include <ftk/Core/Context.h>

#include <nanobind/stl/function.h>
#include <nanobind/operators.h>
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
        void viewportModel(nb::module_& m)
        {
            using namespace models;

            nb::class_<AspectRatioOptions>(m, "AspectRatioOptions")
                .def(nb::init())
                .def_rw("index", &AspectRatioOptions::index)
                .def_rw("options", &AspectRatioOptions::options)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            FTK_ENUM_PY(m, HUDItem);
            FTK_ENUM_BIND(m, HUDItem);

            FTK_ENUM_PY(m, HUDPos);
            FTK_ENUM_BIND(m, HUDPos);

            nb::class_<HUDOptions>(m, "HUDOptions")
                .def(nb::init())
                .def_rw("enabled", &HUDOptions::enabled)
                .def_rw("items", &HUDOptions::items)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            ftk::python::observable<ftk::ImageOptions>(m, "ImageOptions");
            ftk::python::observable<tl::DisplayOptions>(m, "DisplayOptions");
            ftk::python::observable<AspectRatioOptions>(m, "AspectRatioOptions");
            ftk::python::observable<tl::BackgroundOptions>(m, "BackgroundOptions");
            ftk::python::observable<tl::ForegroundOptions>(m, "ForegroundOptions");
            ftk::python::observable<HUDOptions>(m, "HUDOptions");

            nb::class_<ViewportModel>(m, "ViewportModel")
                .def(
                    nb::new_(&ViewportModel::create),
                    nb::arg("context"),
                    nb::arg("settings"))
                .def("save", &ViewportModel::save)
                .def_prop_rw("imageOptions", &ViewportModel::getImageOptions, &ViewportModel::setImageOptions, nb::rv_policy::copy)
                .def_prop_ro("observeImageOptions", &ViewportModel::observeImageOptions)
                .def_prop_rw("displayOptions", &ViewportModel::getDisplayOptions, &ViewportModel::setDisplayOptions, nb::rv_policy::copy)
                .def_prop_ro("observeDisplayOptions", &ViewportModel::observeDisplayOptions)
                .def_prop_rw("aspectRatioOptions", &ViewportModel::getAspectRatioOptions, &ViewportModel::setAspectRatioOptions, nb::rv_policy::copy)
                .def_prop_ro("observeAspectRatioOptions", &ViewportModel::observeAspectRatioOptions)
                .def_prop_rw("backgroundOptions", &ViewportModel::getBackgroundOptions, &ViewportModel::setBackgroundOptions, nb::rv_policy::copy)
                .def_prop_ro("observeBackgroundOptions", &ViewportModel::observeBackgroundOptions)
                .def_prop_rw("foregroundOptions", &ViewportModel::getForegroundOptions, &ViewportModel::setForegroundOptions, nb::rv_policy::copy)
                .def_prop_ro("observeForegroundOptions", &ViewportModel::observeForegroundOptions)
                .def_prop_rw("hudOptions", &ViewportModel::getHUDOptions, &ViewportModel::setHUDOptions, nb::rv_policy::copy)
                .def_prop_ro("observeHUDOptions", &ViewportModel::observeHUDOptions);
        }
    }
}
