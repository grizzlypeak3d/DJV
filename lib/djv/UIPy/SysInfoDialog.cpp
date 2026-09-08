// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/UI/SysInfoDialog.h>

#include <djv/Models/AppInfoModel.h>
#include <djv/Models/SettingsModel.h>

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
        void sysInfoDialog(nb::module_& m)
        {
            using namespace ui;

            m.def(
                "getSysInfo",
                &getSysInfo,
                nb::arg("context"),
                nb::arg("appInfoModel"),
                nb::arg("settingsModel"),
                nb::arg("windowInfo") =
                    std::vector<std::pair<std::string, std::string> >());

            nb::class_<SysInfoDialog, ftk::IDialog>(m, "SysInfoDialog")
                .def(
                    nb::new_(&SysInfoDialog::create),
                    nb::arg("context"),
                    nb::arg("text"),
                    nb::arg("parent") = nullptr);
        }
    }
}
