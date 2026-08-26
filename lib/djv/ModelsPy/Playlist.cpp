// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/Playlist.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void playlist(py::module_& m)
        {
            using namespace models;

            py::class_<Playlist>(m, "Playlist")
                .def(py::init<>())
                .def_readwrite("items", &Playlist::items)
                .def_readwrite("aIndex", &Playlist::aIndex)
                .def_readwrite("bIndexes", &Playlist::bIndexes)
                .def_readwrite("compareOptions", &Playlist::compareOptions)
                .def_readwrite("compareTime", &Playlist::compareTime);

            m.def(
                "playlistSave",
                [](const std::string& fileName,
                    const Playlist& playlist,
                    double defaultRate)
                {
                    playlistSave(fileName, playlist, defaultRate);
                },
                py::arg("fileName"),
                py::arg("playlist"),
                py::arg("defaultRate") = 24.0);

            // The report rides along as the second element of a tuple, the
            // way the settings getters return their values.
            m.def(
                "playlistOpen",
                [](const std::string& fileName)
                {
                    std::vector<std::string> report;
                    const Playlist playlist = playlistOpen(fileName, report);
                    return py::make_tuple(playlist, report);
                },
                py::arg("fileName"));
        }
    }
}
