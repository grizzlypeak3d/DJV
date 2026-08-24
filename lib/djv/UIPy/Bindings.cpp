// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void uiBind(py::module_& m)
        {
            auto mUI = m.def_submodule("ui", "User interface widgets");

            colorWidgets(mUI);
            viewWidgets(mUI);
            viewport(mUI);
            settingsWidgets(mUI);
            fileThumbnail(mUI);
            frameRangePopup(mUI);
        }
    }
}
