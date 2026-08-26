// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/FileThumbnail.h>

#include <djv/Models/FilesModel.h>

#include <ftk/Core/Context.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void fileThumbnail(py::module_& m)
        {
            using namespace ui;

            py::class_<
                FileDragDropData,
                ftk::IDragDropData,
                std::shared_ptr<FileDragDropData> >(m, "FileDragDropData")
                .def(
                    py::init<const std::shared_ptr<models::FilesModelItem>&>(),
                    py::arg("item"))
                .def_property_readonly("item", &FileDragDropData::getItem);

            py::class_<FileThumbnail, ftk::IWidget, std::shared_ptr<FileThumbnail> >(m, "FileThumbnail")
                .def(
                    py::init(&FileThumbnail::create),
                    py::arg("context"),
                    py::arg("item"),
                    py::arg("ioOptions"),
                    py::arg("parent") = nullptr);
        }
    }
}
