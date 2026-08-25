// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/SettingsModel.h>

#include <ftk/CorePy/Bindings.h>
#include <ftk/Core/Context.h>
#include <ftk/UI/Settings.h>

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void settingsModel(py::module_& m)
        {
            using namespace models;

            py::class_<AudioSettings>(m, "AudioSettings")
                .def(py::init())
                .def_readwrite("bufferFrameCount", &AudioSettings::bufferFrameCount)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            py::enum_<ExportRenderSize>(m, "ExportRenderSize")
                .value("Default", ExportRenderSize::Default)
                .value("_1920", ExportRenderSize::_1920)
                .value("_3840", ExportRenderSize::_3840)
                .value("_4096", ExportRenderSize::_4096)
                .value("Custom", ExportRenderSize::Custom);
            FTK_ENUM_BIND(m, ExportRenderSize);

            m.def("getWidth", &getWidth, py::arg("renderSize"));

            py::enum_<ExportFileType>(m, "ExportFileType")
                .value("Image", ExportFileType::Image)
                .value("Seq", ExportFileType::Seq)
                .value("Movie", ExportFileType::Movie);
            FTK_ENUM_BIND(m, ExportFileType);

            py::class_<ExportSettings>(m, "ExportSettings")
                .def(py::init())
                .def_readwrite("dir", &ExportSettings::dir)
                .def_readwrite("renderSize", &ExportSettings::renderSize)
                .def_readwrite("customWidth", &ExportSettings::customWidth)
                .def_readwrite("fileType", &ExportSettings::fileType)
                .def_readwrite("imageBase", &ExportSettings::imageBase)
                .def_readwrite("imageZeroPad", &ExportSettings::imageZeroPad)
                .def_readwrite("imageExt", &ExportSettings::imageExt)
                .def_readwrite("seqBase", &ExportSettings::seqBase)
                .def_readwrite("seqZeroPad", &ExportSettings::seqZeroPad)
                .def_readwrite("seqExt", &ExportSettings::seqExt)
                .def_readwrite("movieBase", &ExportSettings::movieBase)
                .def_readwrite("movieExt", &ExportSettings::movieExt)
                .def_readwrite("movieCodec", &ExportSettings::movieCodec)
                .def_readwrite("movieAudioCodec", &ExportSettings::movieAudioCodec)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            py::class_<FileBrowserSettings>(m, "FileBrowserSettings")
                .def(py::init())
                .def_readwrite("nativeFileDialog", &FileBrowserSettings::nativeFileDialog)
                .def_readwrite("path", &FileBrowserSettings::path)
                .def_readwrite("options", &FileBrowserSettings::options)
                .def_readwrite("ext", &FileBrowserSettings::ext)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            py::class_<ImageSeqSettings>(m, "ImageSeqSettings")
                .def(py::init())
                .def_readwrite("audio", &ImageSeqSettings::audio)
                .def_readwrite("audioExts", &ImageSeqSettings::audioExts)
                .def_readwrite("audioFileName", &ImageSeqSettings::audioFileName)
                .def_readwrite("maxDigits", &ImageSeqSettings::maxDigits)
                .def_readwrite("readThreadCount", &ImageSeqSettings::readThreadCount)
                .def_readwrite("io", &ImageSeqSettings::io)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            // The "spatial" field is omitted: tl::Spatial is not bound.
            py::class_<OTIOSettings>(m, "OTIOSettings")
                .def(py::init())
                .def_readwrite("compat", &OTIOSettings::compat)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            py::class_<MiscSettings>(m, "MiscSettings")
                .def(py::init())
                .def_readwrite("tooltipsEnabled", &MiscSettings::tooltipsEnabled)
                .def_readwrite("showSetup", &MiscSettings::showSetup)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            py::enum_<MouseAction>(m, "MouseAction")
                .value("PanView", MouseAction::PanView)
                .value("CompareWipe", MouseAction::CompareWipe)
                .value("Pick", MouseAction::Pick)
                .value("FrameShuttle", MouseAction::FrameShuttle);
            FTK_ENUM_BIND(m, MouseAction);

            py::class_<MouseActionBinding>(m, "MouseActionBinding")
                .def(py::init())
                .def(
                    py::init<ftk::MouseButton, ftk::KeyModifier>(),
                    py::arg("button"),
                    py::arg("modifier") = ftk::KeyModifier::None)
                .def_readwrite("button", &MouseActionBinding::button)
                .def_readwrite("modifier", &MouseActionBinding::modifier)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            py::class_<MouseSettings>(m, "MouseSettings")
                .def(py::init())
                .def_readwrite("bindings", &MouseSettings::bindings)
                .def_readwrite("wheelScale", &MouseSettings::wheelScale)
                .def_readwrite("frameShuttleScale", &MouseSettings::frameShuttleScale)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            py::class_<PlaybackSettings>(m, "PlaybackSettings")
                .def(py::init())
                .def_readwrite("startPlayback", &PlaybackSettings::startPlayback)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            py::class_<ShortcutsSettings>(m, "ShortcutsSettings")
                .def(py::init())
                .def_readwrite("shortcuts", &ShortcutsSettings::shortcuts)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            // The "colorControls" and "colorStyle" fields are omitted:
            // ftk::ColorControls and ftk::ColorStyle are not bound.
            py::class_<StyleSettings>(m, "StyleSettings")
                .def(py::init())
                .def_readwrite("displayScale", &StyleSettings::displayScale)
                .def_readwrite("colorControls", &StyleSettings::colorControls)
                .def_readwrite("colorStyle", &StyleSettings::colorStyle)
                .def_readwrite("customColorRoles", &StyleSettings::customColorRoles)
                .def_readwrite("fonts", &StyleSettings::fonts)
                .def_readwrite("fontFiles", &StyleSettings::fontFiles)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            py::enum_<TimelineThumbnailSize>(m, "TimelineThumbnailSize")
                .value("Small", TimelineThumbnailSize::Small)
                .value("Medium", TimelineThumbnailSize::Medium)
                .value("Large", TimelineThumbnailSize::Large);
            FTK_ENUM_BIND(m, TimelineThumbnailSize);

            m.def("getTimelineThumbnailSize", &getTimelineThumbnailSize, py::arg("thumbnailSize"));
            m.def("getTimelineWaveformSize", &getTimelineWaveformSize, py::arg("thumbnailSize"));

            py::class_<TimelineSettings>(m, "TimelineSettings")
                .def(py::init())
                .def_readwrite("minimize", &TimelineSettings::minimize)
                .def_readwrite("frameView", &TimelineSettings::frameView)
                .def_readwrite("scrollBars", &TimelineSettings::scrollBars)
                .def_readwrite("autoScroll", &TimelineSettings::autoScroll)
                .def_readwrite("stopOnScrub", &TimelineSettings::stopOnScrub)
                .def_readwrite("trackMedia", &TimelineSettings::trackMedia)
                .def_readwrite("thumbnails", &TimelineSettings::thumbnails)
                .def_readwrite("thumbnailSize", &TimelineSettings::thumbnailSize)
                .def_readwrite("waveforms", &TimelineSettings::waveforms)
                .def_readwrite("waveformSize", &TimelineSettings::waveformSize)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            py::class_<WindowSettings>(m, "WindowSettings")
                .def(py::init())
                .def_readwrite("size", &WindowSettings::size)
                .def_readwrite("fileToolBar", &WindowSettings::fileToolBar)
                .def_readwrite("compareToolBar", &WindowSettings::compareToolBar)
                .def_readwrite("windowToolBar", &WindowSettings::windowToolBar)
                .def_readwrite("viewToolBar", &WindowSettings::viewToolBar)
                .def_readwrite("toolsToolBar", &WindowSettings::toolsToolBar)
                .def_readwrite("tabBar", &WindowSettings::tabBar)
                .def_readwrite("timeline", &WindowSettings::timeline)
                .def_readwrite("bottomToolBar", &WindowSettings::bottomToolBar)
                .def_readwrite("statusToolBar", &WindowSettings::statusToolBar)
                .def_readwrite("tools", &WindowSettings::tools)
                .def_readwrite("splitter", &WindowSettings::splitter)
                .def_readwrite("splitter2", &WindowSettings::splitter2)
                .def(pybind11::self == pybind11::self)
                .def(pybind11::self != pybind11::self);

            ftk::python::observable<AudioSettings>(m, "AudioSettings");
            ftk::python::observable<ExportSettings>(m, "ExportSettings");
            ftk::python::observable<FileBrowserSettings>(m, "FileBrowserSettings");
            ftk::python::observable<ImageSeqSettings>(m, "ImageSeqSettings");
            ftk::python::observable<OTIOSettings>(m, "OTIOSettings");
            ftk::python::observable<MiscSettings>(m, "MiscSettings");
            ftk::python::observable<MouseSettings>(m, "MouseSettings");
            ftk::python::observable<PlaybackSettings>(m, "PlaybackSettings");
            ftk::python::observable<ShortcutsSettings>(m, "ShortcutsSettings");
            ftk::python::observable<StyleSettings>(m, "StyleSettings");
            ftk::python::observable<TimelineSettings>(m, "TimelineSettings");
            ftk::python::observable<WindowSettings>(m, "WindowSettings");

            py::class_<SettingsModel, std::shared_ptr<SettingsModel> >(m, "SettingsModel")
                .def(
                    py::init(&SettingsModel::create),
                    py::arg("context"),
                    py::arg("settings"),
                    py::arg("displayScaleDefault"))

                .def("save", &SettingsModel::save)
                .def("reset", &SettingsModel::reset)

                .def_property("audio", &SettingsModel::getAudio, &SettingsModel::setAudio, py::return_value_policy::copy)
                .def_property_readonly("observeAudio", &SettingsModel::observeAudio)

                .def_property("cache", &SettingsModel::getCache, &SettingsModel::setCache, py::return_value_policy::copy)
                .def_property_readonly("observeCache", &SettingsModel::observeCache)

                .def_property("export", &SettingsModel::getExport, &SettingsModel::setExport, py::return_value_policy::copy)
                .def_property_readonly("observeExport", &SettingsModel::observeExport)

                .def_property("fileBrowser", &SettingsModel::getFileBrowser, &SettingsModel::setFileBrowser, py::return_value_policy::copy)
                .def_property_readonly("observeFileBrowser", &SettingsModel::observeFileBrowser)

                .def_property("imageSeq", &SettingsModel::getImageSeq, &SettingsModel::setImageSeq, py::return_value_policy::copy)
                .def_property_readonly("observeImageSeq", &SettingsModel::observeImageSeq)

                .def_property("otio", &SettingsModel::getOTIO, &SettingsModel::setOTIO, py::return_value_policy::copy)
                .def_property_readonly("observeOTIO", &SettingsModel::observeOTIO)

                .def_property("misc", &SettingsModel::getMisc, &SettingsModel::setMisc, py::return_value_policy::copy)
                .def_property_readonly("observeMisc", &SettingsModel::observeMisc)

                .def_property("mouse", &SettingsModel::getMouse, &SettingsModel::setMouse, py::return_value_policy::copy)
                .def_property_readonly("observeMouse", &SettingsModel::observeMouse)

                .def_property("playback", &SettingsModel::getPlayback, &SettingsModel::setPlayback, py::return_value_policy::copy)
                .def_property_readonly("observePlayback", &SettingsModel::observePlayback)

                .def_property("shortcuts", &SettingsModel::getShortcuts, &SettingsModel::setShortcuts, py::return_value_policy::copy)
                .def_property_readonly("observeShortcuts", &SettingsModel::observeShortcuts)
                .def("addShortcuts", &SettingsModel::addShortcuts, py::arg("shortcuts"))

                .def_property("style", &SettingsModel::getStyle, &SettingsModel::setStyle, py::return_value_policy::copy)
                .def_property_readonly("observeStyle", &SettingsModel::observeStyle)

                .def_property("timeline", &SettingsModel::getTimeline, &SettingsModel::setTimeline, py::return_value_policy::copy)
                .def_property_readonly("observeTimeline", &SettingsModel::observeTimeline)

                .def_property("window", &SettingsModel::getWindow, &SettingsModel::setWindow, py::return_value_policy::copy)
                .def_property_readonly("observeWindow", &SettingsModel::observeWindow)

                .def_property_readonly("ioOptions", &SettingsModel::getIOOptions);
        }
    }
}
