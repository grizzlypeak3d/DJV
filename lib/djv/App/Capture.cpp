// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/Capture.h>

#include <djv/App/App.h>
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

            bool startsWith(const std::string& s, const std::string& prefix)
            {
                return s.size() >= prefix.size() &&
                    0 == s.compare(0, prefix.size(), prefix);
            }

            void collect(
                const std::shared_ptr<ftk::IWidget>& widget,
                std::vector<std::shared_ptr<ftk::IWidget> >& out)
            {
                if (!widget)
                    return;
                if (widget->hasProperty(ui::detail::screenshotKey) && widget->isVisible(true))
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
            if (p.ticks > timeoutTicks)
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
                    p.settleLeft = settleTicks;
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
                        p.settleLeft = settleTicks;
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
                if (step.contains("pick"))
                {
                    // A pick samples the rendered image, so it must wait until
                    // the viewport is sized and fit-zoomed. Defer to after the
                    // first settle (see _onTick).
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
                }
                models::Tool tool = models::Tool::None;
                from_string(toolStr, tool);
                app->getToolsModel()->setActiveTool(tool);
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
                const std::string name = step.at("compare").get<std::string>();
                const auto labels = tl::getCompareLabels();
                int index = -1;
                for (size_t i = 0; i < labels.size(); ++i)
                {
                    if (labels[i] == name)
                    {
                        index = static_cast<int>(i);
                        break;
                    }
                }
                if (index >= 0)
                {
                    auto options = app->getFilesModel()->getCompareOptions();
                    options.compare = static_cast<tl::Compare>(index);
                    app->getFilesModel()->setCompareOptions(options);
                }
                else
                {
                    note(p.shotId, "unknown compare mode '" + name + "'");
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
                        note(p.shotId, "no matching layer for the target file");
                }
            }
            else if (step.contains("view"))
            {
                // Viewport overlays for the "Viewport" docs: grid and/or HUD.
                // e.g. { "view": { "grid": true, "hud": true } }
                const auto& v = step.at("view");
                auto vp = app->getViewportModel();
                if (v.contains("grid"))
                {
                    auto fg = vp->getForegroundOptions();
                    fg.grid.enabled = v.at("grid").get<bool>();
                    vp->setForegroundOptions(fg);
                }
                if (v.contains("hud"))
                {
                    vp->setHUD(v.at("hud").get<bool>());
                }
            }
            else if (step.contains("ocio"))
            {
                // Enable OCIO with a config file for the Color tool screenshot.
                // The Color tool loads the config and fills in its input /
                // display / view lists when it opens. e.g.
                // { "ocio": "etc/SampleData/config.ocio" }
                auto options = app->getColorModel()->getOCIOOptions();
                options.enabled = true;
                options.config = tl::OCIOConfig::File;
                options.fileName = step.at("ocio").get<std::string>();
                app->getColorModel()->setOCIOOptions(options);
            }
            else if (step.contains("fileBrowser"))
            {
                // Open the in-app file browser dialog. Force the non-native
                // dialog so it renders inside our window and can be captured
                // (the native OS dialog is a separate, uncapturable window).
                if (auto context = p.context.lock())
                {
                    auto fbs = context->getSystem<ftk::FileBrowserSystem>();
                    fbs->setNativeFileDialog(false);
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
                    { "id", w->getProperty(ui::detail::screenshotKey) },
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

            std::ofstream f(path);
            f << out.dump(2) << std::endl;
        }
    }
}
