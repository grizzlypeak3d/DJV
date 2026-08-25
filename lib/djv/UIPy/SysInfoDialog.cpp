// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/SysInfoDialog.h>

#include <djv/Models/AppInfoModel.h>
#include <djv/Models/SettingsModel.h>

#include <ftk/Core/Context.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void sysInfoDialog(py::module_& m)
        {
            using namespace ui;

            m.def(
                "getSysInfo",
                &getSysInfo,
                py::arg("context"),
                py::arg("appInfoModel"),
                py::arg("settingsModel"),
                py::arg("windowInfo") =
                    std::vector<std::pair<std::string, std::string> >());

            py::class_<SysInfoDialog, ftk::IDialog, std::shared_ptr<SysInfoDialog> >(m, "SysInfoDialog")
                .def(
                    py::init(&SysInfoDialog::create),
                    py::arg("context"),
                    py::arg("text"),
                    py::arg("parent") = nullptr);
        }
    }
}
