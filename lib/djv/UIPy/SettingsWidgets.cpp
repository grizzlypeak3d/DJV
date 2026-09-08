// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UIPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/UI/SettingsWidgets.h>

#include <djv/Models/SettingsModel.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/Core/Context.h>

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
        namespace
        {
            // The settings widgets share one shape: created from the context
            // and the settings model, with nothing else to bind.
            template<typename T>
            void settingsWidget(nb::module_& m, const char* name)
            {
                nb::class_<T, ui::ISettingsWidget>(m, name)
                    .def(
                        nb::new_(&T::create),
                        nb::arg("context"),
                        nb::arg("settingsModel"),
                        nb::arg("parent") = nullptr);
            }
        }

        void settingsWidgets(nb::module_& m)
        {
            using namespace ui;

            nb::class_<ISettingsWidget, ftk::IContainer>(m, "ISettingsWidget");

            settingsWidget<AudioSettingsWidget>(m, "AudioSettingsWidget");
            settingsWidget<CacheSettingsWidget>(m, "CacheSettingsWidget");
            settingsWidget<FileBrowserSettingsWidget>(m, "FileBrowserSettingsWidget");
            settingsWidget<OTIOSettingsWidget>(m, "OTIOSettingsWidget");
            settingsWidget<MiscSettingsWidget>(m, "MiscSettingsWidget");
            settingsWidget<MouseSettingsWidget>(m, "MouseSettingsWidget");
            settingsWidget<PlaybackSettingsWidget>(m, "PlaybackSettingsWidget");
            settingsWidget<ShortcutsSettingsWidget>(m, "ShortcutsSettingsWidget");
            settingsWidget<StyleSettingsWidget>(m, "StyleSettingsWidget");

            nb::class_<ImageSeqSettingsWidget, ISettingsWidget>(m, "ImageSeqSettingsWidget")
                .def(
                    nb::new_(&ImageSeqSettingsWidget::create),
                    nb::arg("context"),
                    nb::arg("settingsModel"),
                    nb::arg("viewportModel"),
                    nb::arg("parent") = nullptr);

            nb::class_<TimeSettingsWidget, ISettingsWidget>(m, "TimeSettingsWidget")
                .def(
                    nb::new_(&TimeSettingsWidget::create),
                    nb::arg("context"),
                    nb::arg("timeUnitsModel"),
                    nb::arg("parent") = nullptr);

#if defined(TLRENDER_FFMPEG_PLUGIN)
            settingsWidget<FFmpegSettingsWidget>(m, "FFmpegSettingsWidget");
#endif // TLRENDER_FFMPEG_PLUGIN
        }
    }
}
