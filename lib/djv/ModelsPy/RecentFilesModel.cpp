// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/RecentFilesModel.h>

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
        void recentFilesModel(nb::module_& m)
        {
            using namespace models;

            nb::class_<
                RecentFilesModel,
                ftk::RecentFilesModel>(m, "RecentFilesModel")
                .def(
                    nb::new_(&RecentFilesModel::create),
                    nb::arg("context"),
                    nb::arg("settings"),
                    nb::arg("settingsGroup") = "Files")
                .def("save", &RecentFilesModel::save);
        }
    }
}
