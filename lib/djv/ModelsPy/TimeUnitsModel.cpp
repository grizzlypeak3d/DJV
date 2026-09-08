// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/TimeUnitsModel.h>

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
        void timeUnitsModel(nb::module_& m)
        {
            using namespace models;

            nb::class_<TimeUnitsModel, tl::TimeUnitsModel>(m, "TimeUnitsModel")
                .def(
                    nb::new_(&TimeUnitsModel::create),
                    nb::arg("context"),
                    nb::arg("settings"))
                .def("save", &TimeUnitsModel::save);
        }
    }
}
