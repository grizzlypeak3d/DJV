// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Models/SettingsModel.h>

#include <ftk/UI/Settings.h>
#include <ftk/Core/Error.h>
#include <ftk/Core/String.h>

#include <sstream>

namespace djv
{
    namespace app
    {
        bool AdvancedSettings::operator == (const AdvancedSettings& other) const
        {
            return
                compat == other.compat &&
                audioBufferFrameCount == other.audioBufferFrameCount &&
                videoRequestMax == other.videoRequestMax &&
                audioRequestMax == other.audioRequestMax;
        }

        bool AdvancedSettings::operator != (const AdvancedSettings& other) const
        {
            return !(*this == other);
        }

        FTK_ENUM_IMPL(
            ExportRenderSize,
            "Default",
            "1920x1080",
            "3840x2160",
            "4096x2160",
            "Custom");

        ftk::Size2I getSize(ExportRenderSize value)
        {
            const std::array<ftk::Size2I, static_cast<size_t>(ExportRenderSize::Count)> data =
            {
                ftk::Size2I(),
                ftk::Size2I(1920, 1080),
                ftk::Size2I(3840, 2160),
                ftk::Size2I(4096, 2160),
                ftk::Size2I()
            };
            return data[static_cast<size_t>(value)];
        }

        FTK_ENUM_IMPL(
            ExportFileType,
            "Image",
            "Seq",
            "Movie");

        bool ExportSettings::operator == (const ExportSettings& other) const
        {
            return
                dir == other.dir &&
                renderSize == other.renderSize &&
                customSize == other.customSize &&
                fileType == other.fileType &&
                imageBase == other.imageBase &&
                imageZeroPad == other.imageZeroPad &&
                imageExt == other.imageExt &&
                movieBase == other.movieBase &&
                movieExt == other.movieExt &&
                movieCodec == other.movieCodec;
        }

        bool ExportSettings::operator != (const ExportSettings& other) const
        {
            return !(*this == other);
        }

        bool FileBrowserSettings::operator == (const FileBrowserSettings& other) const
        {
            return
                nativeFileDialog == other.nativeFileDialog &&
                path == other.path &&
                options == other.options &&
                ext == other.ext;
        }

        bool FileBrowserSettings::operator != (const FileBrowserSettings& other) const
        {
            return !(*this == other);
        }

        bool ImageSeqSettings::operator == (const ImageSeqSettings& other) const
        {
            return
                audio == other.audio &&
                audioExts == other.audioExts &&
                audioFileName == other.audioFileName &&
                maxDigits == other.maxDigits &&
                io == other.io;
        }

        bool ImageSeqSettings::operator != (const ImageSeqSettings& other) const
        {
            return !(*this == other);
        }

        ShortcutsSettings::ShortcutsSettings()
        {
            shortcuts =
            {
                Shortcut("Audio/VolumeUp", "Volume up", ftk::Key::Period),
                Shortcut("Audio/VolumeDown", "Volume down", ftk::Key::Comma),
                Shortcut("Audio/Mute", "Mute", ftk::Key::M),

                Shortcut("Compare/Next", "Next", ftk::KeyShortcut(ftk::Key::PageDown, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Compare/Prev", "Previous", ftk::KeyShortcut(ftk::Key::PageUp, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Compare/A", "A", ftk::KeyShortcut(ftk::Key::A, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Compare/B", "B", ftk::KeyShortcut(ftk::Key::B, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Compare/ABToggle", "A/B Toggle", ftk::KeyShortcut(ftk::Key::A, static_cast<int>(ftk::KeyModifier::Alt))),
                Shortcut("Compare/Wipe", "Wipe", ftk::KeyShortcut(ftk::Key::W, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Compare/Overlay", "Overlay"),
                Shortcut("Compare/Difference", "Difference"),
                Shortcut("Compare/Horizontal", "Horizontal"),
                Shortcut("Compare/Vertical", "Vertical"),
                Shortcut("Compare/Tile", "Tile", ftk::KeyShortcut(ftk::Key::T, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Compare/Relative", "Relative"),
                Shortcut("Compare/Absolute", "Absolute"),

                Shortcut("File/Open", "Open", ftk::KeyShortcut(ftk::Key::O, static_cast<int>(ftk::commandKeyModifier))),
                Shortcut(
                    "File/OpenAudio",
                    "Open with audio",
                    ftk::KeyShortcut(
                        ftk::Key::O,
                        static_cast<int>(ftk::KeyModifier::Shift) |
                        static_cast<int>(ftk::commandKeyModifier))),
                Shortcut("File/Close", "Close", ftk::KeyShortcut(ftk::Key::E, static_cast<int>(ftk::commandKeyModifier))),
                Shortcut(
                    "File/CloseAll",
                    "Close all",
                    ftk::KeyShortcut(
                        ftk::Key::E,
                        static_cast<int>(ftk::KeyModifier::Shift) | static_cast<int>(ftk::commandKeyModifier))),
                Shortcut(
                    "File/Reload",
                    "Reload",
                    ftk::KeyShortcut(
                        ftk::Key::R,
                        static_cast<int>(ftk::KeyModifier::Shift) | static_cast<int>(ftk::commandKeyModifier))),
                Shortcut("File/Next", "Next", ftk::KeyShortcut(ftk::Key::PageDown, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("File/Prev", "Previous", ftk::KeyShortcut(ftk::Key::PageUp, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("File/NextLayer", "Next layer", ftk::KeyShortcut(ftk::Key::Equals, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("File/PrevLayer", "Previous layer", ftk::KeyShortcut(ftk::Key::Minus, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("File/Exit", "Exit", ftk::KeyShortcut(ftk::Key::Q, static_cast<int>(ftk::commandKeyModifier))),

                Shortcut("Frame/Start", "Start", ftk::Key::Home),
                Shortcut("Frame/End", "End", ftk::Key::End),
                Shortcut("Frame/Prev", "Previous", ftk::Key::Left),
                Shortcut("Frame/PrevX10", "Previous X10", ftk::KeyShortcut(ftk::Key::Left, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Frame/PrevX100", "Previous X100", ftk::KeyShortcut(ftk::Key::Left, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Frame/Next", "Next", ftk::Key::Right),
                Shortcut("Frame/NextX10", "Next X10", ftk::KeyShortcut(ftk::Key::Right, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Frame/NextX100", "Next X100", ftk::KeyShortcut(ftk::Key::Right, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Frame/FocusCurrent", "Focus current", ftk::KeyShortcut(ftk::Key::F, static_cast<int>(ftk::KeyModifier::Control))),

                Shortcut("Playback/Stop", "Stop", ftk::Key::K),
                Shortcut("Playback/Forward", "Forward", ftk::Key::L),
                Shortcut("Playback/Reverse", "Reverse", ftk::Key::J),
                Shortcut("Playback/Toggle", "Toggle", ftk::Key::Space),
                Shortcut("Playback/JumpBack1s", "Jump back 1s", ftk::KeyShortcut(ftk::Key::J, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Playback/JumpBack10s", "Jump back 10s", ftk::KeyShortcut(ftk::Key::J, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Playback/JumpForward1s", "Jump forward 1s", ftk::KeyShortcut(ftk::Key::L, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Playback/JumpForward10s", "Jump forward 10s", ftk::KeyShortcut(ftk::Key::L, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Playback/Loop", "Loop"),
                Shortcut("Playback/Once", "Once"),
                Shortcut("Playback/PingPong", "Ping pong"),
                Shortcut("Playback/SetInPoint", "Set in point", ftk::Key::I),
                Shortcut("Playback/ResetInPoint", "Reset in point", ftk::KeyShortcut(ftk::Key::I, static_cast<int>(ftk::KeyModifier::Shift))),
                Shortcut("Playback/SetOutPoint", "Set out point", ftk::Key::O),
                Shortcut("Playback/ResetOutPoint", "Reset out point", ftk::KeyShortcut(ftk::Key::O, static_cast<int>(ftk::KeyModifier::Shift))),

                Shortcut("Timeline/FrameView", "Frame view"),
                Shortcut("Timeline/Scroll", "Scroll"),
                Shortcut("Timeline/StopOnScrub", "Stop on scrub"),
                Shortcut("Timeline/Thumbnails", "Thumbnails"),
                Shortcut("Timeline/ThumbnailsSmall", "Thumbnails small"),
                Shortcut("Timeline/ThumbnailsMedium", "Thumbnails medium"),
                Shortcut("Timeline/ThumbnailsLarge", "Thumbnails large"),

                Shortcut("Tools/Files", "Files", ftk::Key::F1),
                Shortcut("Tools/Export", "Export", ftk::Key::F2),
                Shortcut("Tools/View", "View", ftk::Key::F3),
                Shortcut("Tools/Color", "Color", ftk::Key::F4),
                Shortcut("Tools/Color Picker", "Color picker", ftk::Key::F5),
                Shortcut("Tools/Magnify", "Magnify", ftk::Key::F6),
                Shortcut("Tools/Information", "Information", ftk::Key::F7),
                Shortcut("Tools/Audio", "Audio", ftk::Key::F8),
                Shortcut("Tools/Devices", "Devices", ftk::Key::F9),
                Shortcut("Tools/Settings", "Settings", ftk::Key::F10),
                Shortcut("Tools/Messages", "Messages", ftk::Key::F11),
                Shortcut("Tools/System Log", "System log", ftk::Key::F12),
                Shortcut("Tools/Diagnostics", "Diagnostics"),

                Shortcut("View/Frame", "Frame", ftk::Key::Backspace),
                Shortcut("View/ZoomReset", "Zoom reset", ftk::Key::_0),
                Shortcut("View/ZoomIn", "Zoom in", ftk::Key::Equals),
                Shortcut("View/ZoomOut", "Zoom out", ftk::Key::Minus),
                Shortcut("View/Red", "Red channel", ftk::Key::R),
                Shortcut("View/Green", "Green channel", ftk::Key::G),
                Shortcut("View/Blue", "Blue channel", ftk::Key::B),
                Shortcut("View/Alpha", "Alpha channel", ftk::Key::A),
                Shortcut("View/MirrorHorizontal", "Mirror horizontal", ftk::Key::H),
                Shortcut("View/MirrorVertical", "Mirror vertical", ftk::Key::V),
                Shortcut("View/MinifyNearest", "Minify nearest"),
                Shortcut("View/MinifyLinear", "Minify linear"),
                Shortcut("View/MagnifyNearest", "Magnify nearest"),
                Shortcut("View/MagnifyLinear", "Magnify linear"),
                Shortcut("View/FromFile", "File video range"),
                Shortcut("View/FullRange", "Full video range"),
                Shortcut("View/LegalRange", "Legal video range"),
                Shortcut("View/AlphaBlendNone", "No alpha blending"),
                Shortcut("View/AlphaBlendStraight", "Straight alpha"),
                Shortcut("View/AlphaBlendPremultiplied", "Pre-multiplied alpha"),
                Shortcut("View/Grid", "Grid", ftk::KeyShortcut(ftk::Key::G, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("View/HUD", "HUD / Information Display", ftk::KeyShortcut(ftk::Key::H, static_cast<int>(ftk::KeyModifier::Control))),

                Shortcut("Window/FullScreen", "Full screen", ftk::Key::U),
                Shortcut("Window/FloatOnTop", "Float on top"),
                Shortcut("Window/Secondary", "Secondary", ftk::Key::Y),
                Shortcut("Window/FileToolBar", "File tool bar"),
                Shortcut("Window/CompareToolBar", "Compare tool bar"),
                Shortcut("Window/WindowToolBar", "Window tool bar"),
                Shortcut("Window/ViewToolBar", "View tool bar"),
                Shortcut("Window/ToolsToolBar", "Tools tool bar"),
                Shortcut("Window/TabBar", "Tab bar"),
                Shortcut("Window/Timeline", "Timeline"),
                Shortcut("Window/BottomToolBar", "Bottom tool bar"),
                Shortcut("Window/StatusToolBar", "Status tool bar"),

                Shortcut("Color/OCIO", "Enable OCIO", ftk::KeyShortcut(ftk::Key::N, static_cast<int>(ftk::KeyModifier::Control))),
                Shortcut("Color/LUT", "Enable LUT", ftk::KeyShortcut(ftk::Key::K, static_cast<int>(ftk::KeyModifier::Control)))
            };
        }

        bool ShortcutsSettings::operator == (const ShortcutsSettings& other) const
        {
            return shortcuts == other.shortcuts;
        }

        bool ShortcutsSettings::operator != (const ShortcutsSettings& other) const
        {
            return !(*this == other);
        }

        bool MiscSettings::operator == (const MiscSettings& other) const
        {
            return
                tooltipsEnabled == other.tooltipsEnabled &&
                showSetup == other.showSetup;
        }

        bool MiscSettings::operator != (const MiscSettings& other) const
        {
            return !(*this == other);
        }

        FTK_ENUM_IMPL(
            MouseAction,
            "Pan View",
            "Compare Wipe",
            "Pick",
            "Frame Shuttle");

        MouseActionBinding::MouseActionBinding(
            ftk::MouseButton button,
            ftk::KeyModifier modifier) :
            button(button),
            modifier(modifier)
        {}

        bool MouseActionBinding::operator == (const MouseActionBinding& other) const
        {
            return
                button == other.button &&
                modifier == other.modifier;
        }

        bool MouseActionBinding::operator != (const MouseActionBinding& other) const
        {
            return !(*this == other);
        }

        bool MouseSettings::operator == (const MouseSettings& other) const
        {
            return
                bindings == other.bindings &&
                wheelScale == other.wheelScale &&
                frameShuttleScale == other.frameShuttleScale;
        }

        bool MouseSettings::operator != (const MouseSettings& other) const
        {
            return !(*this == other);
        }

        bool StyleSettings::operator == (const StyleSettings& other) const
        {
            return
                displayScale == other.displayScale &&
                colorControls == other.colorControls &&
                colorStyle == other.colorStyle &&
                customColorRoles == other.customColorRoles;
        }

        bool StyleSettings::operator != (const StyleSettings& other) const
        {
            return !(*this == other);
        }

        FTK_ENUM_IMPL(
            TimelineThumbnails,
            "None",
            "Small",
            "Medium",
            "Large");

        int getTimelineThumbnailsSize(TimelineThumbnails value)
        {
            const std::array<int, static_cast<size_t>(TimelineThumbnails::Count)> data =
            {
                0,
                50,
                100,
                200
            };
            return data[static_cast<size_t>(value)];
        }

        int getTimelineWaveformSize(TimelineThumbnails value)
        {
            const std::array<int, static_cast<size_t>(TimelineThumbnails::Count)> data =
            {
                0,
                50 / 2,
                100 / 2,
                200 / 2
            };
            return data[static_cast<size_t>(value)];
        }

        bool TimelineSettings::operator == (const TimelineSettings& other) const
        {
            return
                minimize == other.minimize &&
                frameView == other.frameView &&
                scrollBars == other.scrollBars &&
                autoScroll == other.autoScroll &&
                stopOnScrub == other.stopOnScrub &&
                thumbnails == other.thumbnails;
        }

        bool TimelineSettings::operator != (const TimelineSettings& other) const
        {
            return !(*this == other);
        }

        bool WindowSettings::operator == (const WindowSettings& other) const
        {
            return
                size == other.size &&
                fileToolBar == other.fileToolBar &&
                compareToolBar == other.compareToolBar &&
                windowToolBar == other.windowToolBar &&
                viewToolBar == other.viewToolBar &&
                toolsToolBar == other.toolsToolBar &&
                tabBar == other.tabBar &&
                timeline == other.timeline &&
                bottomToolBar == other.bottomToolBar &&
                statusToolBar == other.statusToolBar &&
                splitter == other.splitter &&
                splitter2 == other.splitter2;
        }

        bool WindowSettings::operator != (const WindowSettings& other) const
        {
            return !(*this == other);
        }

        struct SettingsModel::Private
        {
            std::weak_ptr<ftk::Context> context;
            std::shared_ptr<ftk::Settings> settings;
            float defaultDisplayScale = 1.F;

            std::shared_ptr<ftk::Observable<AdvancedSettings> > advanced;
            std::shared_ptr<ftk::Observable<tl::PlayerCacheOptions> > cache;
            std::shared_ptr<ftk::Observable<ExportSettings> > exportSettings;
            std::shared_ptr<ftk::Observable<FileBrowserSettings> > fileBrowser;
            std::shared_ptr<ftk::Observable<ImageSeqSettings> > imageSeq;
            std::shared_ptr<ftk::Observable<ShortcutsSettings> > shortcuts;
            std::shared_ptr<ftk::Observable<MiscSettings> > misc;
            std::shared_ptr<ftk::Observable<MouseSettings> > mouse;
            std::shared_ptr<ftk::Observable<StyleSettings> > style;
            std::shared_ptr<ftk::Observable<TimelineSettings> > timeline;
            std::shared_ptr<ftk::Observable<WindowSettings> > window;
#if defined(TLRENDER_FFMPEG)
            std::shared_ptr<ftk::Observable<tl::ffmpeg::Options> > ffmpeg;
#endif // TLRENDER_FFMPEG
#if defined(TLRENDER_USD)
            std::shared_ptr<ftk::Observable<tl::usd::Options> > usd;
#endif // TLRENDER_USD
        };

        void SettingsModel::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings,
            float defaultDisplayScale)
        {
            FTK_P();

            p.context = context;
            p.settings = settings;
            p.defaultDisplayScale = defaultDisplayScale;

            AdvancedSettings advanced;
            settings->getT("/Advanced", advanced);
            p.advanced = ftk::Observable<AdvancedSettings>::create(advanced);

            tl::PlayerCacheOptions cache;
            settings->getT("/Cache", cache);
            p.cache = ftk::Observable<tl::PlayerCacheOptions>::create(cache);

            ExportSettings exportSettings;
            settings->getT("/Export", exportSettings);
            p.exportSettings = ftk::Observable<ExportSettings>::create(exportSettings);

            FileBrowserSettings fileBrowser;
            settings->getT("/FileBrowser", fileBrowser);
            p.fileBrowser = ftk::Observable<FileBrowserSettings>::create(fileBrowser);
            auto fileBrowserSystem = context->getSystem<ftk::FileBrowserSystem>();
            fileBrowserSystem->setNativeFileDialog(fileBrowser.nativeFileDialog);
            fileBrowserSystem->getModel()->setPath(fileBrowser.path);
            fileBrowserSystem->getModel()->setOptions(fileBrowser.options);
            fileBrowserSystem->getModel()->setExt(fileBrowser.ext);

            ImageSeqSettings imageSeq;
            settings->getT("/ImageSeq", imageSeq);
            p.imageSeq = ftk::Observable<ImageSeqSettings>::create(imageSeq);

            ShortcutsSettings shortcuts;
            settings->getT("/Shortcuts.1", shortcuts);
            p.shortcuts = ftk::Observable<ShortcutsSettings>::create(shortcuts);

            MiscSettings misc;
            settings->getT("/Misc", misc);
            p.misc = ftk::Observable<MiscSettings>::create(misc);

            MouseSettings mouse;
            settings->getT("/Mouse.1", mouse);
            p.mouse = ftk::Observable<MouseSettings>::create(mouse);

            StyleSettings style;
            style.displayScale = defaultDisplayScale;
            settings->getT("/Style", style);
            p.style = ftk::Observable<StyleSettings>::create(style);

            TimelineSettings timeline;
            settings->getT("/Timeline", timeline);
            p.timeline = ftk::Observable<TimelineSettings>::create(timeline);

            WindowSettings window;
            settings->getT("/Window", window);
            p.window = ftk::Observable<WindowSettings>::create(window);

#if defined(TLRENDER_FFMPEG)
            tl::ffmpeg::Options ffmpeg;
            settings->getT("/FFmpeg", ffmpeg);
            p.ffmpeg = ftk::Observable<tl::ffmpeg::Options>::create(ffmpeg);
#endif // TLRENDER_FFMPEG

#if defined(TLRENDER_USD)
            tl::usd::Options usd;
            settings->getT("/USD.1", usd);
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
            const std::shared_ptr<ftk::Settings>& settings,
            float defaultDisplayScale)
        {
            auto out = std::shared_ptr<SettingsModel>(new SettingsModel);
            out->_init(context, settings, defaultDisplayScale);
            return out;
        }

        void SettingsModel::save()
        {
            FTK_P();

            p.settings->setT("/Advanced", p.advanced->get());
            p.settings->setT("/Cache", p.cache->get());
            p.settings->setT("/Export", p.exportSettings->get());

            FileBrowserSettings fileBrowser = p.fileBrowser->get();
            if (auto context = p.context.lock())
            {
                auto fileBrowserSystem = context->getSystem<ftk::FileBrowserSystem>();
                fileBrowser.path = fileBrowserSystem->getModel()->getPath().u8string();
                fileBrowser.options = fileBrowserSystem->getModel()->getOptions();
                fileBrowser.ext = fileBrowserSystem->getModel()->getExt();
            }
            p.settings->setT("/FileBrowser", fileBrowser);

            p.settings->setT("/ImageSeq", p.imageSeq->get());
            p.settings->setT("/Shortcuts.1", p.shortcuts->get());
            p.settings->setT("/Misc", p.misc->get());
            p.settings->setT("/Mouse.1", p.mouse->get());
            p.settings->setT("/Style", p.style->get());
            p.settings->setT("/Timeline", p.timeline->get());
            p.settings->setT("/Window", p.window->get());

#if defined(TLRENDER_FFMPEG)
            p.settings->setT("/FFmpeg", p.ffmpeg->get());
#endif // TLRENDER_FFMPEG
#if defined(TLRENDER_USD)
            p.settings->setT("/USD.1", p.usd->get());
#endif // TLRENDER_USD

            p.settings->save();
        }

        void SettingsModel::reset()
        {
            FTK_P();
            setAdvanced(AdvancedSettings());
            setCache(tl::PlayerCacheOptions());
            setExport(ExportSettings());
            setFileBrowser(FileBrowserSettings());
            setImageSeq(ImageSeqSettings());
            setShortcuts(ShortcutsSettings());
            MiscSettings miscSettings;
            miscSettings.showSetup = false;
            setMisc(miscSettings);
            setMouse(MouseSettings());
            StyleSettings style;
            style.displayScale = p.defaultDisplayScale;
            setStyle(style);
            setTimeline(TimelineSettings());
            setWindow(WindowSettings());
#if defined(TLRENDER_FFMPEG)
            setFFmpeg(tl::ffmpeg::Options());
#endif // TLRENDER_FFMPEG
#if defined(TLRENDER_USD)
            setUSD(tl::usd::Options());
#endif // TLRENDER_USD
        }

        const AdvancedSettings& SettingsModel::getAdvanced() const
        {
            return _p->advanced->get();
        }

        std::shared_ptr<ftk::IObservable<AdvancedSettings> > SettingsModel::observeAdvanced() const
        {
            return _p->advanced;
        }

        void SettingsModel::setAdvanced(const AdvancedSettings& value)
        {
            _p->advanced->setIfChanged(value);
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

#if defined(TLRENDER_FFMPEG)
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
#endif // TLRENDER_FFMPEG

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

        void to_json(nlohmann::json& json, const AdvancedSettings& value)
        {
            json["Compat"] = value.compat;
            json["AudioBufferFrameCount"] = value.audioBufferFrameCount;
            json["VideoRequestMax"] = value.videoRequestMax;
            json["AudioRequestMax"] = value.audioRequestMax;
        }

        void to_json(nlohmann::json& json, const ExportSettings& value)
        {
            json["Dir"] = value.dir;
            json["RenderSize"] = to_string(value.renderSize);
            json["CustomSize"] = value.customSize;
            json["FileType"] = to_string(value.fileType);
            json["ImageBase"] = value.imageBase;
            json["ImageZeroPad"] = value.imageZeroPad;
            json["ImageExt"] = value.imageExt;
            json["MovieBase"] = value.movieBase;
            json["MovieExt"] = value.movieExt;
            json["MovieCodec"] = value.movieCodec;
        }

        void to_json(nlohmann::json& json, const FileBrowserSettings& value)
        {
            json["NativeFileDialog"] = value.nativeFileDialog;
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
            json["IO"] = value.io;
        }

        void to_json(nlohmann::json& json, const ShortcutsSettings& value)
        {
            for (const auto shortcut : value.shortcuts)
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
            json["WheelScale"] = value.wheelScale;
            json["FrameShuttleScale"] = value.frameShuttleScale;
        }

        void to_json(nlohmann::json& json, const StyleSettings& value)
        {
            json["DisplayScale"] = value.displayScale;
            json["ColorControls"] = value.colorControls;
            json["ColorStyle"] = to_string(value.colorStyle);
            for (auto i : value.customColorRoles)
            {
                json["CustomColorRoles"][ftk::getLabel(i.first)] = i.second;
            }
        }

        void to_json(nlohmann::json& json, const TimelineSettings& value)
        {
            json["Minimize"] = value.minimize;
            json["FrameView"] = value.frameView;
            json["ScrollBars"] = value.scrollBars;
            json["AutoScroll"] = value.autoScroll;
            json["StopOnScrub"] = value.stopOnScrub;
            json["Thumbnails"] = to_string(value.thumbnails);
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
                { "Splitter", in.splitter },
                { "Splitter2", in.splitter2 }
            };
        }

        void from_json(const nlohmann::json& json, AdvancedSettings& value)
        {
            json.at("Compat").get_to(value.compat);
            json.at("AudioBufferFrameCount").get_to(value.audioBufferFrameCount);
            json.at("VideoRequestMax").get_to(value.videoRequestMax);
            json.at("AudioRequestMax").get_to(value.audioRequestMax);
        }

        void from_json(const nlohmann::json& json, ExportSettings& value)
        {
            json.at("Dir").get_to(value.dir);
            from_string(json.at("RenderSize").get<std::string>(), value.renderSize);
            json.at("CustomSize").get_to(value.customSize);
            from_string(json.at("FileType").get<std::string>(), value.fileType);
            json.at("ImageBase").get_to(value.imageBase);
            json.at("ImageZeroPad").get_to(value.imageZeroPad);
            json.at("ImageExt").get_to(value.imageExt);
            json.at("MovieBase").get_to(value.movieBase);
            json.at("MovieExt").get_to(value.movieExt);
            json.at("MovieCodec").get_to(value.movieCodec);
        }

        void from_json(const nlohmann::json& json, FileBrowserSettings& value)
        {
            json.at("NativeFileDialog").get_to(value.nativeFileDialog);
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
            json.at("IO").get_to(value.io);
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
            json.at("WheelScale").get_to(value.wheelScale);
            json.at("FrameShuttleScale").get_to(value.frameShuttleScale);
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
            from_string(json.at("ColorStyle").get<std::string>(), value.colorStyle);
            json.at("ColorControls").get_to(value.colorControls);
            for (auto i = json.at("CustomColorRoles").begin(); i != json.at("CustomColorRoles").end(); ++i)
            {
                ftk::ColorRole colorRole = ftk::ColorRole::None;
                from_string(i.key(), colorRole);
                i.value().get_to(value.customColorRoles[colorRole]);
            }
        }

        void from_json(const nlohmann::json& json, TimelineSettings& value)
        {
            json.at("Minimize").get_to(value.minimize);
            json.at("FrameView").get_to(value.frameView);
            json.at("ScrollBars").get_to(value.scrollBars);
            json.at("AutoScroll").get_to(value.autoScroll);
            json.at("StopOnScrub").get_to(value.stopOnScrub);
            from_string(json.at("Thumbnails").get<std::string>(), value.thumbnails);
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
            json.at("Splitter").get_to(value.splitter);
            json.at("Splitter2").get_to(value.splitter2);
        }
    }
}
