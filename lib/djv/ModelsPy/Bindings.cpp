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

            appInfoModel(mModels);
            audioModel(mModels);
            colorModel(mModels);
            commandsModel(mModels);
            filesModel(mModels);
            ocioModel(mModels);
            playlist(mModels);
            recentFilesModel(mModels);
            settingsModel(mModels);
            shortcuts(mModels);
            timeUnitsModel(mModels);
            toolsModel(mModels);
            viewportModel(mModels);
        }
    }
}
