// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/SettingsModel.h>

#include <ftk/UI/ColorWidgetSystem.h>

#include <ftk/UI/Settings.h>
#include <ftk/Core/Error.h>
#include <ftk/Core/Path.h>
#include <ftk/Core/String.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>

namespace djv
{
    namespace models
    {

        FTK_ENUM_IMPL(
            ExportRenderSize,
            "Default",
            "1920",
            "3840",
            "4096",
            "Custom");

        int getWidth(ExportRenderSize value)
        {
            const std::array<int, static_cast<size_t>(ExportRenderSize::Count)> data =
            {
                0,
                1920,
                3840,
                4096,
                0
            };
            return data[static_cast<size_t>(value)];
        }

        FTK_ENUM_IMPL(
            ExportFileType,
            "Image",
            "Seq",
            "Movie");

        ShortcutsSettings::ShortcutsSettings()
        {}

        FTK_ENUM_IMPL(
            MouseAction,
            "Pan View",
            "Compare Wipe",
            "Pick",
            "Frame Shuttle");

        FTK_ENUM_IMPL(
            WheelAction,
            "Zoom",
            "Scrub");

        MouseActionBinding::MouseActionBinding(
            ftk::MouseButton button,
            ftk::KeyModifier modifier) :
            button(button),
            modifier(modifier)
        {}

        StyleSettings::StyleSettings()
        {
            for (const auto font : ftk::getFontTypeEnums())
            {
                fonts[font] = ftk::getDefaultFont(font);
            }
        }

        FTK_ENUM_IMPL(
            TimelineThumbnailSize,
            "Small",
            "Medium",
            "Large");

        int getTimelineThumbnailSize(TimelineThumbnailSize value)
        {
            const std::array<int, static_cast<size_t>(TimelineThumbnailSize::Count)> data =
            {
                50,
                100,
                200
            };
            return data[static_cast<size_t>(value)];
        }

        int getTimelineWaveformSize(TimelineThumbnailSize value)
        {
            const std::array<int, static_cast<size_t>(TimelineThumbnailSize::Count)> data =
            {
                50 / 2,
                100 / 2,
                200 / 2
            };
            return data[static_cast<size_t>(value)];
        }

        struct SettingsModel::Private
        {
            std::weak_ptr<ftk::Context> context;
            std::shared_ptr<ftk::Settings> settings;
            std::map<std::string, Shortcut> savedShortcuts;
            ShortcutsSettings shortcutsDefault;

            std::shared_ptr<ftk::Observable<AudioSettings> > audio;
            std::shared_ptr<ftk::Observable<tl::PlayerCacheOptions> > cache;
            std::shared_ptr<ftk::Observable<tl::ui::ThumbnailCacheOptions> > thumbnailCache;
            std::shared_ptr<ftk::Observable<ExportSettings> > exportSettings;
            std::shared_ptr<ftk::Observable<FileBrowserSettings> > fileBrowser;
            std::shared_ptr<ftk::Observable<ImageSeqSettings> > imageSeq;
            std::shared_ptr<ftk::Observable<OTIOSettings> > otio;
            std::shared_ptr<ftk::Observable<ShortcutsSettings> > shortcuts;
            std::shared_ptr<ftk::Observable<MiscSettings> > misc;
            std::shared_ptr<ftk::Observable<MouseSettings> > mouse;
            std::shared_ptr<ftk::Observable<PlaybackSettings> > playback;
            std::shared_ptr<ftk::Observable<StyleSettings> > style;
            std::shared_ptr<ftk::Observable<TimelineSettings> > timeline;
            std::shared_ptr<ftk::Observable<WindowSettings> > window;
#if defined(TLRENDER_FFMPEG_PLUGIN)
            std::shared_ptr<ftk::Observable<tl::ffmpeg::Options> > ffmpeg;
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_FFMPEG_PLUGIN)
            std::shared_ptr<ftk::Observable<tl::ffmpeg_cmd::Options> > ffmpegCmd;
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_USD)
            std::shared_ptr<ftk::Observable<tl::usd::Options> > usd;
#endif // TLRENDER_USD
        };

        std::vector<Shortcut> getChangedShortcuts(
            const std::vector<Shortcut>& value,
            const std::vector<Shortcut>& defaults)
        {
            std::vector<Shortcut> out;
            for (const auto& shortcut : value)
            {
                const auto i = std::find_if(
                    defaults.begin(),
                    defaults.end(),
                    [&shortcut](const Shortcut& other)
                    {
                        return shortcut.name == other.name;
                    });
                if (i == defaults.end() ||
                    i->primary != shortcut.primary ||
                    i->secondary != shortcut.secondary)
                {
                    out.push_back(shortcut);
                }
            }
            return out;
        }

        std::vector<Shortcut> getBoundShortcuts(const std::vector<Shortcut>& value)
        {
            std::vector<Shortcut> out;
            for (const auto& shortcut : value)
            {
                if (shortcut.primary.key != ftk::Key::Unknown ||
                    shortcut.secondary.key != ftk::Key::Unknown)
                {
                    out.push_back(shortcut);
                }
            }
            return out;
        }

        namespace
        {
            std::map<std::string, std::string> keys =
            {
                { "Audio", "/Audio.1" },
                { "Cache", "/Cache" },
                { "ThumbnailCache", "/ThumbnailCache" },
                { "Export", "/Export" },
                { "FileBrowser", "/FileBrowser" },
                { "ImageSeq", "/ImageSeq.1" },
                { "OTIO", "/OTIO.2" },
                { "Shortcuts", "/Shortcuts.4" },
                { "Misc", "/Misc.2" },
                { "Mouse", "/Mouse.1" },
                { "Playback", "/Playback.1" },
                { "Style", "/Style.2" },
                { "Timeline", "/Timeline" },
                { "Window", "/Window" },
                { "FFmpeg", "/FFmpeg" },
                { "FFmpegCmd", "/FFmpegCmd" },
                { "USD", "/USD.1" },
            };
        }

        void SettingsModel::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            FTK_P();

            p.context = context;
            p.settings = settings;

            AudioSettings audio;
            settings->getT(keys["Audio"], audio);
            p.audio = ftk::Observable<AudioSettings>::create(audio);

            tl::PlayerCacheOptions cache;
            settings->getT(keys["Cache"], cache);
            p.cache = ftk::Observable<tl::PlayerCacheOptions>::create(cache);
            tl::ui::ThumbnailCacheOptions thumbnailCache;
            settings->getT(keys["ThumbnailCache"], thumbnailCache);
            p.thumbnailCache = ftk::Observable<tl::ui::ThumbnailCacheOptions>::create(thumbnailCache);
            auto thumbnailSystem = context->getSystem<tl::ui::ThumbnailSystem>();
            thumbnailSystem->setCacheOptions(thumbnailCache);

            ExportSettings exportSettings;
            settings->getT(keys["Export"], exportSettings);
            if (exportSettings.dir.empty())
            {
                // Show where an export would go rather than leaving the field
                // blank and writing somewhere the user cannot see. Home rather
                // than the current directory, which depends on how the
                // application was started and is the root of the filesystem
                // when it is launched from the Finder.
                exportSettings.dir =
                    ftk::fromFileSystem(ftk::getUserPath(ftk::UserPath::Home));
            }
            p.exportSettings = ftk::Observable<ExportSettings>::create(exportSettings);

            // The color picker opens on the tab the user last chose.
            {
                auto colorWidgetSystem =
                    context->getSystem<ftk::ColorWidgetSystem>();
                int mode = static_cast<int>(colorWidgetSystem->getMode());
                settings->get("/ColorWidget/Mode", mode);
                if (mode >= 0 &&
                    mode < static_cast<int>(ftk::ColorWidgetMode::Count))
                {
                    colorWidgetSystem->setMode(
                        static_cast<ftk::ColorWidgetMode>(mode));
                }
            }

            FileBrowserSettings fileBrowser;
            settings->getT(keys["FileBrowser"], fileBrowser);
            p.fileBrowser = ftk::Observable<FileBrowserSettings>::create(fileBrowser);
            auto fileBrowserSystem = context->getSystem<ftk::FileBrowserSystem>();
            fileBrowserSystem->setNativeFileDialog(fileBrowser.nativeFileDialog);
            fileBrowserSystem->setFloating(fileBrowser.floating);
            fileBrowserSystem->setPinned(fileBrowser.pinned);
            // Only once there is one to apply: an empty setting leaves the
            // file browser to open at the size it chooses. Not applied with
            // the settings below, which are the ones the settings tool
            // edits; this one is only ever changed by resizing the browser.
            if (fileBrowser.windowSize.isValid())
            {
                fileBrowserSystem->setWindowSize(fileBrowser.windowSize);
            }
            // toFileSystem because the setting is stored as UTF-8 -- see
            // the fromFileSystem it is written with below. The implicit
            // conversion
            // reads it as the Windows ANSI code page instead, which throws
            // for bytes that page cannot map, and this runs while settings
            // are loading: a browsed-to Korean directory would take the
            // application down on startup. Issue #779.
            const std::filesystem::path fileBrowserPath =
                ftk::toFileSystem(fileBrowser.path);
            if (std::filesystem::exists(fileBrowserPath))
            {
                fileBrowserSystem->getModel()->setPath(fileBrowserPath);
            }
            fileBrowserSystem->getModel()->setOptions(fileBrowser.options);
            fileBrowserSystem->getModel()->setExt(fileBrowser.ext);

            ImageSeqSettings imageSeq;
            settings->getT(keys["ImageSeq"], imageSeq);
            p.imageSeq = ftk::Observable<ImageSeqSettings>::create(imageSeq);

            OTIOSettings otio;
            settings->getT(keys["OTIO"], otio);
            p.otio = ftk::Observable<OTIOSettings>::create(otio);

            // The saved keyboard shortcuts are applied as the shortcuts are
            // registered with addShortcuts(). Only the ones changed from their
            // defaults are saved, so a default added or changed later reaches
            // everyone. The settings before that saved every shortcut, and a
            // shortcut saved there without a key is taken for the default it
            // predates rather than a key cleared on purpose: kept, it hid the
            // Ctrl+Z that Review/Undo was given after it was saved.
            ShortcutsSettings shortcutsSaved;
            if (settings->contains(keys["Shortcuts"]))
            {
                settings->getT(keys["Shortcuts"], shortcutsSaved);
            }
            else if (settings->getT("/Shortcuts.3", shortcutsSaved))
            {
                shortcutsSaved.shortcuts = getBoundShortcuts(shortcutsSaved.shortcuts);
            }
            for (const auto& shortcut : shortcutsSaved.shortcuts)
            {
                p.savedShortcuts[shortcut.name] = shortcut;
            }
            p.shortcuts = ftk::Observable<ShortcutsSettings>::create(ShortcutsSettings());

            MiscSettings misc;
            settings->getT(keys["Misc"], misc);
            p.misc = ftk::Observable<MiscSettings>::create(misc);

            MouseSettings mouse;
            settings->getT(keys["Mouse"], mouse);
            p.mouse = ftk::Observable<MouseSettings>::create(mouse);

            PlaybackSettings playback;
            settings->getT(keys["Playback"], playback);
            p.playback = ftk::Observable<PlaybackSettings>::create(playback);

            StyleSettings style;
            settings->getT(keys["Style"], style);
            p.style = ftk::Observable<StyleSettings>::create(style);

            TimelineSettings timeline;
            settings->getT(keys["Timeline"], timeline);
            p.timeline = ftk::Observable<TimelineSettings>::create(timeline);

            WindowSettings window;
            settings->getT(keys["Window"], window);
            p.window = ftk::Observable<WindowSettings>::create(window);

#if defined(TLRENDER_FFMPEG_PLUGIN)
            tl::ffmpeg::Options ffmpeg;
            settings->getT(keys["FFmpeg"], ffmpeg);
            p.ffmpeg = ftk::Observable<tl::ffmpeg::Options>::create(ffmpeg);
#endif // TLRENDER_FFMPEG_PLUGIN

#if defined(TLRENDER_FFMPEG_PLUGIN)
            tl::ffmpeg_cmd::Options ffmpegCmd;
            settings->getT(keys["FFmpegCmd"], ffmpegCmd);
            p.ffmpegCmd = ftk::Observable<tl::ffmpeg_cmd::Options>::create(ffmpegCmd);
#endif // TLRENDER_FFMPEG_PLUGIN

#if defined(TLRENDER_USD)
            tl::usd::Options usd;
            settings->getT(keys["USD"], usd);
            p.usd = ftk::Observable<tl::usd::Options>::create(usd);
#endif // TLRENDER_USD
        }

        SettingsModel::SettingsModel() :
            _p(new Private)
        {}

        SettingsModel::~SettingsModel()
        {
            save();
        }

        std::shared_ptr<SettingsModel> SettingsModel::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            auto out = std::shared_ptr<SettingsModel>(new SettingsModel);
            out->_init(context, settings);
            return out;
        }

        void SettingsModel::save()
        {
            FTK_P();

            p.settings->setT(keys["Audio"], p.audio->get());
            p.settings->setT(keys["Cache"], p.cache->get());
            p.settings->setT(keys["ThumbnailCache"], p.thumbnailCache->get());
            p.settings->setT(keys["Export"], p.exportSettings->get());

            FileBrowserSettings fileBrowser = p.fileBrowser->get();
            auto context = p.context.lock();
            auto fileBrowserSystem = context->getSystem<ftk::FileBrowserSystem>();
            fileBrowser.path = ftk::fromFileSystem(fileBrowserSystem->getModel()->getPath());
            fileBrowser.options = fileBrowserSystem->getModel()->getOptions();
            fileBrowser.ext = fileBrowserSystem->getModel()->getExt();
            // Changed from inside the browser, like the path above it.
            fileBrowser.pinned = fileBrowserSystem->isPinned();
            fileBrowser.windowSize = fileBrowserSystem->getWindowSize();
            p.settings->setT(keys["FileBrowser"], fileBrowser);

            p.settings->set(
                "/ColorWidget/Mode",
                static_cast<int>(
                    context->getSystem<ftk::ColorWidgetSystem>()->getMode()));

            p.settings->setT(keys["ImageSeq"], p.imageSeq->get());

            p.settings->setT(keys["OTIO"], p.otio->get());

            // Only the shortcuts changed from their defaults, so a default
            // added or changed later is not hidden by a saved copy of the old
            // one. Saved shortcuts that were not registered are preserved, for
            // example shortcuts for features that are only sometimes available.
            const auto& registered = p.shortcuts->get().shortcuts;
            ShortcutsSettings shortcuts;
            shortcuts.shortcuts = getChangedShortcuts(
                registered,
                p.shortcutsDefault.shortcuts);
            for (const auto& i : p.savedShortcuts)
            {
                const auto j = std::find_if(
                    registered.begin(),
                    registered.end(),
                    [i](const Shortcut& value)
                    {
                        return i.first == value.name;
                    });
                if (j == registered.end())
                {
                    shortcuts.shortcuts.push_back(i.second);
                }
            }
            p.settings->setT(keys["Shortcuts"], shortcuts);
            p.settings->setT(keys["Misc"], p.misc->get());
            p.settings->setT(keys["Mouse"], p.mouse->get());
            p.settings->setT(keys["Playback"], p.playback->get());
            p.settings->setT(keys["Style"], p.style->get());
            p.settings->setT(keys["Timeline"], p.timeline->get());
            p.settings->setT(keys["Window"], p.window->get());

#if defined(TLRENDER_FFMPEG_PLUGIN)
            p.settings->setT(keys["FFmpeg"], p.ffmpeg->get());
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_FFMPEG_PLUGIN)
            p.settings->setT(keys["FFmpegCmd"], p.ffmpegCmd->get());
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_USD)
            p.settings->setT(keys["USD"], p.usd->get());
#endif // TLRENDER_USD

            p.settings->save();
        }

        void SettingsModel::reset()
        {
            FTK_P();
            setAudio(AudioSettings());
            setCache(tl::PlayerCacheOptions());
            setThumbnailCache(tl::ui::ThumbnailCacheOptions());
            setExport(ExportSettings());
            setFileBrowser(FileBrowserSettings());
            setImageSeq(ImageSeqSettings());
            setOTIO(OTIOSettings());
            setShortcuts(p.shortcutsDefault);
            MiscSettings miscSettings;
            miscSettings.showSetup = false;
            setMisc(miscSettings);
            setMouse(MouseSettings());
            setPlayback(PlaybackSettings());
            setStyle(StyleSettings());
            setTimeline(TimelineSettings());
            setWindow(WindowSettings());
#if defined(TLRENDER_FFMPEG_PLUGIN)
            setFFmpeg(tl::ffmpeg::Options());
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_FFMPEG_PLUGIN)
            setFFmpegCmd(tl::ffmpeg_cmd::Options());
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_USD)
            setUSD(tl::usd::Options());
#endif // TLRENDER_USD
        }

        const AudioSettings& SettingsModel::getAudio() const
        {
            return _p->audio->get();
        }

        std::shared_ptr<ftk::IObservable<AudioSettings> > SettingsModel::observeAudio() const
        {
            return _p->audio;
        }

        void SettingsModel::setAudio(const AudioSettings& value)
        {
            _p->audio->setIfChanged(value);
        }

        const tl::PlayerCacheOptions& SettingsModel::getCache() const
        {
            return _p->cache->get();
        }

        std::shared_ptr<ftk::IObservable<tl::PlayerCacheOptions> > SettingsModel::observeCache() const
        {
            return _p->cache;
        }

        void SettingsModel::setCache(const tl::PlayerCacheOptions& value)
        {
            _p->cache->setIfChanged(value);
        }

        const tl::ui::ThumbnailCacheOptions& SettingsModel::getThumbnailCache() const
        {
            return _p->thumbnailCache->get();
        }

        std::shared_ptr<ftk::IObservable<tl::ui::ThumbnailCacheOptions> > SettingsModel::observeThumbnailCache() const
        {
            return _p->thumbnailCache;
        }

        void SettingsModel::setThumbnailCache(const tl::ui::ThumbnailCacheOptions& value)
        {
            FTK_P();
            if (p.thumbnailCache->setIfChanged(value))
            {
                auto context = p.context.lock();
                auto thumbnailSystem = context->getSystem<tl::ui::ThumbnailSystem>();
                thumbnailSystem->setCacheOptions(value);
            }
        }

        const ExportSettings& SettingsModel::getExport() const
        {
            return _p->exportSettings->get();
        }

        std::shared_ptr<ftk::IObservable<ExportSettings> > SettingsModel::observeExport() const
        {
            return _p->exportSettings;
        }

        void SettingsModel::setExport(const ExportSettings& value)
        {
            _p->exportSettings->setIfChanged(value);
        }

        const FileBrowserSettings& SettingsModel::getFileBrowser() const
        {
            return _p->fileBrowser->get();
        }

        std::shared_ptr<ftk::IObservable<FileBrowserSettings> > SettingsModel::observeFileBrowser() const
        {
            return _p->fileBrowser;
        }

        void SettingsModel::setFileBrowser(const FileBrowserSettings& value)
        {
            FTK_P();
            if (p.fileBrowser->setIfChanged(value))
            {
                if (auto context = p.context.lock())
                {
                    auto fileBrowserSystem = context->getSystem<ftk::FileBrowserSystem>();
                    fileBrowserSystem->setNativeFileDialog(value.nativeFileDialog);
                    fileBrowserSystem->setFloating(value.floating);
                    fileBrowserSystem->setPinned(value.pinned);
                }
            }
        }

        const ImageSeqSettings& SettingsModel::getImageSeq() const
        {
            return _p->imageSeq->get();
        }

        std::shared_ptr<ftk::IObservable<ImageSeqSettings> > SettingsModel::observeImageSeq() const
        {
            return _p->imageSeq;
        }

        void SettingsModel::setImageSeq(const ImageSeqSettings& value)
        {
            _p->imageSeq->setIfChanged(value);
        }

        const OTIOSettings& SettingsModel::getOTIO() const
        {
            return _p->otio->get();
        }

        std::shared_ptr<ftk::IObservable<OTIOSettings> > SettingsModel::observeOTIO() const
        {
            return _p->otio;
        }

        void SettingsModel::setOTIO(const OTIOSettings& value)
        {
            _p->otio->setIfChanged(value);
        }

        const ShortcutsSettings& SettingsModel::getShortcuts() const
        {
            return _p->shortcuts->get();
        }

        std::shared_ptr<ftk::IObservable<ShortcutsSettings> > SettingsModel::observeShortcuts() const
        {
            return _p->shortcuts;
        }

        void SettingsModel::setShortcuts(const ShortcutsSettings& value)
        {
            _p->shortcuts->setIfChanged(value);
        }

        void SettingsModel::addShortcuts(const std::vector<Shortcut>& value)
        {
            FTK_P();
            ShortcutsSettings shortcuts = p.shortcuts->get();
            for (Shortcut shortcut : value)
            {
                // Keep the declared defaults for reset().
                p.shortcutsDefault.shortcuts.push_back(shortcut);

                // Apply any saved key bindings.
                if (const auto i = p.savedShortcuts.find(shortcut.name);
                    i != p.savedShortcuts.end())
                {
                    shortcut.primary = i->second.primary;
                    shortcut.secondary = i->second.secondary;
                }
                shortcuts.shortcuts.push_back(shortcut);
            }
            p.shortcuts->setIfChanged(shortcuts);
        }

        const MiscSettings& SettingsModel::getMisc() const
        {
            return _p->misc->get();
        }

        std::shared_ptr<ftk::IObservable<MiscSettings> > SettingsModel::observeMisc() const
        {
            return _p->misc;
        }

        void SettingsModel::setMisc(const MiscSettings& value)
        {
            _p->misc->setIfChanged(value);
        }

        const MouseSettings& SettingsModel::getMouse() const
        {
            return _p->mouse->get();
        }

        std::shared_ptr<ftk::IObservable<MouseSettings> > SettingsModel::observeMouse() const
        {
            return _p->mouse;
        }

        void SettingsModel::setMouse(const MouseSettings& value)
        {
            _p->mouse->setIfChanged(value);
        }

        const PlaybackSettings& SettingsModel::getPlayback() const
        {
            return _p->playback->get();
        }

        std::shared_ptr<ftk::IObservable<PlaybackSettings> > SettingsModel::observePlayback() const
        {
            return _p->playback;
        }

        void SettingsModel::setPlayback(const PlaybackSettings& value)
        {
            _p->playback->setIfChanged(value);
        }

        const StyleSettings& SettingsModel::getStyle() const
        {
            return _p->style->get();
        }

        std::shared_ptr<ftk::IObservable<StyleSettings> > SettingsModel::observeStyle() const
        {
            return _p->style;
        }

        void SettingsModel::setStyle(const StyleSettings& value)
        {
            _p->style->setIfChanged(value);
        }

        const TimelineSettings& SettingsModel::getTimeline() const
        {
            return _p->timeline->get();
        }

        std::shared_ptr<ftk::IObservable<TimelineSettings> > SettingsModel::observeTimeline() const
        {
            return _p->timeline;
        }

        void SettingsModel::setTimeline(const TimelineSettings& value)
        {
            _p->timeline->setIfChanged(value);
        }

        const WindowSettings& SettingsModel::getWindow() const
        {
            return _p->window->get();
        }

        std::shared_ptr<ftk::IObservable<WindowSettings> > SettingsModel::observeWindow() const
        {
            return _p->window;
        }

        void SettingsModel::setWindow(const WindowSettings& value)
        {
            _p->window->setIfChanged(value);
        }

#if defined(TLRENDER_FFMPEG_PLUGIN)
        const tl::ffmpeg::Options& SettingsModel::getFFmpeg() const
        {
            return _p->ffmpeg->get();
        }

        std::shared_ptr<ftk::IObservable<tl::ffmpeg::Options> > SettingsModel::observeFFmpeg() const
        {
            return _p->ffmpeg;
        }

        void SettingsModel::setFFmpeg(const tl::ffmpeg::Options& value)
        {
            _p->ffmpeg->setIfChanged(value);
        }
#endif // TLRENDER_FFMPEG_PLUGIN

#if defined(TLRENDER_FFMPEG_PLUGIN)
        const tl::ffmpeg_cmd::Options& SettingsModel::getFFmpegCmd() const
        {
            return _p->ffmpegCmd->get();
        }

        std::shared_ptr<ftk::IObservable<tl::ffmpeg_cmd::Options> > SettingsModel::observeFFmpegCmd() const
        {
            return _p->ffmpegCmd;
        }

        void SettingsModel::setFFmpegCmd(const tl::ffmpeg_cmd::Options& value)
        {
            _p->ffmpegCmd->setIfChanged(value);
        }
#endif // TLRENDER_FFMPEG_PLUGIN

#if defined(TLRENDER_USD)
        const tl::usd::Options& SettingsModel::getUSD() const
        {
            return _p->usd->get();
        }

        std::shared_ptr<ftk::IObservable<tl::usd::Options> > SettingsModel::observeUSD() const
        {
            return _p->usd;
        }

        void SettingsModel::setUSD(const tl::usd::Options& value)
        {
            _p->usd->setIfChanged(value);
        }
#endif // TLRENDER_USD

        tl::IOOptions SettingsModel::getIOOptions() const
        {
            FTK_P();
            tl::IOOptions out;
            out = tl::merge(out, tl::getOptions(p.imageSeq->get().io));
#if defined(TLRENDER_FFMPEG_PLUGIN)
            out = tl::merge(out, tl::ffmpeg::getOptions(p.ffmpeg->get()));
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_FFMPEG_PLUGIN)
            out = tl::merge(out, p.ffmpegCmd->get().getIOOptions());
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_USD)
            out = tl::merge(out, tl::usd::getOptions(p.usd->get()));
#endif // TLRENDER_USD
            return out;
        }

        void to_json(nlohmann::json& json, const AudioSettings& value)
        {
            json["BufferFrameCount"] = value.bufferFrameCount;
        }

        // Whether the number in a file name is a run of '#'. Only that is
        // replaced by the frame number: the path reads trailing digits as
        // a frame number too, and replacing those would write
        // "shot_v002.exr" as "shot_v023.exr".
        bool isFrameTemplate(const ftk::Path& path)
        {
            const std::string num = path.getNum();
            return !num.empty() &&
                std::string::npos == num.find_first_not_of('#');
        }

        // A name typed or pasted with one of the extensions on it takes
        // that extension, rather than being written as "shot.exr.tif".
        bool splitExt(
            std::string& name,
            std::string& ext,
            const std::vector<std::string>& exts)
        {
            const std::string lower = ftk::toLower(name);
            for (const auto& i : exts)
            {
                if (lower.size() > i.size() &&
                    0 == lower.compare(lower.size() - i.size(), i.size(), i))
                {
                    name.resize(name.size() - i.size());
                    ext = i;
                    return true;
                }
            }
            return false;
        }

        // What is wrong with an export file name, or nothing.
        std::string getFileNameError(
            const std::string& fileName,
            const std::string& ext,
            ExportFileType fileType,
            const std::vector<std::string>& exts)
        {
            std::string out;
            const ftk::Path path(fileName + ext);
            if (fileName.empty())
            {
                out = "No file name";
            }
            else if (path.hasDir())
            {
                out = "No directory; that is set above";
            }
            else if (std::find(exts.begin(), exts.end(), ext) == exts.end())
            {
                out = "No extension";
            }
            else if (ExportFileType::Seq == fileType &&
                !isFrameTemplate(path))
            {
                out = "Needs # where the frame number goes";
            }
            else if (ExportFileType::Movie == fileType &&
                isFrameTemplate(path))
            {
                out = "A movie has no frame number";
            }
            return out;
        }

        void getExportNames(
            const ftk::Path& path,
            const std::vector<std::string>& seqExts,
            std::string& name,
            std::string& seqName)
        {
            const std::string base = path.getBase();
            const std::string num = path.getNum();
            // Trailing digits are a frame number only in an image
            // sequence. A path that names one file carries the frame
            // parsed out of its name whatever the file is, so it is the
            // extension that says which this is: "Wide_169.mov" is a
            // movie whose name ends in digits, and is written as
            // "Wide_169".
            if (!(path.hasNum() && path.testExt(seqExts)))
            {
                name = base + num;
                seqName = name + ".####";
                return;
            }
            // Without the separator: it belongs to the frame number that
            // a single file does not have.
            name = base;
            if (!name.empty() &&
                !std::isalnum(static_cast<unsigned char>(name.back())))
            {
                name.resize(name.size() - 1);
            }
            // From the padding rather than the length of the number: a
            // negative frame carries its sign in the number.
            size_t digits = 0;
            for (char c : num)
            {
                if (std::isdigit(static_cast<unsigned char>(c)))
                {
                    ++digits;
                }
            }
            const int pad = path.getPad();
            seqName = base + std::string(
                std::max<size_t>(pad > 0 ? pad : digits, 1), '#');
        }

        void to_json(nlohmann::json& json, const ExportSettings& value)
        {
            json["Dir"] = value.dir;
            json["RenderSize"] = to_string(value.renderSize);
            json["CustomWidth"] = value.customWidth;
            json["FileType"] = to_string(value.fileType);
            // The names are not kept: an output is named for the file being
            // exported, which the export widget works out each time. The
            // extensions are kept, being a choice about how to write rather
            // than about what.
            json["ImageExt"] = value.imageExt;
            json["MovieExt"] = value.movieExt;
            json["MoviePreset"] = value.moviePreset;
            json["MovieAudioCodec"] = value.movieAudioCodec;
            json["SeqExt"] = value.seqExt;
        }

        void to_json(nlohmann::json& json, const FileBrowserSettings& value)
        {
            json["NativeFileDialog"] = value.nativeFileDialog;
            json["Floating"] = value.floating;
            json["Pinned"] = value.pinned;
            json["WindowSize"] = value.windowSize;
            json["Path"] = value.path;
            json["Options"] = value.options;
            json["Ext"] = value.ext;
        }

        void to_json(nlohmann::json& json, const ImageSeqSettings& value)
        {
            json["Audio"] = tl::to_string(value.audio);
            json["AudioExts"] = value.audioExts;
            json["AudioFileName"] = value.audioFileName;
            json["MaxDigits"] = value.maxDigits;
            json["ReadThreadCount"] = value.readThreadCount;
            json["IO"] = value.io;
        }

        void to_json(nlohmann::json& json, const OTIOSettings& value)
        {
            json["Spatial"] = tl::to_string(value.spatial);
            json["Compat"] = value.compat;
        }

        void to_json(nlohmann::json& json, const ShortcutsSettings& value)
        {
            for (const auto& shortcut : value.shortcuts)
            {
                json["Shortcuts"].push_back(shortcut);
            }
        }

        void to_json(nlohmann::json& json, const MiscSettings& value)
        {
            json["TooltipsEnabled"] = value.tooltipsEnabled;
            json["ShowSetup"] = value.showSetup;
        }

        void to_json(nlohmann::json& json, const MouseActionBinding& value)
        {
            json["Button"] = to_string(value.button);
            json["Modifier"] = to_string(value.modifier);
        }

        void to_json(nlohmann::json& json, const MouseSettings& value)
        {
            for (const auto& i : value.bindings)
            {
                to_json(json["Bindings"][to_string(i.first)], i.second);
            }
            for (const auto& i : value.wheelBindings)
            {
                json["WheelBindings"][to_string(i.first)] = to_string(i.second);
            }
            json["WheelScale"] = value.wheelScale;
            json["FrameShuttleScale"] = value.frameShuttleScale;
        }

        void to_json(nlohmann::json& json, const PlaybackSettings& value)
        {
            json["StartPlayback"] = value.startPlayback;
        }

        void to_json(nlohmann::json& json, const StyleSettings& value)
        {
            json["DisplayScale"] = value.displayScale;
            json["ColorControls"] = value.colorControls;
            json["ColorStyle"] = to_string(value.colorStyle);
            for (auto i : value.fonts)
            {
                json["Fonts"][ftk::getLabel(i.first)] = i.second;
            }
            json["FontFiles"] = value.fontFiles;
        }

        void to_json(nlohmann::json& json, const TimelineSettings& value)
        {
            json["Minimize"] = value.minimize;
            json["FrameView"] = value.frameView;
            json["ScrollBars"] = value.scrollBars;
            json["AutoScroll"] = value.autoScroll;
            json["StopOnScrub"] = value.stopOnScrub;
            json["Preview"] = value.preview;
            json["TrackMedia"] = value.trackMedia;
            json["Thumbnails"] = value.thumbnails;
            json["ThumbnailSize"] = to_string(value.thumbnailSize);
            json["WaveformSize"] = to_string(value.waveformSize);
            json["Waveforms"] = value.waveforms;
        }

        void to_json(nlohmann::json& json, const WindowSettings& in)
        {
            json = nlohmann::json
            {
                { "Size", in.size },
                { "FileToolBar", in.fileToolBar },
                { "CompareToolBar", in.compareToolBar },
                { "WindowToolBar", in.windowToolBar },
                { "ViewToolBar", in.viewToolBar },
                { "ToolsToolBar", in.toolsToolBar },
                { "TabBar", in.tabBar },
                { "Timeline", in.timeline },
                { "BottomToolBar", in.bottomToolBar },
                { "StatusToolBar", in.statusToolBar },
                { "Tools", in.tools },
                { "Splitter", in.splitter },
                { "Splitter2", in.splitter2 }
            };
        }

        void from_json(const nlohmann::json& json, AudioSettings& value)
        {
            json.at("BufferFrameCount").get_to(value.bufferFrameCount);
        }

        void from_json(const nlohmann::json& json, ExportSettings& value)
        {
            json.at("Dir").get_to(value.dir);
            const std::string renderSize = json.at("RenderSize").get<std::string>();
            if (!from_string(renderSize, value.renderSize))
            {
                // The presets became widths, so a settings file written
                // before that names a size no label matches. Unmatched would
                // silently drop to the default; carry the width over instead.
                const std::map<std::string, ExportRenderSize> old =
                {
                    { "1920x1080", ExportRenderSize::_1920 },
                    { "3840x2160", ExportRenderSize::_3840 },
                    { "4096x2160", ExportRenderSize::_4096 }
                };
                const auto i = old.find(renderSize);
                if (i != old.end())
                {
                    value.renderSize = i->second;
                }
            }
            // Neither key is read with at(): a settings file written before
            // the custom size became a width alone has "CustomSize" instead,
            // and throwing on the missing key would take every other export
            // setting with it. The width carries over from the old pair.
            if (json.contains("CustomWidth"))
            {
                json.at("CustomWidth").get_to(value.customWidth);
            }
            else if (json.contains("CustomSize"))
            {
                ftk::Size2I customSize;
                json.at("CustomSize").get_to(customSize);
                value.customWidth = customSize.w;
            }
            from_string(json.at("FileType").get<std::string>(), value.fileType);
            if (json.contains("ImageExt"))
            {
                json.at("ImageExt").get_to(value.imageExt);
            }
            if (json.contains("SeqExt"))
            {
                json.at("SeqExt").get_to(value.seqExt);
            }
            if (json.contains("MovieExt"))
            {
                json.at("MovieExt").get_to(value.movieExt);
            }
            if (json.contains("MoviePreset"))
            {
                json.at("MoviePreset").get_to(value.moviePreset);
            }
            json.at("MovieAudioCodec").get_to(value.movieAudioCodec);
        }

        void from_json(const nlohmann::json& json, FileBrowserSettings& value)
        {
            json.at("NativeFileDialog").get_to(value.nativeFileDialog);
            // Looked for rather than required: at() throws on a key that is
            // not there, and the caller answers that by dropping the whole
            // group back to its defaults. A settings file written before this
            // was added would lose the path and options with it.
            if (const auto i = json.find("Floating"); i != json.end())
            {
                i->get_to(value.floating);
            }
            if (const auto i = json.find("Pinned"); i != json.end())
            {
                i->get_to(value.pinned);
            }
            if (const auto i = json.find("WindowSize"); i != json.end())
            {
                i->get_to(value.windowSize);
            }
            json.at("Path").get_to(value.path);
            json.at("Options").get_to(value.options);
            json.at("Ext").get_to(value.ext);
        }

        void from_json(const nlohmann::json& json, ImageSeqSettings& value)
        {
            tl::from_string(json.at("Audio").get<std::string>(), value.audio);
            json.at("AudioExts").get_to(value.audioExts);
            json.at("AudioFileName").get_to(value.audioFileName);
            json.at("MaxDigits").get_to(value.maxDigits);
            json.at("ReadThreadCount").get_to(value.readThreadCount);
            json.at("IO").get_to(value.io);
        }

        void from_json(const nlohmann::json& json, OTIOSettings& value)
        {
            tl::from_string(json.at("Spatial").get<std::string>(), value.spatial);
            json.at("Compat").get_to(value.compat);
        }

        void from_json(const nlohmann::json& json, MiscSettings& value)
        {
            json.at("TooltipsEnabled").get_to(value.tooltipsEnabled);
            json.at("ShowSetup").get_to(value.showSetup);
        }

        void from_json(const nlohmann::json& json, MouseActionBinding& value)
        {
            from_string(json.at("Button").get<std::string>(), value.button);
            from_string(json.at("Modifier").get<std::string>(), value.modifier);
        }

        void from_json(const nlohmann::json& json, MouseSettings& value)
        {
            for (auto i = json.at("Bindings").begin(); i != json.at("Bindings").end(); ++i)
            {
                MouseAction mouseAction = MouseAction::First;
                from_string(i.key(), mouseAction);
                from_json(i.value(), value.bindings[mouseAction]);
            }
            // Not required: settings written before the wheel bindings
            // existed still load.
            if (json.contains("WheelBindings"))
            {
                for (auto i = json.at("WheelBindings").begin();
                    i != json.at("WheelBindings").end();
                    ++i)
                {
                    WheelAction wheelAction = WheelAction::First;
                    from_string(i.key(), wheelAction);
                    ftk::KeyModifier modifier = ftk::KeyModifier::None;
                    from_string(i.value().get<std::string>(), modifier);
                    value.wheelBindings[wheelAction] = modifier;
                }
            }
            json.at("WheelScale").get_to(value.wheelScale);
            json.at("FrameShuttleScale").get_to(value.frameShuttleScale);
        }

        void from_json(const nlohmann::json& json, PlaybackSettings& value)
        {
            json.at("StartPlayback").get_to(value.startPlayback);
        }

        void from_json(const nlohmann::json& json, ShortcutsSettings& value)
        {
            for (auto i = json.at("Shortcuts").begin(); i != json.at("Shortcuts").end(); ++i)
            {
                const Shortcut shortcut = i->get<Shortcut>();
                const auto j = std::find_if(
                    value.shortcuts.begin(),
                    value.shortcuts.end(),
                    [shortcut](const Shortcut& value)
                    {
                        return shortcut.name == value.name;
                    });
                if (j != value.shortcuts.end())
                {
                    *j = shortcut;
                }
                else
                {
                    value.shortcuts.push_back(shortcut);
                }
            }
        }

        void from_json(const nlohmann::json& json, StyleSettings& value)
        {
            json.at("DisplayScale").get_to(value.displayScale);
            // A style that is no longer offered, such as the Custom one
            // there used to be, is left at the default.
            from_string(json.at("ColorStyle").get<std::string>(), value.colorStyle);
            json.at("ColorControls").get_to(value.colorControls);
            for (auto i = json.at("Fonts").begin(); i != json.at("Fonts").end(); ++i)
            {
                ftk::FontType font = ftk::FontType::Regular;
                from_string(i.key(), font);
                i.value().get_to(value.fonts[font]);
            }
            value.fontFiles.clear();
            for (auto i = json.at("FontFiles").begin(); i != json.at("FontFiles").end(); ++i)
            {
                value.fontFiles.push_back(*i);
            }
        }

        void from_json(const nlohmann::json& json, TimelineSettings& value)
        {
            json.at("Minimize").get_to(value.minimize);
            json.at("FrameView").get_to(value.frameView);
            json.at("ScrollBars").get_to(value.scrollBars);
            json.at("AutoScroll").get_to(value.autoScroll);
            json.at("StopOnScrub").get_to(value.stopOnScrub);
            if (json.contains("Preview"))
            {
                json.at("Preview").get_to(value.preview);
            }
            // Asked for rather than required, so settings written before this
            // still load the rest of the timeline.
            if (json.contains("TrackMedia"))
            {
                json.at("TrackMedia").get_to(value.trackMedia);
            }
            json.at("Thumbnails").get_to(value.thumbnails);
            from_string(json.at("ThumbnailSize").get<std::string>(), value.thumbnailSize);
            from_string(json.at("WaveformSize").get<std::string>(), value.waveformSize);
            json.at("Waveforms").get_to(value.waveforms);
        }

        void from_json(const nlohmann::json& json, WindowSettings& value)
        {
            json.at("Size").get_to(value.size);
            json.at("FileToolBar").get_to(value.fileToolBar);
            json.at("CompareToolBar").get_to(value.compareToolBar);
            json.at("WindowToolBar").get_to(value.windowToolBar);
            json.at("ViewToolBar").get_to(value.viewToolBar);
            json.at("ToolsToolBar").get_to(value.toolsToolBar);
            json.at("TabBar").get_to(value.tabBar);
            json.at("Timeline").get_to(value.timeline);
            json.at("BottomToolBar").get_to(value.bottomToolBar);
            json.at("StatusToolBar").get_to(value.statusToolBar);
            // Asked for rather than required, so that settings written before
            // the panel could be hidden still load the rest of the window
            // instead of falling back to the defaults for all of it.
            if (json.contains("Tools"))
            {
                json.at("Tools").get_to(value.tools);
            }
            json.at("Splitter").get_to(value.splitter);
            json.at("Splitter2").get_to(value.splitter2);
        }
    }
}
