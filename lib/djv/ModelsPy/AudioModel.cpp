// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/AudioModel.h>

#include <ftk/CorePy/Bindings.h>
#include <ftk/UI/Settings.h>
#include <ftk/Core/Context.h>

#include <pybind11/functional.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void audioModel(py::module_& m)
        {
            using namespace models;

            ftk::python::observable<tl::AudioDeviceID>(m, "AudioDeviceID");
            ftk::python::observableList<tl::AudioDeviceID>(m, "AudioDeviceID");

            py::class_<AudioModel, std::shared_ptr<AudioModel> >(m, "AudioModel")
                .def(
                    py::init(&AudioModel::create),
                    py::arg("context"),
                    py::arg("settings"))
                .def_property_readonly("devices", &AudioModel::getDevices, py::return_value_policy::copy)
                .def_property_readonly("observeDevices", &AudioModel::observeDevices)
                .def_property("device", &AudioModel::getDevice, &AudioModel::setDevice, py::return_value_policy::copy)
                .def_property_readonly("observeDevice", &AudioModel::observeDevice)
                .def_property("volume", &AudioModel::getVolume, &AudioModel::setVolume)
                .def_property_readonly("observeVolume", &AudioModel::observeVolume)
                .def("volumeUp", &AudioModel::volumeUp)
                .def("volumeDown", &AudioModel::volumeDown)
                .def_property("mute", &AudioModel::isMuted, &AudioModel::setMute)
                .def_property_readonly("observeMute", &AudioModel::observeMute)
                .def_property("channelMute", &AudioModel::getChannelMute, &AudioModel::setChannelMute)
                .def_property_readonly("observeChannelMute", &AudioModel::observeChannelMute)
                .def_property("syncOffset", &AudioModel::getSyncOffset, &AudioModel::setSyncOffset)
                .def_property_readonly("observeSyncOffset", &AudioModel::observeSyncOffset);
        }
    }
}
