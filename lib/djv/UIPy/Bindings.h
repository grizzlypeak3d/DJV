// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once


#include <pybind11/pybind11.h>

namespace djv
{
    namespace python
    {
        void colorWidgets(pybind11::module_&);
        void viewWidgets(pybind11::module_&);
        void viewport(pybind11::module_&);

        void uiBind(pybind11::module_&);
    }
}
