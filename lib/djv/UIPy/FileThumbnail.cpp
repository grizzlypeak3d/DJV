// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/UI/FileThumbnail.h>

#include <djv/Models/FilesModel.h>

#include <ftk/Core/Context.h>

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
        void fileThumbnail(nb::module_& m)
        {
            using namespace ui;

            nb::class_<
                FileDragDropData,
                ftk::IDragDropData>(m, "FileDragDropData")
                .def(
                    nb::init<const std::shared_ptr<models::FilesModelItem>&>(),
                    nb::arg("item"))
                .def_prop_ro("item", &FileDragDropData::getItem);

            nb::class_<FileThumbnail, ftk::IWidget>(
                m, "FileThumbnail",
                // The Python examples hold this through weakref, which
                // nanobind classes opt into.
                nb::is_weak_referenceable())
                .def(
                    nb::new_(&FileThumbnail::create),
                    nb::arg("context"),
                    nb::arg("item"),
                    nb::arg("ioOptions"),
                    nb::arg("parent") = nullptr)
                .def_prop_ro("thumbnail", &FileThumbnail::getThumbnail);
        }
    }
}
