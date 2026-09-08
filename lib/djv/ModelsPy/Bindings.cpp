// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

namespace djv
{
    namespace python
    {
        void modelsBind(nb::module_& m)
        {
            auto mModels = m.def_submodule("models", "Data models");

            annotationsModel(mModels);
            appInfoModel(mModels);
            audioModel(mModels);
            colorModel(mModels);
            commandsModel(mModels);
            drawModel(mModels);
            filesModel(mModels);
            markersModel(mModels);
            ocioModel(mModels);
            parse(mModels);
            playlist(mModels);
            recentFilesModel(mModels);
            review(mModels);
            settingsModel(mModels);
            shortcuts(mModels);
            timeUnitsModel(mModels);
            toolsModel(mModels);
            viewportModel(mModels);
        }
    }
}
