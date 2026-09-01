// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void modelsBind(py::module_& m)
        {
            auto mModels = m.def_submodule("models", "Data models");

            annotationsModel(mModels);
            appInfoModel(mModels);
            audioModel(mModels);
            colorModel(mModels);
            commandsModel(mModels);
            drawModel(mModels);
            filesModel(mModels);
            notesModel(mModels);
            ocioModel(mModels);
            parse(mModels);
            playlist(mModels);
            rangesModel(mModels);
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
