// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/AppInfoModel.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void appInfoModel(py::module_& m)
        {
            using namespace models;

            py::class_<AppInfoModel, std::shared_ptr<AppInfoModel> >(m, "AppInfoModel")
                .def(py::init(&AppInfoModel::create))

                .def_property_readonly("fullName", &AppInfoModel::getFullName)
                .def_property_readonly("shortName", &AppInfoModel::getShortName)
                .def_property_readonly("docsDirName", &AppInfoModel::getDocsDirName)

                .def_property_readonly("versionMajor", &AppInfoModel::getVersionMajor)
                .def_property_readonly("versionMinor", &AppInfoModel::getVersionMinor)
                .def_property_readonly("versionPatch", &AppInfoModel::getVersionPatch)
                .def_property_readonly("versionDev", &AppInfoModel::getVersionDev)
                .def_property_readonly("version", &AppInfoModel::getVersion)
                .def_property_readonly("commitDate", &AppInfoModel::getCommitDate)
                .def_property_readonly("gitCommit", &AppInfoModel::getGitCommit)

                .def_property_readonly("docsURL", &AppInfoModel::getDocsURL)

                .def_property_readonly("license", &AppInfoModel::getLicense)
                .def_property_readonly("licensesURL", &AppInfoModel::getLicensesURL);
        }
    }
}
