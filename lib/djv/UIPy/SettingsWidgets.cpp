// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <djv/UI/SettingsWidgets.h>

#include <djv/Models/SettingsModel.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/Core/Context.h>

#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        namespace
        {
            // The settings widgets share one shape: created from the context
            // and the settings model, with nothing else to bind.
            template<typename T>
            void settingsWidget(py::module_& m, const char* name)
            {
                py::class_<T, ui::ISettingsWidget, std::shared_ptr<T> >(m, name)
                    .def(
                        py::init(&T::create),
                        py::arg("context"),
                        py::arg("settingsModel"),
                        py::arg("parent") = nullptr);
            }
        }

        void settingsWidgets(py::module_& m)
        {
            using namespace ui;

            py::class_<ISettingsWidget, ftk::IContainer, std::shared_ptr<ISettingsWidget> >(m, "ISettingsWidget");

            settingsWidget<AudioSettingsWidget>(m, "AudioSettingsWidget");
            settingsWidget<CacheSettingsWidget>(m, "CacheSettingsWidget");
            settingsWidget<FileBrowserSettingsWidget>(m, "FileBrowserSettingsWidget");
            settingsWidget<OTIOSettingsWidget>(m, "OTIOSettingsWidget");
            settingsWidget<MiscSettingsWidget>(m, "MiscSettingsWidget");
            settingsWidget<MouseSettingsWidget>(m, "MouseSettingsWidget");
            settingsWidget<PlaybackSettingsWidget>(m, "PlaybackSettingsWidget");
            settingsWidget<ShortcutsSettingsWidget>(m, "ShortcutsSettingsWidget");
            settingsWidget<StyleSettingsWidget>(m, "StyleSettingsWidget");

            py::class_<ImageSeqSettingsWidget, ISettingsWidget, std::shared_ptr<ImageSeqSettingsWidget> >(m, "ImageSeqSettingsWidget")
                .def(
                    py::init(&ImageSeqSettingsWidget::create),
                    py::arg("context"),
                    py::arg("settingsModel"),
                    py::arg("viewportModel"),
                    py::arg("parent") = nullptr);

            py::class_<TimeSettingsWidget, ISettingsWidget, std::shared_ptr<TimeSettingsWidget> >(m, "TimeSettingsWidget")
                .def(
                    py::init(&TimeSettingsWidget::create),
                    py::arg("context"),
                    py::arg("timeUnitsModel"),
                    py::arg("parent") = nullptr);

#if defined(TLRENDER_FFMPEG_PLUGIN)
            settingsWidget<FFmpegSettingsWidget>(m, "FFmpegSettingsWidget");
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_FFMPEG_PLUGIN)
            settingsWidget<FFmpegCmdSettingsWidget>(m, "FFmpegCmdSettingsWidget");
#endif // TLRENDER_FFMPEG_PLUGIN
        }
    }
}
