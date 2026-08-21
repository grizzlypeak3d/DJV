// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <pybind11/pybind11.h>

namespace djv
{
    namespace python
    {
        void appInfoModel(pybind11::module_&);
        void audioModel(pybind11::module_&);
        void colorModel(pybind11::module_&);
        void commandsModel(pybind11::module_&);
        void filesModel(pybind11::module_&);
        void ocioModel(pybind11::module_&);
        void recentFilesModel(pybind11::module_&);
        void settingsModel(pybind11::module_&);
        void shortcuts(pybind11::module_&);
        void timeUnitsModel(pybind11::module_&);
        void toolsModel(pybind11::module_&);
        void viewportModel(pybind11::module_&);

        DJV_API void modelsBind(pybind11::module_&);
    }
}
