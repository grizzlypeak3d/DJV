// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/RecentFilesModel.h>

#include <ftk/UI/Settings.h>

#include <ftk/Core/Context.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void recentFilesModel(py::module_& m)
        {
            using namespace models;

            py::class_<
                RecentFilesModel,
                ftk::RecentFilesModel,
                std::shared_ptr<RecentFilesModel> >(m, "RecentFilesModel")
                .def(
                    py::init(&RecentFilesModel::create),
                    py::arg("context"),
                    py::arg("settings"));
        }
    }
}
