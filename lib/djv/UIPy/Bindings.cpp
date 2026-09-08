// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

namespace djv
{
    namespace python
    {
        void uiBind(nb::module_& m)
        {
            auto mUI = m.def_submodule("ui", "User interface widgets");

            audioPopup(mUI);
            uiInit(mUI);
            colorWidgets(mUI);
            viewWidgets(mUI);
            viewport(mUI);
            settingsWidgets(mUI);
            fileThumbnail(mUI);
            frameRangePopup(mUI);
            infoWidget(mUI);
            separateAudioDialog(mUI);
            speedPopup(mUI);
            statusIndicator(mUI);
            sysInfoDialog(mUI);
            magnifyWidget(mUI);
            exportWidget(mUI);
        }
    }
}
