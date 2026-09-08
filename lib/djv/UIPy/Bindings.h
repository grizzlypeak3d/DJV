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
        void audioPopup(nanobind::module_&);
        void uiInit(nanobind::module_&);
        void colorWidgets(nanobind::module_&);
        void viewWidgets(nanobind::module_&);
        void viewport(nanobind::module_&);
        void settingsWidgets(nanobind::module_&);
        void fileThumbnail(nanobind::module_&);
        void frameRangePopup(nanobind::module_&);
        void infoWidget(nanobind::module_&);
        void separateAudioDialog(nanobind::module_&);
        void speedPopup(nanobind::module_&);
        void statusIndicator(nanobind::module_&);
        void sysInfoDialog(nanobind::module_&);
        void magnifyWidget(nanobind::module_&);
        void exportWidget(nanobind::module_&);

        void uiBind(nanobind::module_&);
    }
}
