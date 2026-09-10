// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/SettingsModel.h>

#include <ftk/CorePy/Bindings.h>
#include <ftk/Core/Context.h>
#include <ftk/UI/Settings.h>

#include <nanobind/stl/function.h>
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
        void settingsModel(nb::module_& m)
        {
            using namespace models;

            nb::class_<AudioSettings>(m, "AudioSettings")
                .def(nb::init())
                .def_rw("bufferFrameCount", &AudioSettings::bufferFrameCount)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            FTK_ENUM_PY(m, ExportRenderSize);
            FTK_ENUM_BIND(m, ExportRenderSize);

            m.def("getWidth", &getWidth, nb::arg("renderSize"));

            FTK_ENUM_PY(m, ExportFileType);
            FTK_ENUM_BIND(m, ExportFileType);

            nb::class_<ExportSettings>(m, "ExportSettings")
                .def(nb::init())
                .def_rw("dir", &ExportSettings::dir)
                .def_rw("renderSize", &ExportSettings::renderSize)
                .def_rw("customWidth", &ExportSettings::customWidth)
                .def_rw("fileType", &ExportSettings::fileType)
                .def_rw("imageBase", &ExportSettings::imageBase)
                .def_rw("imageZeroPad", &ExportSettings::imageZeroPad)
                .def_rw("imageExt", &ExportSettings::imageExt)
                .def_rw("seqBase", &ExportSettings::seqBase)
                .def_rw("seqZeroPad", &ExportSettings::seqZeroPad)
                .def_rw("seqExt", &ExportSettings::seqExt)
                .def_rw("movieBase", &ExportSettings::movieBase)
                .def_rw("movieExt", &ExportSettings::movieExt)
                .def_rw("moviePreset", &ExportSettings::moviePreset)
                .def_rw("movieAudioCodec", &ExportSettings::movieAudioCodec)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            nb::class_<FileBrowserSettings>(m, "FileBrowserSettings")
                .def(nb::init())
                .def_rw("nativeFileDialog", &FileBrowserSettings::nativeFileDialog)
                .def_rw("path", &FileBrowserSettings::path)
                .def_rw("options", &FileBrowserSettings::options)
                .def_rw("ext", &FileBrowserSettings::ext)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            nb::class_<ImageSeqSettings>(m, "ImageSeqSettings")
                .def(nb::init())
                .def_rw("audio", &ImageSeqSettings::audio)
                .def_rw("audioExts", &ImageSeqSettings::audioExts)
                .def_rw("audioFileName", &ImageSeqSettings::audioFileName)
                .def_rw("maxDigits", &ImageSeqSettings::maxDigits)
                .def_rw("readThreadCount", &ImageSeqSettings::readThreadCount)
                .def_rw("io", &ImageSeqSettings::io)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            // The "spatial" field is omitted: tl::Spatial is not bound.
            nb::class_<OTIOSettings>(m, "OTIOSettings")
                .def(nb::init())
                .def_rw("compat", &OTIOSettings::compat)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            nb::class_<MiscSettings>(m, "MiscSettings")
                .def(nb::init())
                .def_rw("tooltipsEnabled", &MiscSettings::tooltipsEnabled)
                .def_rw("showSetup", &MiscSettings::showSetup)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            FTK_ENUM_PY(m, MouseAction);
            FTK_ENUM_BIND(m, MouseAction);

            nb::class_<MouseActionBinding>(m, "MouseActionBinding")
                .def(nb::init())
                .def(
                    nb::init<ftk::MouseButton, ftk::KeyModifier>(),
                    nb::arg("button"),
                    nb::arg("modifier") = ftk::KeyModifier::None)
                .def_rw("button", &MouseActionBinding::button)
                .def_rw("modifier", &MouseActionBinding::modifier)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            nb::class_<MouseSettings>(m, "MouseSettings")
                .def(nb::init())
                .def_rw("bindings", &MouseSettings::bindings)
                .def_rw("wheelScale", &MouseSettings::wheelScale)
                .def_rw("frameShuttleScale", &MouseSettings::frameShuttleScale)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            nb::class_<PlaybackSettings>(m, "PlaybackSettings")
                .def(nb::init())
                .def_rw("startPlayback", &PlaybackSettings::startPlayback)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            nb::class_<ShortcutsSettings>(m, "ShortcutsSettings")
                .def(nb::init())
                .def_rw("shortcuts", &ShortcutsSettings::shortcuts)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            // The "colorControls" and "colorStyle" fields are omitted:
            // ftk::ColorControls and ftk::ColorStyle are not bound.
            nb::class_<StyleSettings>(m, "StyleSettings")
                .def(nb::init())
                .def_rw("displayScale", &StyleSettings::displayScale)
                .def_rw("colorControls", &StyleSettings::colorControls)
                .def_rw("colorStyle", &StyleSettings::colorStyle)
                .def_rw("customColorRoles", &StyleSettings::customColorRoles)
                .def_rw("fonts", &StyleSettings::fonts)
                .def_rw("fontFiles", &StyleSettings::fontFiles)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            FTK_ENUM_PY(m, TimelineThumbnailSize);
            FTK_ENUM_BIND(m, TimelineThumbnailSize);

            m.def("getTimelineThumbnailSize", &getTimelineThumbnailSize, nb::arg("thumbnailSize"));
            m.def("getTimelineWaveformSize", &getTimelineWaveformSize, nb::arg("thumbnailSize"));

            nb::class_<TimelineSettings>(m, "TimelineSettings")
                .def(nb::init())
                .def_rw("minimize", &TimelineSettings::minimize)
                .def_rw("frameView", &TimelineSettings::frameView)
                .def_rw("scrollBars", &TimelineSettings::scrollBars)
                .def_rw("autoScroll", &TimelineSettings::autoScroll)
                .def_rw("stopOnScrub", &TimelineSettings::stopOnScrub)
                .def_rw("preview", &TimelineSettings::preview)
                .def_rw("trackMedia", &TimelineSettings::trackMedia)
                .def_rw("thumbnails", &TimelineSettings::thumbnails)
                .def_rw("thumbnailSize", &TimelineSettings::thumbnailSize)
                .def_rw("waveforms", &TimelineSettings::waveforms)
                .def_rw("waveformSize", &TimelineSettings::waveformSize)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

            nb::class_<WindowSettings>(m, "WindowSettings")
                .def(nb::init())
                .def_rw("size", &WindowSettings::size)
                .def_rw("fileToolBar", &WindowSettings::fileToolBar)
                .def_rw("compareToolBar", &WindowSettings::compareToolBar)
                .def_rw("windowToolBar", &WindowSettings::windowToolBar)
                .def_rw("viewToolBar", &WindowSettings::viewToolBar)
                .def_rw("toolsToolBar", &WindowSettings::toolsToolBar)
                .def_rw("tabBar", &WindowSettings::tabBar)
                .def_rw("timeline", &WindowSettings::timeline)
                .def_rw("bottomToolBar", &WindowSettings::bottomToolBar)
                .def_rw("statusToolBar", &WindowSettings::statusToolBar)
                .def_rw("tools", &WindowSettings::tools)
                .def_rw("splitter", &WindowSettings::splitter)
                .def_rw("splitter2", &WindowSettings::splitter2)
                .def(nanobind::self == nanobind::self)
                .def(nanobind::self != nanobind::self);

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

            nb::class_<SettingsModel>(m, "SettingsModel")
                .def(
                    nb::new_(&SettingsModel::create),
                    nb::arg("context"),
                    nb::arg("settings"))

                .def("save", &SettingsModel::save)
                .def("reset", &SettingsModel::reset)

                .def_prop_rw("audio", &SettingsModel::getAudio, &SettingsModel::setAudio, nb::rv_policy::copy)
                .def_prop_ro("observeAudio", &SettingsModel::observeAudio)

                .def_prop_rw("cache", &SettingsModel::getCache, &SettingsModel::setCache, nb::rv_policy::copy)
                .def_prop_ro("observeCache", &SettingsModel::observeCache)

                .def_prop_rw("export", &SettingsModel::getExport, &SettingsModel::setExport, nb::rv_policy::copy)
                .def_prop_ro("observeExport", &SettingsModel::observeExport)

                .def_prop_rw("fileBrowser", &SettingsModel::getFileBrowser, &SettingsModel::setFileBrowser, nb::rv_policy::copy)
                .def_prop_ro("observeFileBrowser", &SettingsModel::observeFileBrowser)

                .def_prop_rw("imageSeq", &SettingsModel::getImageSeq, &SettingsModel::setImageSeq, nb::rv_policy::copy)
                .def_prop_ro("observeImageSeq", &SettingsModel::observeImageSeq)

                .def_prop_rw("otio", &SettingsModel::getOTIO, &SettingsModel::setOTIO, nb::rv_policy::copy)
                .def_prop_ro("observeOTIO", &SettingsModel::observeOTIO)

                .def_prop_rw("misc", &SettingsModel::getMisc, &SettingsModel::setMisc, nb::rv_policy::copy)
                .def_prop_ro("observeMisc", &SettingsModel::observeMisc)

                .def_prop_rw("mouse", &SettingsModel::getMouse, &SettingsModel::setMouse, nb::rv_policy::copy)
                .def_prop_ro("observeMouse", &SettingsModel::observeMouse)

                .def_prop_rw("playback", &SettingsModel::getPlayback, &SettingsModel::setPlayback, nb::rv_policy::copy)
                .def_prop_ro("observePlayback", &SettingsModel::observePlayback)

                .def_prop_rw("shortcuts", &SettingsModel::getShortcuts, &SettingsModel::setShortcuts, nb::rv_policy::copy)
                .def_prop_ro("observeShortcuts", &SettingsModel::observeShortcuts)
                .def("addShortcuts", &SettingsModel::addShortcuts, nb::arg("shortcuts"))

                .def_prop_rw("style", &SettingsModel::getStyle, &SettingsModel::setStyle, nb::rv_policy::copy)
                .def_prop_ro("observeStyle", &SettingsModel::observeStyle)

                .def_prop_rw("timeline", &SettingsModel::getTimeline, &SettingsModel::setTimeline, nb::rv_policy::copy)
                .def_prop_ro("observeTimeline", &SettingsModel::observeTimeline)

                .def_prop_rw("window", &SettingsModel::getWindow, &SettingsModel::setWindow, nb::rv_policy::copy)
                .def_prop_ro("observeWindow", &SettingsModel::observeWindow)

                .def_prop_ro("ioOptions", &SettingsModel::getIOOptions);
        }
    }
}
