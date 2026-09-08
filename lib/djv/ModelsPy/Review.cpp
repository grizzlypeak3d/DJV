// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/Review.h>

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
        void review(nb::module_& m)
        {
            using namespace models;

            m.attr("reviewVersion") = reviewVersion;
            m.def("reviewVersionSupported", &reviewVersionSupported, nb::arg("version"));
            m.def("reviewExtension", &reviewExtension);
            m.def("sameTime", &sameTime);
            m.def("sameRange", &sameRange);
            m.def("generateId", &generateId);
            m.def("timestamp", &timestamp);
            m.def("reviewAuthor", &reviewAuthor);

            nb::class_<ReviewFile>(m, "ReviewFile")
                .def(nb::init())
                .def_rw("id", &ReviewFile::id)
                .def_rw("path", &ReviewFile::path)
                .def_rw("pathAbsolute", &ReviewFile::pathAbsolute)
                .def_rw("audioPath", &ReviewFile::audioPath)
                .def_rw("audioPathAbsolute", &ReviewFile::audioPathAbsolute)
                .def_rw("videoLayer", &ReviewFile::videoLayer)
                .def_rw("speed", &ReviewFile::speed)
                .def_rw("currentTime", &ReviewFile::currentTime)
                .def_rw("inOutRange", &ReviewFile::inOutRange);

            nb::class_<ReviewCompare>(m, "ReviewCompare")
                .def(nb::init())
                .def_rw("aId", &ReviewCompare::aId)
                .def_rw("bIds", &ReviewCompare::bIds)
                .def_rw("options", &ReviewCompare::options)
                .def_rw("time", &ReviewCompare::time);

            nb::class_<ReviewView>(m, "ReviewView")
                .def(nb::init())
                .def_rw("frameView", &ReviewView::frameView)
                .def_rw("pos", &ReviewView::pos)
                .def_rw("zoom", &ReviewView::zoom);

            nb::class_<ReviewColor>(m, "ReviewColor")
                .def(nb::init())
                .def_rw("ocio", &ReviewColor::ocio)
                .def_rw("lut", &ReviewColor::lut)
                .def_rw("display", &ReviewColor::display)
                .def_rw("background", &ReviewColor::background)
                .def_rw("foreground", &ReviewColor::foreground)
                .def_rw("aspectRatio", &ReviewColor::aspectRatio)
                .def_rw("hud", &ReviewColor::hud);

            nb::class_<ReviewUI>(m, "ReviewUI")
                .def(nb::init())
                .def_rw("openTools", &ReviewUI::openTools);

            nb::class_<Review>(m, "Review")
                .def(nb::init())
                .def_rw("version", &Review::version)
                .def_rw("app", &Review::app)
                .def_rw("created", &Review::created)
                .def_rw("files", &Review::files)
                .def_rw("compare", &Review::compare)
                .def_rw("view", &Review::view)
                .def_rw("color", &Review::color)
                .def_rw("ui", &Review::ui)
                .def_rw("annotations", &Review::annotations)
                .def_rw("markers", &Review::markers)
                .def_ro("unreadSections", &Review::unreadSections)
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
                    nb::arg("other"));

            m.def("reviewOpen", &reviewOpen, nb::arg("fileName"));
            m.def("reviewSave", &reviewSave, nb::arg("fileName"), nb::arg("review"));
            m.def(
                "reviewRelativePath",
                &reviewRelativePath,
                nb::arg("path"),
                nb::arg("base"));
            m.def("reviewGenericPath", &reviewGenericPath, nb::arg("path"));
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
                nb::arg("relative"),
                nb::arg("absolute"),
                nb::arg("base"),
                nb::arg("substituteRoot") = std::filesystem::path(),
                nb::arg("pathOptions") = ftk::PathOptions());
        }
    }
}
