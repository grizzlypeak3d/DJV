// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/AudioModel.h>

#include <ftk/CorePy/Bindings.h>
#include <ftk/UI/Settings.h>
#include <ftk/Core/Context.h>

#include <nanobind/stl/function.h>
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
        void audioModel(nb::module_& m)
        {
            using namespace models;

            ftk::python::observable<tl::AudioDeviceID>(m, "AudioDeviceID");
            ftk::python::observableList<tl::AudioDeviceID>(m, "AudioDeviceID");

            nb::class_<AudioModel>(m, "AudioModel")
                .def(
                    nb::new_(&AudioModel::create),
                    nb::arg("context"),
                    nb::arg("settings"))
                .def_prop_ro("devices", &AudioModel::getDevices, nb::rv_policy::copy)
                .def_prop_ro("observeDevices", &AudioModel::observeDevices)
                .def_prop_rw("device", &AudioModel::getDevice, &AudioModel::setDevice, nb::rv_policy::copy)
                .def_prop_ro("observeDevice", &AudioModel::observeDevice)
                .def_prop_rw("volume", &AudioModel::getVolume, &AudioModel::setVolume)
                .def_prop_ro("observeVolume", &AudioModel::observeVolume)
                .def("save", &AudioModel::save)
                .def("volumeUp", &AudioModel::volumeUp)
                .def("volumeDown", &AudioModel::volumeDown)
                .def_prop_rw("mute", &AudioModel::isMuted, &AudioModel::setMute)
                .def_prop_ro("observeMute", &AudioModel::observeMute)
                .def_prop_rw("channelMute", &AudioModel::getChannelMute, &AudioModel::setChannelMute)
                .def_prop_ro("observeChannelMute", &AudioModel::observeChannelMute)
                .def_prop_rw("syncOffset", &AudioModel::getSyncOffset, &AudioModel::setSyncOffset)
                .def_prop_ro("observeSyncOffset", &AudioModel::observeSyncOffset);
        }
    }
}
