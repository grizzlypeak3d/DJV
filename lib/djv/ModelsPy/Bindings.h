// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once


#include <pybind11/pybind11.h>

namespace djv
{
    namespace python
    {
        void annotationsModel(pybind11::module_&);
        void appInfoModel(pybind11::module_&);
        void audioModel(pybind11::module_&);
        void colorModel(pybind11::module_&);
        void commandsModel(pybind11::module_&);
        void drawModel(pybind11::module_&);
        void filesModel(pybind11::module_&);
        void notesModel(pybind11::module_&);
        void ocioModel(pybind11::module_&);
        void parse(pybind11::module_&);
        void playlist(pybind11::module_&);
        void rangesModel(pybind11::module_&);
        void recentFilesModel(pybind11::module_&);
        void review(pybind11::module_&);
        void settingsModel(pybind11::module_&);
        void shortcuts(pybind11::module_&);
        void timeUnitsModel(pybind11::module_&);
        void toolsModel(pybind11::module_&);
        void viewportModel(pybind11::module_&);

        void modelsBind(pybind11::module_&);
    }
}
