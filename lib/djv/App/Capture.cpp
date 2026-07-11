// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/Capture.h>

#include <djv/App/App.h>
#include <djv/App/IToolWidget.h>
#include <djv/App/MainWindow.h>
#include <djv/App/Viewport.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/ToolsModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/UI/App.h>
#include <ftk/UI/FileBrowser.h>
#include <ftk/UI/IWindow.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Settings.h>
#include <ftk/Core/Context.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Image.h>
#include <ftk/Core/ImageIO.h>
#include <ftk/Core/Timer.h>
#include <ftk/Core/Path.h>

#include <tlRender/Timeline/CompareOptions.h>
#include <tlRender/Timeline/Player.h>

#include <fstream>
#include <iostream>

namespace djv
{
    namespace app
    {
        namespace
        {
            // Timer cadence and budgets, in timer ticks.
            const std::chrono::milliseconds tickInterval(30);
            const int settleTicks = 15;     // frames to draw before capturing
            const int reloadGraceTicks = 4; // let a setup-triggered reload begin
            const int timeoutTicks = 400;   // ~12s hard cap waiting for media

            // Console diagnostics so failures are not silent.
            void note(const std::string& shot, const std::string& msg)
            {
                std::cerr << "djv capture [" << shot << "]: " << msg << std::endl;
            }

            void collect(
                const std::shared_ptr<ftk::IWidget>& widget,
                std::vector<std::shared_ptr<ftk::IWidget> >& out)
            {
                if (!widget)
                    return;
                if (ftk::hasScreenshotTag(widget) && widget->isVisible(true))
                    out.push_back(widget);
                for (const auto& child : widget->getChildren())
                    collect(child, out);
            }

            enum class Phase { WaitReady, ApplyRest, Reload, Settle, Done };
        }

        struct Capture::Private
        {
            std::weak_ptr<ftk::Context> context;
            std::weak_ptr<App> app;
            std::filesystem::path manifest;
            std::string shotId;
            std::filesystem::path outputDir;

            nlohmann::json shot;
            bool expectMedia = false;

            std::shared_ptr<ftk::Timer> timer;
            Phase phase = Phase::WaitReady;
            int ticks = 0;
            int settleLeft = settleTicks;
            int settleTicksShot = settleTicks; // per-shot, from the "settle" field
            int reloadGrace = reloadGraceTicks;
            std::vector<nlohmann::json> lateSteps;  // applied after first settle
            bool lateApplied = false;
            bool done = false;
            bool success = false;
        };

        void Capture::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::filesystem::path& manifest,
            const std::string& shotId,
            const std::filesystem::path& outputDir)
        {
            FTK_P();
            p.context = context;
            p.app = app;
            p.manifest = manifest;
            p.shotId = shotId;
            p.outputDir = outputDir;
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

        bool Capture::begin()
        {
            FTK_P();
            auto context = p.context.lock();
            auto app = p.app.lock();
            if (!context || !app)
                return false;

            // Resolve the requested shot from the manifest.
            try
            {
                std::ifstream f(p.manifest);
                if (!f.is_open())
                    throw std::runtime_error(ftk::Format(
                        "cannot open manifest \"{0}\"").arg(p.manifest.u8string()));
                nlohmann::json doc;
                f >> doc;
                bool found = false;
                for (const auto& shot : doc.at("shots"))
                {
                    if (shot.value("id", std::string()) == p.shotId)
                    {
                        p.shot = shot;
                        found = true;
                        break;
                    }
                }
                if (!found)
                    throw std::runtime_error("shot id not found in manifest");
            }
            catch (const std::exception& e)
            {
                note(p.shotId, e.what());
                return false;
            }

            // A shot may widen the settle window (in seconds) to let slow async
            // work finish before the capture -- e.g. timeline thumbnails, which
            // stream in after the media is ready. Defaults to settleTicks.
            const double settleSeconds = p.shot.value("settle", 0.0);
            if (settleSeconds > 0.0)
            {
                int ticks = static_cast<int>(
                    settleSeconds * 1000.0 / tickInterval.count());
                if (ticks < settleTicks)
                    ticks = settleTicks;
                p.settleTicksShot = ticks;
            }

            if (app->getWindows().empty())
            {
                note(p.shotId, "no window was created");
                return false;
            }
            auto window = app->getWindows().front();

            // Deterministic presentation. Capture runs should also pass
            // -resetSettings so saved window state can't override this.
            app->setColorStyle(ftk::ColorStyle::Dark);
            app->setTooltipsEnabled(false);
            if (p.shot.contains("window"))
            {
                const auto& w = p.shot.at("window");
                if (w.contains("w") && w.contains("h"))
                    window->setSize(ftk::Size2I(w.at("w").get<int>(), w.at("h").get<int>()));
                if (w.contains("scale"))
                    app->setDisplayScale(w.at("scale").get<float>());
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
            window->show();

            // Open the shot's files now; the rest of the setup waits until the
            // player is ready (handled in the timer state machine).
            _applyOpens(p.shot.value("setup", nlohmann::json::array()));

            // Arm the capture timer. It fires from inside ftk::App::run(),
            // after the window is realized and drawing.
            p.timer = ftk::Timer::create(context);
            p.timer->setRepeating(true);
            auto weak = std::weak_ptr<Capture>(shared_from_this());
            p.timer->start(tickInterval, [weak] {
                if (auto self = weak.lock())
                    self->_onTick();
            });
            return true;
        }

        bool Capture::succeeded() const
        {
            return _p->success;
        }

        void Capture::_onTick()
        {
            FTK_P();
            if (p.done)
                return;
            ++p.ticks;
            if (p.ticks > timeoutTicks + (p.settleTicksShot - settleTicks))
            {
                note(p.shotId, "timed out waiting for the shot to become ready");
                _finish(false);
                return;
            }

            switch (p.phase)
            {
            case Phase::WaitReady:
                if (!p.expectMedia || _ready())
                    p.phase = Phase::ApplyRest;
                break;
            case Phase::ApplyRest:
                _applyRest(p.shot.value("setup", nlohmann::json::array()));
                p.phase = Phase::Reload;
                p.reloadGrace = reloadGraceTicks;
                break;
            case Phase::Reload:
                // Setup steps that change the active files (A/B, compare) make
                // the player reload. Give the reload a moment to begin, then
                // wait until the player is ready again before settling.
                if (p.reloadGrace > 0)
                    --p.reloadGrace;
                else if (!p.expectMedia || _ready())
                {
                    p.phase = Phase::Settle;
                    p.settleLeft = p.settleTicksShot;
                }
                break;
            case Phase::Settle:
                if (--p.settleLeft <= 0)
                {
                    if (!p.lateSteps.empty() && !p.lateApplied)
                    {
                        // Viewport is now sized and fit; apply deferred picks
                        // and settle once more so the sample/HUD render.
                        p.lateApplied = true;
                        for (const auto& step : p.lateSteps)
                            _applyStep(step);
                        p.settleLeft = p.settleTicksShot;
                    }
                    else
                    {
                        p.phase = Phase::Done; // captured below this switch
                    }
                }
                break;
            default:
                break;
            }

            if (Phase::Done == p.phase && !p.done)
            {
                std::error_code ec;
                std::filesystem::create_directories(p.outputDir, ec);
                const auto png = p.outputDir / (p.shotId + ".png");
                const auto json = p.outputDir / (p.shotId + ".json");
                bool ok = _writePNG(png);
                if (ok)
                {
                    _writeMetadata(json);
                    note(p.shotId, ftk::Format("captured {0}").arg(png.u8string()));
                }
                _finish(ok);
            }
        }

        void Capture::_finish(bool ok)
        {
            FTK_P();
            p.success = ok;
            p.done = true;
            if (p.timer)
                p.timer->stop();
            if (auto app = p.app.lock())
                app->exit();
        }

        void Capture::_applyOpens(const nlohmann::json& setup)
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
                    p.expectMedia = true;
                }
            }
        }

        void Capture::_applyRest(const nlohmann::json& setup)
        {
            FTK_P();
            for (const auto& step : setup)
            {
                if (step.contains("open"))
                    continue;
                if (step.contains("pick") || step.contains("zoom"))
                {
                    // A pick samples the rendered image and a zoom needs the
                    // viewport's laid-out geometry, so both must wait until the
                    // viewport is sized and fit-zoomed. Defer to after the first
                    // settle (see _onTick). Manifest order is preserved, so a
                    // zoom listed before a pick is applied first.
                    p.lateSteps.push_back(step);
                    continue;
                }
                _applyStep(step);
            }
        }

        void Capture::_applyStep(const nlohmann::json& step)
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return;
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
                        p.lateSteps.push_back({ { "scrollTool", section } });
                }
                app->getToolsModel()->setActiveTool(toolStr);
            }
            else if (step.contains("scrollTool"))
            {
                // Deferred: scroll the active tool to a section (see the tool
                // verb above). Runs after the tool is laid out so scrollTo can
                // resolve the section's geometry.
                const std::string section =
                    step.at("scrollTool").get<std::string>();
                if (auto mainWindow = app->getMainWindow())
                {
                    if (auto tool = mainWindow->getToolWidget())
                        tool->scrollTo(section);
                }
            }
            else if (step.contains("frame"))
            {
                if (auto player = app->observePlayer()->get())
                {
                    const auto start = player->getTimeRange().start_time();
                    player->seek(OTIO_NS::RationalTime(
                        start.value() + step.at("frame").get<double>(),
                        start.rate()));
                }
            }
            else if (step.contains("inOut"))
            {
                // Set the playback in/out range (the blue range on the timeline)
                // from a pair of 0-based frame numbers, relative to the timeline
                // start like the "frame" verb. The out frame is inclusive, the
                // same convention as Set Out Point. e.g. { "inOut": [10, 50] }
                if (auto player = app->observePlayer()->get())
                {
                    const auto& io = step.at("inOut");
                    if (io.is_array() && io.size() >= 2)
                    {
                        const auto start = player->getTimeRange().start_time();
                        const double rate = start.rate();
                        const OTIO_NS::RationalTime inT(
                            start.value() + io[0].get<double>(), rate);
                        const OTIO_NS::RationalTime outT(
                            start.value() + io[1].get<double>(), rate);
                        player->setInOutRange(
                            OTIO_NS::TimeRange::range_from_start_end_time_inclusive(
                                inT, outT));
                    }
                }
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
                // "Wipe", "Overlay", "Difference" (case-insensitive). The B
                // files themselves are set with the "b" verb.
                const std::string name = step.at("compare").get<std::string>();
                auto options = app->getFilesModel()->getCompareOptions();
                if (from_string(name, options.compare))
                    app->getFilesModel()->setCompareOptions(options);
                else
                    note(p.shotId, "unknown compare mode '" + name + "'");
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
                        note(p.shotId, "no matching layer for the target file");
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
                                note(p.shotId,
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
                                note(p.shotId,
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
                                    note(p.shotId,
                                        "unrecognized HUD item '" + key + "'");
                                else if (!from_string(value.get<std::string>(), pos))
                                    note(p.shotId,
                                        "unrecognized HUD position '" +
                                        value.get<std::string>() + "'");
                                else
                                    hud.items[item] = pos;
                            }
                        }
                    }
                    vp->setHUDOptions(hud);
                }
                if (v.contains("magnify") || v.contains("minify"))
                {
                    // The viewport's magnify/minify filters live in the display
                    // options (this is what the View tool reads and writes), not
                    // the image options -- both structs have an imageFilters
                    // field, but only the display one drives the render.
                    // "linear" or "nearest" (case-insensitive) via from_string.
                    auto display = vp->getDisplayOptions();
                    if (v.contains("magnify"))
                    {
                        const std::string s = v.at("magnify").get<std::string>();
                        if (!from_string(s, display.imageFilters.magnify))
                            note(p.shotId,
                                "unrecognized magnify filter '" + s + "'");
                    }
                    if (v.contains("minify"))
                    {
                        const std::string s = v.at("minify").get<std::string>();
                        if (!from_string(s, display.imageFilters.minify))
                            note(p.shotId,
                                "unrecognized minify filter '" + s + "'");
                    }
                    vp->setDisplayOptions(display);
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
                        note(p.shotId,
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
            else if (step.contains("fileBrowser"))
            {
                // Open the in-app file browser dialog. Force the non-native
                // dialog so it renders inside our window and can be captured
                // (the native OS dialog is a separate, uncapturable window).
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
                if (auto context = p.context.lock())
                {
                    auto fbs = context->getSystem<ftk::FileBrowserSystem>();
                    fbs->setNativeFileDialog(false);

                    const auto& v = step.at("fileBrowser");
                    if (v.is_object())
                    {
                        auto model = fbs->getModel();
                        if (v.contains("path"))
                        {
                            const auto path = std::filesystem::u8path(
                                v.at("path").get<std::string>());
                            if (std::filesystem::exists(path))
                                model->setPath(path);
                            else
                                note(p.shotId, "fileBrowser path does not exist: " +
                                    path.u8string());
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
            note(p.shotId, "no file matches '" + s + "' for A/B");
            return -1;
        }

        bool Capture::_ready() const
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return false;
            auto player = app->observePlayer()->get();
            return player && !player->getIOInfo().video.empty();
        }

        bool Capture::_writePNG(const std::filesystem::path& path) const
        {
            FTK_P();
            auto context = p.context.lock();
            auto app = p.app.lock();
            if (!context || !app || app->getWindows().empty())
                return false;
            auto image = app->getWindows().front()->screenshot();
            if (!image)
            {
                note(p.shotId, "screenshot returned no image (offscreen buffer not ready)");
                return false;
            }

            // ftk blends with straight (non-premultiplied) alpha, so a
            // semi-transparent overlay -- e.g. a dialog's dimming scrim -- leaves
            // that region of the RGBA buffer with alpha < 1, even though its RGB
            // is the correct darkened result. A window capture is logically
            // opaque; left as-is those pixels let the white docs page show
            // through and the dimming reads as light/inverted. Force the image
            // fully opaque before writing (the RGB already matches the screen).
            if (uint8_t* data = image->getData())
            {
                const ftk::Size2I size = image->getSize();
                const size_t bytes = image->getByteCount();
                if (bytes == static_cast<size_t>(size.w) * size.h * 4)
                {
                    for (size_t i = 3; i < bytes; i += 4)
                        data[i] = 255;
                }
            }

            auto io = context->getSystem<ftk::ImageIO>();
            auto writer = io->write(path, image->getInfo());
            if (!writer)
            {
                note(p.shotId, ftk::Format("no PNG writer for \"{0}\"").arg(path.u8string()));
                return false;
            }
            writer->write(image);
            return true;
        }

        void Capture::_writeMetadata(const std::filesystem::path& path) const
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app || app->getWindows().empty())
                return;
            auto window = app->getWindows().front();

            std::vector<std::shared_ptr<ftk::IWidget> > tagged;
            collect(window, tagged);

            nlohmann::json widgets = nlohmann::json::array();
            for (const auto& w : tagged)
            {
                const ftk::Box2I g = w->getGeometry();
                widgets.push_back({
                    { "id", ftk::getScreenshotTag(w) },
                    { "box", { g.x(), g.y(), g.w(), g.h() } } });
            }

            // Boxes and the screenshot share the offscreen buffer's pixel space
            // (window size x display scale), so they already line up regardless
            // of scale. Record the display scale as "dpr" so make_svg can show a
            // high-DPI capture at its logical size -- crisp, not enlarged.
            const ftk::Size2I size = window->getGeometry().size();
            nlohmann::json out = {
                { "shot", p.shotId },
                { "image", p.shotId + ".png" },
                { "dpr", app->getDisplayScale() },
                { "window", { { "w", size.w }, { "h", size.h } } },
                { "widgets", widgets } };
            if (p.shot.contains("annotate"))
                out["annotate"] = p.shot.at("annotate");
            // Per-shot make_svg options pass straight through to the sidecar so
            // the SVG step is driven entirely by the sidecar -- build_screenshots
            // no longer needs to re-parse the manifest to forward them.
            if (p.shot.contains("crop"))
                out["crop"] = p.shot.at("crop");
            if (p.shot.contains("layout"))
                out["layout"] = p.shot.at("layout");
            if (p.shot.contains("cropFit"))
                out["cropFit"] = p.shot.at("cropFit");

            std::ofstream f(path);
            f << out.dump(2) << std::endl;
        }
    }
}
