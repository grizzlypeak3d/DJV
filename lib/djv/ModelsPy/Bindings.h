// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once


#include <nanobind/nanobind.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

namespace djv
{
    namespace python
    {
        void annotationsModel(nanobind::module_&);
        void appInfoModel(nanobind::module_&);
        void audioModel(nanobind::module_&);
        void colorModel(nanobind::module_&);
        void commandsModel(nanobind::module_&);
        void drawModel(nanobind::module_&);
        void filesModel(nanobind::module_&);
        void markersModel(nanobind::module_&);
        void ocioModel(nanobind::module_&);
        void parse(nanobind::module_&);
        void playlist(nanobind::module_&);
        void recentFilesModel(nanobind::module_&);
        void review(nanobind::module_&);
        void settingsModel(nanobind::module_&);
        void shortcuts(nanobind::module_&);
        void timeUnitsModel(nanobind::module_&);
        void toolsModel(nanobind::module_&);
        void viewportModel(nanobind::module_&);

        void modelsBind(nanobind::module_&);
    }
}
