// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/Playlist.h>

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
        void playlist(nb::module_& m)
        {
            using namespace models;

            nb::class_<Playlist>(m, "Playlist")
                .def(nb::init<>())
                .def_rw("items", &Playlist::items)
                .def_rw("aIndex", &Playlist::aIndex)
                .def_rw("bIndexes", &Playlist::bIndexes)
                .def_rw("compareOptions", &Playlist::compareOptions)
                .def_rw("compareTime", &Playlist::compareTime);

            m.def(
                "playlistSave",
                [](const std::string& fileName,
                    const Playlist& playlist,
                    double defaultRate)
                {
                    playlistSave(fileName, playlist, defaultRate);
                },
                nb::arg("fileName"),
                nb::arg("playlist"),
                nb::arg("defaultRate") = 24.0);

            // The report rides along as the second element of a tuple, the
            // way the settings getters return their values.
            m.def(
                "playlistOpen",
                [](const std::string& fileName)
                {
                    std::vector<std::string> report;
                    const Playlist playlist = playlistOpen(fileName, report);
                    return nb::make_tuple(playlist, report);
                },
                nb::arg("fileName"));
        }
    }
}
