// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/AppInfoModel.h>

#include <nanobind/nanobind.h>
#include <nanobind/trampoline.h>

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
        //! The name getters are virtual so that an application built on DJV
        //! can say who it is -- the trampoline lets a Python application do
        //! the same.
        class PyAppInfoModel : public models::AppInfoModel
        {
        public:
            NB_TRAMPOLINE(models::AppInfoModel);

            std::string getFullName() const override
            {
                NB_OVERRIDE(getFullName);
            }

            std::string getShortName() const override
            {
                NB_OVERRIDE(getShortName);
            }

            std::string getDocsDirName() const override
            {
                NB_OVERRIDE(getDocsDirName);
            }

            std::string getTitle() const override
            {
                NB_OVERRIDE(getTitle);
            }

            std::string getStudioURL() const override
            {
                NB_OVERRIDE(getStudioURL);
            }

            std::string getDocsSearchPath() const override
            {
                NB_OVERRIDE(getDocsSearchPath);
            }
        };

        void appInfoModel(nb::module_& m)
        {
            using namespace models;

            nb::class_<AppInfoModel, PyAppInfoModel>(m, "AppInfoModel")
                .def(
                    "__init__",
                    // A trampolined class: Python owns the storage, so no
                    // create() factory (see WidgetTrampoline.h in ftk).
                    [](AppInfoModel* self)
                    {
                        new (static_cast<void*>(self)) PyAppInfoModel;
                    })

                // The virtual getters by their method names as well as
                // through the properties below: the trampoline's override
                // lookup needs the attributes to exist, and a Python
                // subclass gets super().
                .def("getFullName", &AppInfoModel::getFullName)
                .def("getShortName", &AppInfoModel::getShortName)
                .def("getDocsDirName", &AppInfoModel::getDocsDirName)
                .def("getTitle", &AppInfoModel::getTitle)
                .def("getStudioURL", &AppInfoModel::getStudioURL)
                .def("getDocsSearchPath", &AppInfoModel::getDocsSearchPath)

                .def_prop_ro("fullName", &AppInfoModel::getFullName)
                .def_prop_ro("shortName", &AppInfoModel::getShortName)
                .def_prop_ro("docsDirName", &AppInfoModel::getDocsDirName)

                .def_prop_ro("versionMajor", &AppInfoModel::getVersionMajor)
                .def_prop_ro("versionMinor", &AppInfoModel::getVersionMinor)
                .def_prop_ro("versionPatch", &AppInfoModel::getVersionPatch)
                .def_prop_ro("versionDev", &AppInfoModel::getVersionDev)
                .def_prop_ro("version", &AppInfoModel::getVersion)
                .def_prop_ro("commitDate", &AppInfoModel::getCommitDate)
                .def_prop_ro("gitCommit", &AppInfoModel::getGitCommit)

                .def_prop_ro("title", &AppInfoModel::getTitle)

                .def_prop_ro("docsURL", &AppInfoModel::getDocsURL)
                .def_prop_ro("docsSearchPath", &AppInfoModel::getDocsSearchPath)

                .def_prop_ro("license", &AppInfoModel::getLicense)
                .def_prop_ro("licensesURL", &AppInfoModel::getLicensesURL)

                .def_prop_ro("studioURL", &AppInfoModel::getStudioURL);
        }
    }
}
