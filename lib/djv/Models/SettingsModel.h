// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>
#include <djv/Models/Shortcuts.h>

#include <tlRender/UI/ItemOptions.h>
#include <tlRender/UI/ThumbnailSystem.h>
#include <tlRender/Timeline/Player.h>
#include <tlRender/IO/SeqIO.h>
#include <tlRender/Timeline/TimelineOptions.h>
#if defined(TLRENDER_FFMPEG_PLUGIN)
#include <tlRender/IO/FFmpeg.h>
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_FFMPEG_PLUGIN)
#include <tlRender/IO/FFmpegCmd.h>
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_USD)
#include <tlRender/IO/USD.h>
#endif // TLRENDER_USD

#include <ftk/Core/Path.h>

#include <ftk/UI/App.h>
#include <ftk/UI/FileBrowser.h>
#include <ftk/Core/Observable.h>

#include <nlohmann/json.hpp>

namespace ftk
{
    class Context;
    class Settings;
}

namespace djv
{
    namespace models
    {
        //! Audio settings.
        struct DJV_MODELS_API_TYPE AudioSettings
        {
            //! The size of the buffer the audio device is asked for, in
            //! sample frames; zero is the device's own default. Larger
            //! buffers are less likely to underrun and produce glitches;
            //! smaller ones reduce latency.
            size_t bufferFrameCount = 0;

            bool operator == (const AudioSettings&) const = default;
        };

        //! Export render size: the source's own, a preset width, a width
        //! typed in, or a width and height typed in. A width alone has the
        //! height follow the aspect ratio of what is being exported.
        enum class DJV_MODELS_API_TYPE ExportRenderSize
        {
            Source,
            _1920,
            _3840,
            _4096,
            Width,
            Custom,

            Count,
            First = Source
        };
        FTK_ENUM(DJV_MODELS_API, ExportRenderSize);

        //! Get an export render width, or zero for the sizes that are not
        //! one of the presets.
        DJV_MODELS_API int getWidth(ExportRenderSize);

        //! Export file type.
        enum class DJV_MODELS_API_TYPE ExportFileType
        {
            Image,
            Seq,
            Movie,

            Count,
            First = Image
        };
        FTK_ENUM(DJV_MODELS_API, ExportFileType);

        //! Whether the number in a file name is a run of '#', which is
        //! what the frame number replaces. Trailing digits are read as a
        //! frame number too, and replacing those would write "shot_v002.exr"
        //! as "shot_v023.exr".
        DJV_MODELS_API bool isFrameTemplate(const ftk::Path&);

        //! Take one of the given extensions off the end of a name, so that a
        //! name typed or pasted with an extension is not written as
        //! "shot.exr.tif". Returns whether one was taken.
        DJV_MODELS_API bool splitExt(
            std::string& name,
            std::string& ext,
            const std::vector<std::string>& exts);

        //! What is wrong with an export file name, or nothing.
        DJV_MODELS_API std::string getFileNameError(
            const std::string& fileName,
            const std::string& ext,
            ExportFileType,
            const std::vector<std::string>& exts);

        //! What to name the output of a file, without the extension: the
        //! name for a single file, and the name for a sequence with a run
        //! of '#' where the frame number goes.
        //!
        //! Trailing digits are a frame number only in an image sequence,
        //! which is what the extensions say: "Wide_169.mov" is a movie whose
        //! name ends in digits and is written as "Wide_169". A sequence keeps
        //! the shape it had, since the base carries whatever separator came
        //! before the frame number.
        DJV_MODELS_API void getExportNames(
            const ftk::Path& path,
            const std::vector<std::string>& seqExts,
            std::string& name,
            std::string& seqName);

        //! Export settings.
        struct DJV_MODELS_API_TYPE ExportSettings
        {
            //! The directory the exports are written to. One that is empty when
            //! the settings are read is set to the home directory.
            std::string dir;
            //! The size the export is rendered at.
            ExportRenderSize renderSize = ExportRenderSize::Source;
            //! The width typed in, for Width and Custom, and the height for
            //! Custom, used as they are.
            int customWidth = 1920;
            //! The height typed in, for Custom.
            int customHeight = 1080;
            //! The kind of export, as the tabs of the Export tool choose it.
            ExportFileType fileType = ExportFileType::Image;

            //! The output file names, without their extensions. A run of '#'
            //! is where the frame number goes, as many digits wide as there
            //! are '#'; digits are always part of the name.
            std::string imageFileName = "render";
            //! The extension of an image export, which chooses its file format.
            std::string imageExt = ".tif";
            //! The name of a sequence export.
            std::string seqFileName = "render.####";
            //! The extension of a sequence export, which chooses its file
            //! format.
            std::string seqExt = ".tif";
            //! The name of a movie export.
            std::string movieFileName = "render";
            //! The extension of a movie export: one of those the preset writes.
            std::string movieExt = ".mov";
            //! The audio codec of a movie export, by its codec name, such as
            //! "aac". "Auto" leaves it to the file format.
            std::string movieAudioCodec = "Auto";
            //! The movie export preset. Presets are the whole surface:
            //! what they cannot express is what tlbake and the command
            //! line writer's options are for.
            std::string moviePreset = "MJPEG";

            //! Whether the drawings are written into the picture.
            bool burnAnnotations = false;

            bool operator == (const ExportSettings&) const = default;
        };

        //! The path an export of the given type is written to, given the
        //! file name with any frame number put in. The writers put the
        //! frame in place of the path's number, so a name typed without a
        //! run of '#' is given a path without one: an image named
        //! "shot_v002" is written as that, not with the frame's number.
        DJV_MODELS_API ftk::Path getExportPath(
            const ExportSettings&,
            ExportFileType,
            const std::string& fileName);

        //! File browser settings.
        struct DJV_MODELS_API_TYPE FileBrowserSettings
        {
            //! Whether files are chosen with the platform's own file dialog
            //! rather than the built-in file browser. Where the build has no
            //! native dialog it is kept but does nothing.
            bool nativeFileDialog = true;
            bool floating = false;
            bool pinned = false;
            //! The size a floating file browser opens at. Unset until one
            //! has been opened, leaving the size to the file browser.
            ftk::Size2I windowSize;
            //! The directory the built-in file browser is in. It is given to
            //! the file browser at startup and read back from it when the
            //! settings are saved, so setting it does not move the file
            //! browser.
            std::string path;
            //! The built-in file browser's options, kept the same way as the
            //! path.
            ftk::FileBrowserOptions options;
            //! The extension the built-in file browser filters by, kept the
            //! same way as the path.
            std::string ext;

            bool operator == (const FileBrowserSettings&) const = default;
        };

        //! Image sequence settings.
        struct DJV_MODELS_API_TYPE ImageSeqSettings
        {
            //! How an image sequence finds its audio: not at all, by extension,
            //! or by file name.
            tl::ImageSeqAudio audio = tl::Options().imageSeqAudio;
            //! The audio file extensions looked for, by extension: first a file
            //! with the sequence's base name, then the first such file in its
            //! directory.
            std::vector<std::string> audioExts = tl::Options().imageSeqAudioExts;
            //! The audio file opened with a sequence, by file name, in the
            //! sequence's directory.
            std::string audioFileName = tl::Options().imageSeqAudioFileName;
            //! The most digits a frame number can have. A file name ending in
            //! more is not read as a frame of a sequence.
            size_t maxDigits = 9;

            //! How many frames are decoded at once. This is a timeline
            //! setting rather than a per format one: the timeline is what
            //! decodes sequences.
            size_t readThreadCount = tl::getDefaultReadThreadCount();

            //! The sequence read options: the default speed and what to do
            //! about a missing frame.
            tl::SeqOptions io;

            bool operator == (const ImageSeqSettings&) const = default;
        };

        //! OTIO settings.
        struct DJV_MODELS_API_TYPE OTIOSettings
        {
            tl::Spatial spatial = tl::Options().spatial;
            //! Whether to work around timelines that do not conform exactly to
            //! the specification.
            bool compat = tl::Options().compat;

            bool operator == (const OTIOSettings&) const = default;
        };

        //! Miscellaneous settings.
        struct DJV_MODELS_API_TYPE MiscSettings
        {
            //! Whether tooltips are shown. Always off in the tablet build,
            //! where nothing hovers.
            bool tooltipsEnabled = true;
            //! Whether the tour is shown at startup. It is turned off when the
            //! tour is shown, so a first start shows it once.
            bool showSetup = true;

            //! Forget the recent files, playlists, reviews and directories
            //! when the application exits (DJV #909).
            bool clearRecentOnExit = false;

            bool operator == (const MiscSettings&) const = default;
        };

        //! Mouse actions.
        enum class DJV_MODELS_API_TYPE MouseAction
        {
            PanView,
            CompareWipe,
            Pick,
            FrameShuttle,

            Count,
            First = PanView
        };
        FTK_ENUM(DJV_MODELS_API, MouseAction);

        //! Mouse wheel actions. The wheel rides a modifier rather than a
        //! button, so these bind separately from the button actions.
        enum class DJV_MODELS_API_TYPE WheelAction
        {
            Zoom,
            Scrub,

            Count,
            First = Zoom
        };
        FTK_ENUM(DJV_MODELS_API, WheelAction);

        //! Mouse tools: what the plain left button does, chosen rather than
        //! held. A touch screen has no modifier keys, and a pen tablet is
        //! easier without them; with a tool set, the plain left button is
        //! the tool's, and any other action bound to it gives it up. None
        //! leaves the bindings as they are.
        enum class DJV_MODELS_API_TYPE MouseTool
        {
            None,
            PanView,
            CompareWipe,
            Pick,
            FrameShuttle,

            Count,
            First = None
        };
        FTK_ENUM(DJV_MODELS_API, MouseTool);

        //! Mouse action binding.
        struct DJV_MODELS_API_TYPE MouseActionBinding
        {
            MouseActionBinding() = default;
            DJV_MODELS_API MouseActionBinding(
                ftk::MouseButton,
                ftk::KeyModifier modifier = ftk::KeyModifier::None);

            //! The mouse button.
            ftk::MouseButton button = ftk::MouseButton::None;
            //! The modifier key held with the button.
            ftk::KeyModifier modifier = ftk::KeyModifier::None;

            bool operator == (const MouseActionBinding&) const = default;
        };

        //! Mouse settings.
        struct DJV_MODELS_API_TYPE MouseSettings
        {
            //! The mouse button and modifier key bound to each action.
            std::map<MouseAction, MouseActionBinding> bindings =
            {
                {
                    MouseAction::PanView,
                    MouseActionBinding(ftk::MouseButton::Middle)
                },
                {
                    MouseAction::CompareWipe,
                    MouseActionBinding(ftk::MouseButton::Left, ftk::KeyModifier::Alt)
                },
                {
                    MouseAction::Pick,
                    MouseActionBinding(ftk::MouseButton::Left, ftk::KeyModifier::Control)
                },
                {
                    MouseAction::FrameShuttle,
                    MouseActionBinding(ftk::MouseButton::Left)
                }
            };

            MouseTool tool = MouseTool::None;
            std::map<WheelAction, ftk::KeyModifier> wheelBindings =
            {
                { WheelAction::Zoom, ftk::KeyModifier::None },
                { WheelAction::Scrub, ftk::KeyModifier::Control }
            };

            //! The factor one click of the mouse wheel zooms the view and the
            //! timeline by. The Settings tool allows 0.5 to 5.
            float wheelScale = 1.1F;
            //! How fast the frame shuttle steps: at one, a frame each 20 pixels
            //! dragged. The Settings tool allows 0.1 to 10.
            float frameShuttleScale = 1.F;

            bool operator == (const MouseSettings&) const = default;
        };

        //! Playback settings.
        struct DJV_MODELS_API_TYPE PlaybackSettings
        {
            //! Whether playback starts when a file is opened.
            bool startPlayback = false;

            //! Wait for each frame rather than drop the ones the cache does
            //! not have in time, with the audio silent; see
            //! tl::Player::setPlayEveryFrame().
            bool playEveryFrame = false;

            bool operator == (const PlaybackSettings&) const = default;
        };

        //! Review settings.
        struct DJV_MODELS_API_TYPE ReviewSettings
        {
            //! Who the markers and drawings are by. Empty is the user name
            //! the system gives.
            std::string author;

            bool operator == (const ReviewSettings&) const = default;
        };

        //! Keyboard shortcuts settings.
        struct DJV_MODELS_API_TYPE ShortcutsSettings
        {
            DJV_MODELS_API ShortcutsSettings();

            //! The keyboard shortcuts of the actions, with the keys they have
            //! now.
            std::vector<Shortcut> shortcuts;

            bool operator == (const ShortcutsSettings&) const = default;
        };

        //! Get the shortcuts that differ from their defaults, which are the
        //! ones saved. A shortcut without a default counts as changed.
        DJV_MODELS_API std::vector<Shortcut> getChangedShortcuts(
            const std::vector<Shortcut>&,
            const std::vector<Shortcut>& defaults);

        //! Get the shortcuts that have a key.
        DJV_MODELS_API std::vector<Shortcut> getBoundShortcuts(
            const std::vector<Shortcut>&);

        //! Style settings.
        struct DJV_MODELS_API_TYPE StyleSettings
        {
            //! Zero follows the display.
            float displayScale = 0.F;
            //! The brightness and contrast of the interface.
            ftk::ColorControls colorControls;
            //! The interface colors, dark or light.
            ftk::ColorStyle colorStyle = ftk::ColorStyle::Dark;

            bool operator == (const StyleSettings&) const = default;
        };

        //! Timeline thumbnails.
        enum class DJV_MODELS_API_TYPE TimelineThumbnailSize
        {
            Small,
            Medium,
            Large,

            Count,
            First = Small
        };
        FTK_ENUM(DJV_MODELS_API, TimelineThumbnailSize);

        //! Get the timeline thumbnails size.
        DJV_MODELS_API int getTimelineThumbnailSize(TimelineThumbnailSize);

        //! Get the timeline waveform size.
        DJV_MODELS_API int getTimelineWaveformSize(TimelineThumbnailSize);

        //! Timeline settings.
        struct DJV_MODELS_API_TYPE TimelineSettings
        {
            //! Whether the timeline shows only the first video and audio track:
            //! Timeline > Minimize.
            bool minimize = true;
            //! Whether the timeline is zoomed to show the whole of it:
            //! Timeline > Frame View. Panning the timeline turns it off.
            bool frameView = true;
            //! Whether a zoomed timeline has scroll bars:
            //! Timeline > Scroll Bars.
            bool scrollBars = true;
            //! Whether the timeline scrolls to keep the current frame in sight
            //! while playing: Timeline > Auto Scroll.
            bool autoScroll = true;
            //! Whether scrubbing stops playback, rather than pausing it while
            //! the button is held: Timeline > Stop Playback When Scrubbing.
            bool stopOnScrub = false;
            //! A preview of the frame under the cursor.
            bool preview = true;
            //! The timeline of each file being compared, under the current
            //! file's.
            bool compareTimelines = false;
            // Whether media is shown in the timeline at all. Separate from
            // the two below rather than setting them, so that turning it off
            // and on again does not forget which of them was wanted.
            //! Whether the video thumbnails and audio waveforms are shown at
            //! all: Timeline > Track Media.
            bool trackMedia = true;
            //! Whether the video thumbnails are shown, with track media on:
            //! Timeline > Video Thumbnails.
            bool thumbnails = true;
            //! The height of the video thumbnails:
            //! Timeline > Video Thumbnail Size.
            TimelineThumbnailSize thumbnailSize = TimelineThumbnailSize::Small;
            //! Whether the audio waveforms are shown, with track media on:
            //! Timeline > Audio Waveforms.
            bool waveforms = true;
            //! The height of the audio waveforms:
            //! Timeline > Audio Waveform Size.
            TimelineThumbnailSize waveformSize = TimelineThumbnailSize::Small;

            bool operator == (const TimelineSettings&) const = default;
        };

        //! Window settings.
        struct DJV_MODELS_API_TYPE WindowSettings
        {
            //! The size the window has when it is not maximized.
            ftk::Size2I size = ftk::Size2I(1600, 960);
            //! Where the window was, once it has been somewhere: until then
            //! the platform places it.
            ftk::V2I pos;
            //! Whether there is a position to put the window at.
            bool hasPos = false;
            //! Whether the window is maximized. The position, size and
            //! maximized state are applied when the main window is made, and
            //! saved from it.
            bool maximized = false;
            //! Whether the file tool bar is shown: Window > File Tool Bar.
            bool fileToolBar = true;
            //! Whether the compare tool bar is shown:
            //! Window > Compare Tool Bar.
            bool compareToolBar = true;
            //! Whether the window tool bar is shown: Window > Window Tool Bar.
            bool windowToolBar = true;
            //! Whether the view tool bar is shown: Window > View Tool Bar.
            bool viewToolBar = true;
            //! Whether the tools tool bar is shown: Window > Tools Tool Bar.
            bool toolsToolBar = true;
            //! Whether the tab bar is shown: Window > Tab Bar.
            bool tabBar = true;
            //! Whether the timeline is shown, when a file is open:
            //! Window > Timeline.
            bool timeline = true;
            //! Whether the bottom tool bar is shown: Window > Bottom Tool Bar.
            bool bottomToolBar = true;
            //! Whether the status bar is shown: Window > Status Tool Bar.
            bool statusToolBar = true;
            //! Whether the tools panel is shown, when a tool is open:
            //! Window > Tools.
            bool tools = true;
            //! The share of the height the view has above the timeline, from
            //! zero to one. The splitters are applied when the main window is
            //! made.
            float splitter = .7F;
            //! The share of the width the view has beside the tools panel, from
            //! zero to one.
            float splitter2 = .7F;

            bool operator == (const WindowSettings&) const = default;
        };

        //! Settings model.
        class DJV_MODELS_API_TYPE SettingsModel : public std::enable_shared_from_this<SettingsModel>
        {
            FTK_NON_COPYABLE(SettingsModel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            SettingsModel();

        public:
            DJV_MODELS_API ~SettingsModel();

            //! Create a new model.
            DJV_MODELS_API static std::shared_ptr<SettingsModel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            //! Save the settings. Settings are also saved on exit.
            DJV_MODELS_API void save();

            //! Reset to default values.
            DJV_MODELS_API void reset();

            //! \name Audio
            ///@{

            //! Get the audio settings.
            DJV_MODELS_API const AudioSettings& getAudio() const;
            //! Observe the audio settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<AudioSettings> > observeAudio() const;
            //! Set the audio settings.
            DJV_MODELS_API void setAudio(const AudioSettings&);

            ///@}

            //! \name Cache
            ///@{

            //! Get the cache size the machine can hold: what it has of RAM,
            //! in gigabytes, and at least one. Nothing else is allowed for
            //! here -- the decoders, the display, and whatever else is
            //! running want their share too -- but a cache larger than the
            //! machine itself can only end one way. On the iPad, a quarter
            //! of it, as iOS stops an application that takes much more.
            DJV_MODELS_API static float getMaxCacheGB();

            //! Get the cache sizes a machine of this size starts with: a
            //! quarter of RAM for the video.
            DJV_MODELS_API static tl::PlayerCacheOptions getDefaultCache();

            //! Get the cache settings: the sizes of the video and audio caches,
            //! in gigabytes, and how much is read behind the current frame.
            //! Sizes above getMaxCacheGB() are lowered to it when the settings
            //! are read.
            DJV_MODELS_API const tl::PlayerCacheOptions& getCache() const;
            //! Observe the cache settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::PlayerCacheOptions> > observeCache() const;
            //! Set the cache settings.
            DJV_MODELS_API void setCache(const tl::PlayerCacheOptions&);

            DJV_MODELS_API const tl::ui::ThumbnailCacheOptions& getThumbnailCache() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::ui::ThumbnailCacheOptions> > observeThumbnailCache() const;
            DJV_MODELS_API void setThumbnailCache(const tl::ui::ThumbnailCacheOptions&);

            ///@}

            //! \name Export
            ///@{

            //! Get the export settings.
            DJV_MODELS_API const ExportSettings& getExport() const;
            //! Observe the export settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<ExportSettings> > observeExport() const;
            //! Set the export settings.
            DJV_MODELS_API void setExport(const ExportSettings&);

            ///@}

            //! \name File Browser
            ///@{

            //! Get the file browser settings.
            DJV_MODELS_API const FileBrowserSettings& getFileBrowser() const;
            //! Observe the file browser settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<FileBrowserSettings> > observeFileBrowser() const;
            //! Set the file browser settings. Of these, whether the native file
            //! dialog is used, and whether the built-in one floats, apply at
            //! once.
            DJV_MODELS_API void setFileBrowser(const FileBrowserSettings&);

            ///@}

            //! \name Image Sequences
            ///@{

            //! Get the image sequence settings. They are taken when a file is
            //! opened.
            DJV_MODELS_API const ImageSeqSettings& getImageSeq() const;
            //! Observe the image sequence settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<ImageSeqSettings> > observeImageSeq() const;
            //! Set the image sequence settings.
            DJV_MODELS_API void setImageSeq(const ImageSeqSettings&);

            ///@}

            //! \name OTIO
            ///@{

            //! Get the OTIO settings. They are taken when a file is opened.
            DJV_MODELS_API const OTIOSettings& getOTIO() const;
            //! Observe the OTIO settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<OTIOSettings> > observeOTIO() const;
            //! Set the OTIO settings.
            DJV_MODELS_API void setOTIO(const OTIOSettings&);

            ///@}

            //! \name Miscellaneous
            ///@{

            //! Get the miscellaneous settings.
            DJV_MODELS_API const MiscSettings& getMisc() const;
            //! Observe the miscellaneous settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<MiscSettings> > observeMisc() const;
            //! Set the miscellaneous settings.
            DJV_MODELS_API void setMisc(const MiscSettings&);

            ///@}

            //! \name Mouse
            ///@{

            //! Get the mouse settings.
            DJV_MODELS_API const MouseSettings& getMouse() const;
            //! Observe the mouse settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<MouseSettings> > observeMouse() const;
            //! Set the mouse settings.
            DJV_MODELS_API void setMouse(const MouseSettings&);

            ///@}

            //! \name Playback
            ///@{

            //! Get the playback settings.
            DJV_MODELS_API const PlaybackSettings& getPlayback() const;
            //! Observe the playback settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<PlaybackSettings> > observePlayback() const;
            //! Set the playback settings.
            DJV_MODELS_API void setPlayback(const PlaybackSettings&);

            //! Get the review settings.
            DJV_MODELS_API const ReviewSettings& getReview() const;
            //! Observe the review settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<ReviewSettings> > observeReview() const;
            //! Set the review settings.
            DJV_MODELS_API void setReview(const ReviewSettings&);

            ///@}

            //! \name Keyboard Shortcuts
            ///@{

            //! Get the keyboard shortcuts settings.
            DJV_MODELS_API const ShortcutsSettings& getShortcuts() const;
            //! Observe the keyboard shortcuts settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<ShortcutsSettings> > observeShortcuts() const;
            //! Set the keyboard shortcuts settings.
            DJV_MODELS_API void setShortcuts(const ShortcutsSettings&);

            //! Add keyboard shortcuts. Any saved key bindings are applied to
            //! the added shortcuts.
            DJV_MODELS_API void addShortcuts(const std::vector<Shortcut>&);

            ///@}

            //! \name Style
            ///@{

            //! Get the style settings.
            DJV_MODELS_API const StyleSettings& getStyle() const;
            //! Observe the style settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<StyleSettings> > observeStyle() const;
            //! Set the style settings.
            DJV_MODELS_API void setStyle(const StyleSettings&);

            ///@}

            //! \name Timeline
            ///@{

            //! Get the timeline settings.
            DJV_MODELS_API const TimelineSettings& getTimeline() const;
            //! Observe the timeline settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<TimelineSettings> > observeTimeline() const;
            //! Set the timeline settings.
            DJV_MODELS_API void setTimeline(const TimelineSettings&);

            ///@}

            //! \name Window
            ///@{

            //! Get the window settings.
            DJV_MODELS_API const WindowSettings& getWindow() const;
            //! Observe the window settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<WindowSettings> > observeWindow() const;
            //! Set the window settings.
            DJV_MODELS_API void setWindow(const WindowSettings&);

            ///@}

#if defined(TLRENDER_FFMPEG_PLUGIN)
            //! \name FFmpeg
            ///@{

            DJV_MODELS_API const tl::ffmpeg::Options& getFFmpeg() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::ffmpeg::Options> > observeFFmpeg() const;
            DJV_MODELS_API void setFFmpeg(const tl::ffmpeg::Options&);

            ///@}
#endif // TLRENDER_FFMPEG_PLUGIN

#if defined(TLRENDER_FFMPEG_PLUGIN)
            //! \name FFmpeg Command
            ///@{

            DJV_MODELS_API const tl::ffmpeg_cmd::Options& getFFmpegCmd() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::ffmpeg_cmd::Options> > observeFFmpegCmd() const;
            DJV_MODELS_API void setFFmpegCmd(const tl::ffmpeg_cmd::Options&);

            ///@}
#endif // TLRENDER_FFMPEG_PLUGIN

#if defined(TLRENDER_USD)
            //! \name USD
            ///@{

            DJV_MODELS_API const tl::usd::Options& getUSD() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::usd::Options> > observeUSD() const;
            DJV_MODELS_API void setUSD(const tl::usd::Options&);

            ///@}
#endif // TLRENDER_USD

            //! \name I/O Options
            ///@{

            DJV_MODELS_API tl::IOOptions getIOOptions() const;

            //! Observe the I/O options: the image sequence, FFmpeg and USD
            //! settings together, as they are given to what reads files.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::IOOptions> > observeIOOptions() const;

            ///@}

        private:
            FTK_PRIVATE();
        };

        //! \name Serialize
        ///@{

        DJV_MODELS_API void to_json(nlohmann::json&, const AudioSettings&);
        DJV_MODELS_API void to_json(nlohmann::json&, const ExportSettings&);
        DJV_MODELS_API void to_json(nlohmann::json&, const FileBrowserSettings&);
        DJV_MODELS_API void to_json(nlohmann::json&, const ImageSeqSettings&);
        DJV_MODELS_API void to_json(nlohmann::json&, const OTIOSettings&);
        DJV_MODELS_API void to_json(nlohmann::json&, const MiscSettings&);
        DJV_MODELS_API void to_json(nlohmann::json&, const MouseActionBinding&);
        DJV_MODELS_API void to_json(nlohmann::json&, const MouseSettings&);
        DJV_MODELS_API void to_json(nlohmann::json&, const PlaybackSettings&);
        DJV_MODELS_API void to_json(nlohmann::json&, const ReviewSettings&);
        DJV_MODELS_API void to_json(nlohmann::json&, const ShortcutsSettings&);
        DJV_MODELS_API void to_json(nlohmann::json&, const StyleSettings&);
        DJV_MODELS_API void to_json(nlohmann::json&, const TimelineSettings&);
        DJV_MODELS_API void to_json(nlohmann::json&, const WindowSettings&);

        DJV_MODELS_API void from_json(const nlohmann::json&, AudioSettings&);
        DJV_MODELS_API void from_json(const nlohmann::json&, ExportSettings&);
        DJV_MODELS_API void from_json(const nlohmann::json&, FileBrowserSettings&);
        DJV_MODELS_API void from_json(const nlohmann::json&, ImageSeqSettings&);
        DJV_MODELS_API void from_json(const nlohmann::json&, OTIOSettings&);
        DJV_MODELS_API void from_json(const nlohmann::json&, MiscSettings&);
        DJV_MODELS_API void from_json(const nlohmann::json&, MouseActionBinding&);
        DJV_MODELS_API void from_json(const nlohmann::json&, MouseSettings&);
        DJV_MODELS_API void from_json(const nlohmann::json&, PlaybackSettings&);
        DJV_MODELS_API void from_json(const nlohmann::json&, ReviewSettings&);
        DJV_MODELS_API void from_json(const nlohmann::json&, ShortcutsSettings&);
        DJV_MODELS_API void from_json(const nlohmann::json&, StyleSettings&);
        DJV_MODELS_API void from_json(const nlohmann::json&, TimelineSettings&);
        DJV_MODELS_API void from_json(const nlohmann::json&, WindowSettings&);

        ///@}
    }
}
