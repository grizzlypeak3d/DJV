// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/Shortcuts.h>

#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/filesystem.h>

namespace nb = nanobind;

namespace djv
{
    namespace python
    {
        void shortcuts(nb::module_& m)
        {
            using namespace models;

            nb::class_<Shortcut>(m, "Shortcut")
                .def(nb::init())
                .def(
                    nb::init<
                        const std::string&,
                        const std::string&,
                        const ftk::KeyShortcut&,
                        const ftk::KeyShortcut&>(),
                    nb::arg("name"),
                    nb::arg("text"),
                    nb::arg("primary") = ftk::KeyShortcut(),
                    nb::arg("secondary") = ftk::KeyShortcut())
                .def_rw("name", &Shortcut::name)
                .def_rw("text", &Shortcut::text)
                .def_rw("primary", &Shortcut::primary)
                .def_rw("secondary", &Shortcut::secondary)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);
        }
    }
}
