// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/Review.h>

#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void review(py::module_& m)
        {
            using namespace models;

            m.attr("reviewVersion") = reviewVersion;
            m.def("reviewVersionSupported", &reviewVersionSupported, py::arg("version"));
            m.def("reviewExtension", &reviewExtension);
            m.def("sameTime", &sameTime);
            m.def("sameRange", &sameRange);
            m.def("generateId", &generateId);
            m.def("timestamp", &timestamp);
            m.def("reviewAuthor", &reviewAuthor);

            py::class_<ReviewFile>(m, "ReviewFile")
                .def(py::init())
                .def_readwrite("id", &ReviewFile::id)
                .def_readwrite("path", &ReviewFile::path)
                .def_readwrite("pathAbsolute", &ReviewFile::pathAbsolute)
                .def_readwrite("audioPath", &ReviewFile::audioPath)
                .def_readwrite("audioPathAbsolute", &ReviewFile::audioPathAbsolute)
                .def_readwrite("videoLayer", &ReviewFile::videoLayer)
                .def_readwrite("speed", &ReviewFile::speed)
                .def_readwrite("currentTime", &ReviewFile::currentTime)
                .def_readwrite("inOutRange", &ReviewFile::inOutRange);

            py::class_<ReviewCompare>(m, "ReviewCompare")
                .def(py::init())
                .def_readwrite("aId", &ReviewCompare::aId)
                .def_readwrite("bIds", &ReviewCompare::bIds)
                .def_readwrite("options", &ReviewCompare::options)
                .def_readwrite("time", &ReviewCompare::time);

            py::class_<ReviewView>(m, "ReviewView")
                .def(py::init())
                .def_readwrite("frameView", &ReviewView::frameView)
                .def_readwrite("pos", &ReviewView::pos)
                .def_readwrite("zoom", &ReviewView::zoom);

            py::class_<ReviewColor>(m, "ReviewColor")
                .def(py::init())
                .def_readwrite("ocio", &ReviewColor::ocio)
                .def_readwrite("lut", &ReviewColor::lut)
                .def_readwrite("display", &ReviewColor::display)
                .def_readwrite("background", &ReviewColor::background)
                .def_readwrite("foreground", &ReviewColor::foreground)
                .def_readwrite("aspectRatio", &ReviewColor::aspectRatio)
                .def_readwrite("hud", &ReviewColor::hud);

            py::class_<ReviewUI>(m, "ReviewUI")
                .def(py::init())
                .def_readwrite("openTools", &ReviewUI::openTools);

            py::class_<Review>(m, "Review")
                .def(py::init())
                .def_readwrite("version", &Review::version)
                .def_readwrite("app", &Review::app)
                .def_readwrite("created", &Review::created)
                .def_readwrite("files", &Review::files)
                .def_readwrite("compare", &Review::compare)
                .def_readwrite("view", &Review::view)
                .def_readwrite("color", &Review::color)
                .def_readwrite("ui", &Review::ui)
                .def_readwrite("annotations", &Review::annotations)
                .def_readwrite("markers", &Review::markers)
                .def_readonly("unreadSections", &Review::unreadSections)
                // The raw document and the unread items are nlohmann JSON,
                // which has no Python form; what Python needs is to carry
                // them from the review it opened into the one it saves.
                .def(
                    "carryUnread",
                    [](Review& self, const Review& other)
                    {
                        self.raw = other.raw;
                        self.unreadSections = other.unreadSections;
                        self.unreadItems = other.unreadItems;
                    },
                    py::arg("other"));

            m.def("reviewOpen", &reviewOpen, py::arg("fileName"));
            m.def("reviewSave", &reviewSave, py::arg("fileName"), py::arg("review"));
            m.def(
                "reviewRelativePath",
                &reviewRelativePath,
                py::arg("path"),
                py::arg("base"));
            m.def("reviewGenericPath", &reviewGenericPath, py::arg("path"));
            m.def(
                "resolveReviewPath",
                [](const std::string& relative,
                    const std::string& absolute,
                    const std::filesystem::path& base,
                    const std::filesystem::path& substituteRoot,
                    const ftk::PathOptions& pathOptions)
                {
                    bool exists = false;
                    const std::filesystem::path out = models::resolveReviewPath(
                        relative, absolute, base, substituteRoot, pathOptions, exists);
                    return std::make_pair(out, exists);
                },
                py::arg("relative"),
                py::arg("absolute"),
                py::arg("base"),
                py::arg("substituteRoot") = std::filesystem::path(),
                py::arg("pathOptions") = ftk::PathOptions());
        }
    }
}
