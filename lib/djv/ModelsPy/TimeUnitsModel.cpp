// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/TimeUnitsModel.h>

#include <ftk/UI/Settings.h>
#include <ftk/Core/Context.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void timeUnitsModel(py::module_& m)
        {
            using namespace models;

            py::class_<TimeUnitsModel, tl::TimeUnitsModel, std::shared_ptr<TimeUnitsModel> >(m, "TimeUnitsModel")
                .def(
                    py::init(&TimeUnitsModel::create),
                    py::arg("context"),
                    py::arg("settings"));
        }
    }
}
