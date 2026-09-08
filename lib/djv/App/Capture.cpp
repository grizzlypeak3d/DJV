// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/Capture.h>

#include <djv/App/App.h>
#include <djv/App/IToolWidget.h>
#include <djv/App/MainWindow.h>
#include <djv/UI/Viewport.h>
#include <djv/Models/CommandsModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/MarkersModel.h>
#include <djv/Models/ToolsModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/UI/FileBrowser.h>
#include <ftk/UI/Settings.h>
#include <ftk/Core/Context.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Path.h>

#include <tlRender/Timeline/CompareOptions.h>
#include <tlRender/Timeline/Player.h>

#include <algorithm>
#include <optional>

namespace djv
{
    namespace app
    {
        struct Capture::Private
        {
            std::weak_ptr<App> app;
        };

        void Capture::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::filesystem::path& manifest,
            const std::string& shotId,
            const std::filesystem::path& outputDir)
        {
            ftk::Capture::_init(context, app, manifest, shotId, outputDir);
            _p->app = app;
        }

        Capture::Capture() :
            _p(new Private)
        {}

        Capture::~Capture()
        {}

        std::shared_ptr<Capture> Capture::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::filesystem::path& manifest,
            const std::string& shotId,
            const std::filesystem::path& outputDir)
        {
            auto out = std::shared_ptr<Capture>(new Capture);
            out->_init(context, app, manifest, shotId, outputDir);
            return out;
        }

        void Capture::_setupWindow(const nlohmann::json& w)
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return;
            if (w.contains("splitter") || w.contains("splitter2"))
            {
                // The splitter positions (0-1) live in the window settings
                // but are only applied to the widgets at construction, so
                // set them on the window directly. The vertical "splitter"
                // sizes the timeline (lower = taller); the horizontal
                // "splitter2" sizes the tools panel. The ratio persists
                // through the minimize reparent, so applying it here holds.
                auto settingsModel = app->getSettingsModel();
                auto win = settingsModel->getWindow();
                if (w.contains("splitter"))
                    win.splitter = w.at("splitter").get<float>();
                if (w.contains("splitter2"))
                    win.splitter2 = w.at("splitter2").get<float>();
                settingsModel->setWindow(win); // keep the settings consistent
                if (auto mw = app->getMainWindow())
                    mw->setSplitters(win.splitter, win.splitter2);
            }
        }

        void Capture::_applyEarly(const nlohmann::json& setup)
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return;
            for (const auto& step : setup)
            {
                if (step.contains("open"))
                {
                    ftk::Path path(step.at("open").get<std::string>());
                    if (path.hasSeqWildcard())
                        path = ftk::expandSeq(path);
                    app->open(path);
                    _setExpectMedia(true);
                }
            }
        }

        bool Capture::_isEarlyStep(const nlohmann::json& step) const
        {
            return step.contains("open");
        }

        bool Capture::_isLateStep(const nlohmann::json& step) const
        {
            // A pick samples the rendered image and a zoom needs the
            // viewport's laid-out geometry, so both must wait until the
            // viewport is sized and fit-zoomed.
            return
                ftk::Capture::_isLateStep(step) ||
                step.contains("pick") ||
                step.contains("zoom");
        }

        bool Capture::_applyStep(const nlohmann::json& step)
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return false;
            if (step.contains("tool"))
            {
                const std::string toolStr = step.at("tool").get<std::string>();
                // Pre-seed the tool's bellows-open settings so it expands the
                // requested sections when it loads (IToolWidget::_loadSettings).
                if (step.contains("expand"))
                {
                    const std::string key =
                        ftk::Format("/{0}/Bellows").arg(toolStr);
                    nlohmann::json bellows;
                    app->getSettings()->get(key, bellows);
                    const auto& expand = step.at("expand");
                    if (expand.is_string())
                        bellows[expand.get<std::string>()] = true;
                    else
                        for (const auto& name : expand)
                            bellows[name.get<std::string>()] = true;
                    app->getSettings()->set(key, bellows);

                    // Bring the expanded section into view once the tool is laid
                    // out: a bellows low in the list (e.g. Keyboard Shortcuts)
                    // would otherwise expand below the panel's visible area. The
                    // section is the last expanded one; tools without scrollable
                    // sections inherit IToolWidget's no-op scrollTo. Deferred to
                    // the late phase so the bellows geometry is valid.
                    std::string section;
                    if (expand.is_string())
                        section = expand.get<std::string>();
                    else if (expand.is_array() && !expand.empty())
                        section = expand.back().get<std::string>();
                    if (!section.empty())
                        _addLateStep(
                            { { "scrollTool",
                                { { "tool", toolStr }, { "section", section } } } });
                }
                app->getCommandsModel()->exec(
                    ftk::Format("Tools/{0}").arg(toolStr),
                    { { "value", true } });
            }
            else if (step.contains("scrollTool"))
            {
                // Deferred: scroll the active tool to a section (see the tool
                // verb above). Runs after the tool is laid out so scrollTo can
                // resolve the section's geometry.
                const auto& v = step.at("scrollTool");
                if (auto mainWindow = app->getMainWindow())
                {
                    if (auto tool = mainWindow->getToolWidget(
                        v.at("tool").get<std::string>()))
                    {
                        tool->scrollTo(v.at("section").get<std::string>());
                    }
                }
            }
            else if (step.contains("command"))
            {
                // Run a command by name, for the things that are neither a
                // setting nor have a verb of their own, e.g.
                //   { "command": "Window/PresentMode",
                //     "args": { "value": true } }
                // The name is the one the commands model knows it by, which
                // is what the shortcuts and the command line use.
                app->getCommandsModel()->exec(
                    step.at("command").get<std::string>(),
                    step.contains("args") ? step.at("args") : nlohmann::json::object());
            }
            else if (step.contains("exportDir"))
            {
                // Set the export directory, e.g. { "exportDir": "/tmp" }. It is
                // filled in with the home directory otherwise, which would put
                // whoever built the documentation into the screenshot.
                auto settingsModel = app->getSettingsModel();
                auto exportSettings = settingsModel->getExport();
                exportSettings.dir = step.at("exportDir").get<std::string>();
                settingsModel->setExport(exportSettings);
            }
            else if (step.contains("frame"))
            {
                app->getCommandsModel()->exec(
                    "Playback/Seek",
                    { { "frame", step.at("frame").get<double>() } });
            }
            else if (step.contains("inOut"))
            {
                // Set the playback in/out range (the blue range on the timeline)
                // from a pair of 0-based frame numbers, relative to the timeline
                // start like the "frame" verb. The out frame is inclusive, the
                // same convention as Set Out Point. e.g. { "inOut": [10, 50] }
                const auto& io = step.at("inOut");
                if (io.is_array() && io.size() >= 2)
                {
                    app->getCommandsModel()->exec(
                        "Playback/InOutRange",
                        {
                            { "in", io[0].get<double>() },
                            { "out", io[1].get<double>() }
                        });
                }
            }
            else if (step.contains("markers"))
            {
                // Replace the review markers, bypassing the editor. The
                // frames are 0-based relative to the timeline start like the
                // "frame" verb, and the out frame is inclusive. The creation
                // time is fixed here so rebuilding the documentation does not
                // stamp the build date into the shot. e.g.
                //   { "markers": [
                //     { "frame": 15, "text": "Flare on the lens" },
                //     { "range": [30, 60], "name": "Sky",
                //       "color": [.3, .5, .9] } ] }
                std::vector<models::ReviewMarker> markers;
                if (auto player = app->observePlayer()->get())
                {
                    const auto start = player->getTimeRange().start_time();
                    const double rate = start.rate();
                    int index = 0;
                    for (const auto& v : step.at("markers"))
                    {
                        models::ReviewMarker marker;
                        marker.id = ftk::Format("capture-{0}").arg(index++);
                        marker.created = "2026-09-01T17:00:00Z";
                        if (v.contains("name"))
                            marker.name = v.at("name").get<std::string>();
                        if (v.contains("text"))
                            marker.text = v.at("text").get<std::string>();
                        if (v.contains("frame"))
                        {
                            const OTIO_NS::RationalTime frame(
                                start.value() + v.at("frame").get<double>(),
                                rate);
                            marker.range = OTIO_NS::TimeRange(
                                frame, OTIO_NS::RationalTime(1.0, rate));
                        }
                        else if (v.contains("range"))
                        {
                            const auto& r = v.at("range");
                            marker.range = OTIO_NS::TimeRange::
                                range_from_start_end_time_inclusive(
                                    OTIO_NS::RationalTime(
                                        start.value() + r[0].get<double>(),
                                        rate),
                                    OTIO_NS::RationalTime(
                                        start.value() + r[1].get<double>(),
                                        rate));
                        }
                        if (v.contains("color"))
                        {
                            const auto& c = v.at("color");
                            marker.color = ftk::Color4F(
                                c[0].get<float>(),
                                c[1].get<float>(),
                                c[2].get<float>());
                        }
                        markers.push_back(marker);
                    }
                }
                app->getMarkersModel()->setMarkers(markers);
            }
            else if (step.contains("a"))
            {
                // Set the "A" (current) file by open-order index or path.
                const int i = _fileIndex(step.at("a"));
                if (i >= 0)
                    app->getFilesModel()->setA(i);
            }
            else if (step.contains("b"))
            {
                // Add a "B" file by open-order index or path.
                const int i = _fileIndex(step.at("b"));
                if (i >= 0)
                    app->getFilesModel()->setB(i, true);
            }
            else if (step.contains("compare"))
            {
                // Set the A/B comparison mode by its label, e.g. "Tile",
                // "Wipe", "Overlay", "Difference". The B files themselves are
                // set with the "b" verb.
                // Either { "compare": "Wipe" } to turn a comparison on, or
                // { "compare": { "mode": "Wipe", "value": false } } to turn
                // one off again: the comparisons are toggles, and not
                // comparing is the state with none of them on.
                const auto& v = step.at("compare");
                std::string name;
                nlohmann::json args;
                if (v.is_object())
                {
                    name = v.at("mode").get<std::string>();
                    if (v.contains("value"))
                        args["value"] = v.at("value").get<bool>();
                }
                else
                {
                    name = v.get<std::string>();
                }
                if (!app->getCommandsModel()->exec(
                    ftk::Format("Compare/{0}").arg(name), args))
                {
                    _note("unknown compare mode '" + name + "'");
                }
            }
            else if (step.contains("layer"))
            {
                // Set a file's active layer (for multi-layer EXRs), by layer
                // index or a name substring. "file" picks the target file
                // (index or path); without it, the current "A" file is used.
                auto model = app->getFilesModel();
                const int fi = step.contains("file")
                    ? _fileIndex(step.at("file")) : model->getAIndex();
                const auto& files = model->getFiles();
                if (fi >= 0 && fi < static_cast<int>(files.size()))
                {
                    const auto& item = files[fi];
                    const auto& value = step.at("layer");
                    int li = -1;
                    if (value.is_number_integer())
                    {
                        li = value.get<int>();
                    }
                    else
                    {
                        const std::string s = value.get<std::string>();
                        for (size_t k = 0; k < item->videoLayers.size(); ++k)
                        {
                            if (item->videoLayers[k].find(s) != std::string::npos)
                            {
                                li = static_cast<int>(k);
                                break;
                            }
                        }
                    }
                    if (li >= 0 && li < static_cast<int>(item->videoLayers.size()))
                        model->setLayer(item, li);
                    else
                        _note("no matching layer for the target file");
                }
            }
            else if (step.contains("mediaReference"))
            {
                // Set the media reference key, for clips that carry more than
                // one media reference (for example a proxy and a full
                // resolution version of the same media). An empty key leaves
                // each clip on the media reference it was authored with.
                const std::string key = step.at("mediaReference").get<std::string>();
                if (auto player = app->observePlayer()->get())
                {
                    const auto keys = player->getMediaReferenceKeys();
                    if (key.empty() ||
                        std::find(keys.begin(), keys.end(), key) != keys.end())
                    {
                        player->setMediaReferenceKey(key);
                    }
                    else
                    {
                        _note("no matching media reference key");
                    }
                }
            }
            else if (step.contains("view"))
            {
                // Viewport overlays/filters for the "Viewport" docs: grid,
                // HUD, and the magnify/minify image filters.
                // e.g. { "view": { "grid": true, "hud": true,
                //                  "magnify": "linear" } }
                const auto& v = step.at("view");
                auto vp = app->getViewportModel();
                if (v.contains("grid"))
                {
                    // Either { "grid": true } or an object that also sets the
                    // cell size, e.g. { "grid": { "enabled": true,
                    // "cellSize": 1 } } for a 1x1 (per-pixel) grid. Giving a
                    // cellSize switches the grid to cell-size mode.
                    auto fg = vp->getForegroundOptions();
                    const auto& gv = v.at("grid");
                    if (gv.is_boolean())
                    {
                        fg.grid.enabled = gv.get<bool>();
                    }
                    else if (gv.is_object())
                    {
                        if (gv.contains("enabled"))
                            fg.grid.enabled = gv.at("enabled").get<bool>();
                        if (gv.contains("cellSize"))
                        {
                            fg.grid.cellMode = tl::GridCellMode::CellSize;
                            fg.grid.cellSize = gv.at("cellSize").get<int>();
                        }
                        if (gv.contains("cellMode"))
                        {
                            // The grid cell mode, "Cell Size" or "Cell Count"
                            // (case-insensitive), parsed from the enum labels via
                            // from_string. Set explicitly when you want cell-count
                            // mode, or to override the mode implied by "cellSize".
                            const std::string s =
                                gv.at("cellMode").get<std::string>();
                            if (!from_string(s, fg.grid.cellMode))
                                _note(
                                    "unrecognized grid cell mode '" + s + "'");
                        }
                        if (gv.contains("labels"))
                        {
                            // The grid-labels mode, parsed case-insensitively
                            // from the same names shown in the View > Grid
                            // "Labels" menu via the enum's from_string.
                            const std::string s =
                                gv.at("labels").get<std::string>();
                            if (!from_string(s, fg.grid.labels))
                                _note(
                                    "unrecognized grid labels '" + s + "'");
                        }
                    }
                    vp->setForegroundOptions(fg);
                }
                if (v.contains("hud"))
                {
                    // Either { "hud": true } to enable/disable the HUD while
                    // keeping the current per-item corner layout, or an object
                    // that also sets item positions, e.g.
                    //   { "hud": { "enabled": true,
                    //              "items": { "File Name": "Top Left" } } }
                    // Item and position names are parsed case-insensitively from
                    // their enum labels via from_string, as in the View > HUD menu.
                    auto hud = vp->getHUDOptions();
                    const auto& hv = v.at("hud");
                    if (hv.is_boolean())
                    {
                        hud.enabled = hv.get<bool>();
                    }
                    else if (hv.is_object())
                    {
                        if (hv.contains("enabled"))
                            hud.enabled = hv.at("enabled").get<bool>();
                        if (hv.contains("items"))
                        {
                            for (const auto& [key, value] : hv.at("items").items())
                            {
                                models::HUDItem item = models::HUDItem::First;
                                models::HUDPos pos = models::HUDPos::First;
                                if (!from_string(key, item))
                                    _note(
                                        "unrecognized HUD item '" + key + "'");
                                else if (!from_string(value.get<std::string>(), pos))
                                    _note(
                                        "unrecognized HUD position '" +
                                        value.get<std::string>() + "'");
                                else
                                    hud.items[item] = pos;
                            }
                        }
                    }
                    vp->setHUDOptions(hud);
                }
                if (v.contains("aspectRatio"))
                {
                    // Select one of the aspect ratio presets by its index in
                    // the View > Aspect Ratio menu, 0 being Default, e.g.
                    //   { "aspectRatio": 3 }
                    // or override the preset outright, which is the only way
                    // to capture a pixel aspect ratio since the defaults are
                    // all display ratios, e.g.
                    //   { "aspectRatio": { "index": 3, "num": 2, "den": 1,
                    //                      "type": "Pixel" } }
                    auto options = vp->getAspectRatioOptions();
                    const auto& av = v.at("aspectRatio");
                    int index = options.index;
                    if (av.is_number_integer())
                    {
                        index = av.get<int>();
                    }
                    else if (av.is_object())
                    {
                        if (av.contains("index"))
                            index = av.at("index").get<int>();
                        if (index > 0 &&
                            index < static_cast<int>(options.options.size()))
                        {
                            auto& preset = options.options[index];
                            if (av.contains("num"))
                                preset.value.num = av.at("num").get<float>();
                            if (av.contains("den"))
                                preset.value.den = av.at("den").get<float>();
                            if (av.contains("type"))
                            {
                                const std::string s =
                                    av.at("type").get<std::string>();
                                if (!from_string(s, preset.type))
                                    _note(
                                        "unrecognized aspect ratio type '" + s + "'");
                            }
                        }
                    }
                    if (index >= 0 &&
                        index < static_cast<int>(options.options.size()))
                    {
                        options.index = index;
                        vp->setAspectRatioOptions(options);
                    }
                    else
                    {
                        _note(ftk::Format(
                            "aspect ratio index {0} is out of range").arg(index));
                    }
                }
                if (v.contains("magnify") || v.contains("minify"))
                {
                    // The viewport's magnify/minify filters, which the View
                    // tool reads and writes as well. "linear", "nearest" or
                    // "high quality" (case-insensitive) via from_string.
                    auto display = vp->getImageOptions();
                    if (v.contains("magnify"))
                    {
                        const std::string s = v.at("magnify").get<std::string>();
                        if (!from_string(s, display.imageFilters.magnify))
                            _note(
                                "unrecognized magnify filter '" + s + "'");
                    }
                    if (v.contains("minify"))
                    {
                        const std::string s = v.at("minify").get<std::string>();
                        if (!from_string(s, display.imageFilters.minify))
                            _note(
                                "unrecognized minify filter '" + s + "'");
                    }
                    vp->setImageOptions(display);
                }
            }
            else if (step.contains("timeline"))
            {
                // Timeline settings, which live in the settings model and drive
                // the timeline widget through its observer (same path as the
                // Timeline menu). e.g. { "timeline": { "minimize": false,
                // "thumbnailSize": "Large" } } -- minimize=false expands the
                // timeline to show all of its parts; a larger thumbnailSize
                // makes it taller.
                const auto& v = step.at("timeline");
                auto settingsModel = app->getSettingsModel();
                auto settings = settingsModel->getTimeline();
                if (v.contains("minimize"))
                    settings.minimize = v.at("minimize").get<bool>();
                if (v.contains("thumbnailSize"))
                {
                    const std::string s =
                        v.at("thumbnailSize").get<std::string>();
                    if (!from_string(s, settings.thumbnailSize))
                        _note(
                            "unrecognized timeline thumbnailSize '" + s + "'");
                }
                settingsModel->setTimeline(settings);
            }
            else if (step.contains("ocio"))
            {
                // Enable OCIO for the Color tool screenshot. Two forms:
                //   { "ocio": "etc/SampleData/config.ocio" }   // config file
                //   { "ocio": { "config": "Built In",          // or "Environment Variable" / "File"
                //               "fileName": "...",             // for File
                //               "input":   "ACES2065-1",
                //               "display": "sRGB - Display",
                //               "view":    "ACES 2.0 - SDR 100 nits (Rec.709)",
                //               "look":    "..." } }
                // The string form selects a config file; the object form sets
                // the config mode plus the color-space selections shown in the
                // tool. With a built-in config the spaces are available
                // immediately, so the selections take effect on load.
                auto options = app->getColorModel()->getOCIOOptions();
                options.enabled = true;
                const auto& ocio = step.at("ocio");
                if (ocio.is_string())
                {
                    options.config = tl::OCIOConfig::File;
                    options.fileName = ocio.get<std::string>();
                }
                else if (ocio.is_object())
                {
                    if (ocio.contains("config"))
                        from_string(ocio.at("config").get<std::string>(), options.config);
                    if (ocio.contains("fileName"))
                        options.fileName = ocio.at("fileName").get<std::string>();
                    if (ocio.contains("input"))
                        options.input = ocio.at("input").get<std::string>();
                    if (ocio.contains("display"))
                        options.display = ocio.at("display").get<std::string>();
                    if (ocio.contains("view"))
                        options.view = ocio.at("view").get<std::string>();
                    if (ocio.contains("look"))
                        options.look = ocio.at("look").get<std::string>();
                }
                app->getColorModel()->setOCIOOptions(options);
            }
            else if (step.contains("lut"))
            {
                // Enable a LUT file, any format OpenColorIO's FileTransform
                // reads. Two forms:
                //   { "lut": "monitor.icc" }
                //   { "lut": { "fileName": "monitor.icc",
                //              "direction": "Inverse" } }  // or "Forward"
                auto options = app->getColorModel()->getLUTOptions();
                options.enabled = true;
                const auto& lut = step.at("lut");
                if (lut.is_string())
                {
                    options.fileName = lut.get<std::string>();
                }
                else if (lut.is_object())
                {
                    if (lut.contains("fileName"))
                        options.fileName = lut.at("fileName").get<std::string>();
                    if (lut.contains("direction"))
                        from_string(
                            lut.at("direction").get<std::string>(),
                            options.direction);
                }
                app->getColorModel()->setLUTOptions(options);
            }
            else if (step.contains("fileBrowser"))
            {
                // Open the in-app file browser dialog. Force the non-native
                // dialog, and force it not to float, so it renders inside our
                // window and can be captured (the native OS dialog and a
                // floating browser are separate, uncapturable windows).
                //
                // Accepts either { "fileBrowser": true } or an object that
                // configures the dialog before opening:
                //   { "fileBrowser": {
                //       "path": "/abs/or/cwd-relative/dir",
                //       "bellows": { "Drives": false }   // sidebar sections
                //   } }
                // These mirror the file-browser settings (path + options),
                // applied straight to the model since the live settings only
                // re-push them at startup.
                if (auto context = _getContext())
                {
                    auto fbs = context->getSystem<ftk::FileBrowserSystem>();
                    fbs->setNativeFileDialog(false);
                    fbs->setFloating(false);

                    const auto& v = step.at("fileBrowser");
                    if (v.is_object())
                    {
                        auto model = fbs->getModel();
                        if (v.contains("path"))
                        {
                            const auto path = ftk::toFileSystem(
                                v.at("path").get<std::string>());
                            if (std::filesystem::exists(path))
                                model->setPath(path);
                            else
                                _note("fileBrowser path does not exist: " +
                                    ftk::fromFileSystem(path));
                        }
                        if (v.contains("bellows") && v.at("bellows").is_object())
                        {
                            // Open/close named sidebar sections: Drives,
                            // Shortcuts, Recent, Settings. Unlisted ones keep
                            // their defaults.
                            auto options = model->getOptions();
                            const auto& b = v.at("bellows");
                            for (auto it = b.begin(); it != b.end(); ++it)
                                options.bellows[it.key()] = it.value().get<bool>();
                            model->setOptions(options);
                        }
                    }
                }
                app->openDialog();
            }
            else if (step.contains("nativeFileBrowser"))
            {
                // Toggle the "native file dialog" setting -- for its own
                // Settings screenshot, or to prepare the file-browser shot.
                auto sm = app->getSettingsModel();
                auto fb = sm->getFileBrowser();
                fb.nativeFileDialog = step.at("nativeFileBrowser").get<bool>();
                sm->setFileBrowser(fb);
            }
            else if (step.contains("zoom"))
            {
                // Set the viewport zoom (turning off auto-frame so it sticks),
                // e.g. to make a fine grid visible. Deferred by _applyRest until
                // the viewport is laid out. Forms:
                //   { "zoom": 16 }                      centered on the view
                //   { "zoom": { "value": 16 } }         same
                //   { "zoom": { "value": 16, "center": [cx, cy] } }
                //       center image pixel (cx, cy) in the viewport
                if (auto mw = app->getMainWindow())
                {
                    auto viewport = mw->getViewport();
                    viewport->setFrameView(false); // else auto-fit overrides it
                    const auto& v = step.at("zoom");
                    double value = 1.0;
                    if (v.is_number())
                        value = v.get<double>();
                    else if (v.is_object())
                        value = v.value("value", 1.0);
                    const ftk::Box2I g = viewport->getGeometry();
                    if (v.is_object() && v.contains("center") &&
                        v.at("center").is_array() && v.at("center").size() >= 2)
                    {
                        const auto& c = v.at("center");
                        const double cx = c[0].get<double>();
                        const double cy = c[1].get<double>();
                        // Place image pixel (cx, cy) at the viewport center:
                        // viewPos = center - imagePixel * zoom.
                        const ftk::V2I pos(
                            g.w() / 2 - static_cast<int>(cx * value),
                            g.h() / 2 - static_cast<int>(cy * value));
                        viewport->setViewPosAndZoom(pos, value);
                    }
                    else
                    {
                        const ftk::V2I focus(g.w() / 2, g.h() / 2);
                        viewport->setZoom(value, focus);
                    }
                }
            }
            else if (step.contains("pick"))
            {
                // Sample the image at the given pixel for the Color Picker and
                // Magnify tools. Deferred by _applyRest until after the viewport
                // settles, so the zoom and rendered pixel are final.
                // e.g. { "pick": [160, 90] }
                const auto& v = step.at("pick");
                if (v.is_array() && v.size() >= 2)
                {
                    const ftk::V2I imagePos(v[0].get<int>(), v[1].get<int>());
                    if (auto mw = app->getMainWindow())
                        mw->getViewport()->pick(imagePos);
                }
            }
            else
            {
                return ftk::Capture::_applyStep(step);
            }
            return true;
        }

        int Capture::_fileIndex(const nlohmann::json& value) const
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return -1;
            if (value.is_number_integer())
                return value.get<int>();
            // Match a substring of the file path (e.g. a base name).
            const std::string s = value.get<std::string>();
            const auto& files = app->getFilesModel()->getFiles();
            for (size_t i = 0; i < files.size(); ++i)
            {
                if (files[i]->path.get().find(s) != std::string::npos)
                    return static_cast<int>(i);
            }
            _note("no file matches '" + s + "' for A/B");
            return -1;
        }

        bool Capture::_ready() const
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return false;
            auto player = app->observePlayer()->get();
            // Audio counts: audio-only media has no video information but
            // does have something to show, the waveform in the timeline.
            return player &&
                (!player->getIOInfo().video.empty() ||
                    player->getIOInfo().audio.isValid());
        }

        bool Capture::_frameReady() const
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return false;
            auto player = app->observePlayer()->get();
            if (!player)
                return false;
            if (player->getIOInfo().video.empty())
                return true; // Audio-only: there is no frame to wait for.

            // A seek reports the new current time straight away, but the
            // frame for it is decoded asynchronously and arrives later. The
            // settle alone is not long enough to cover a cold read, so a shot
            // could capture whatever frame was on screen beforehand.
            const auto& video = player->getCurrentVideo();
            if (video.empty() ||
                video.front().layers.empty() ||
                !video.front().layers.front().image)
                return false;
            return tl::compareExact(
                std::optional<OTIO_NS::RationalTime>(video.front().time),
                std::optional<OTIO_NS::RationalTime>(player->getCurrentTime()));
        }

        std::string Capture::_mediaError() const
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return std::string();
            auto player = app->observePlayer()->get();
            if (!player || _ready())
                return std::string();
            // A player's information is worked out when its timeline is read,
            // so a player with neither video nor audio will never gain them:
            // the media could not be read, and a path in the manifest that
            // does not exist arrives here. Waiting out the timeout for it
            // reports the wrong thing and costs twelve seconds per shot.
            const std::string error =
                player->getTimeline()->getReadError();
            return !error.empty() ?
                error :
                std::string("no video or audio; run with -log for the reader");
        }

        std::string Capture::_waitingFor() const
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return "the application to still be running";
            if (!app->observePlayer()->get())
                return "a player, which the media never produced";
            if (!_ready())
                return "the media to report its video or audio information";
            return "the frame at the current time to be decoded";
        }
    }
}
