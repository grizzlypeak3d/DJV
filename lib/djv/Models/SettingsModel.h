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
#if defined(TLRENDER_FFMPEG_CMD)
#include <tlRender/IO/FFmpegCmd.h>
#endif // TLRENDER_FFMPEG_CMD
#if defined(TLRENDER_USD)
#include <tlRender/IO/USD.h>
#endif // TLRENDER_USD

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
        struct DJV_API_TYPE AudioSettings
        {
            //! The size of the buffer the audio device is given. Larger
            //! buffers are less likely to underrun and produce glitches;
            //! smaller ones reduce latency.
            size_t bufferFrameCount = tl::PlayerOptions().audioBufferFrameCount;

            DJV_API bool operator == (const AudioSettings&) const;
            DJV_API bool operator != (const AudioSettings&) const;
        };

        //! Export render width. Only the width, because the height follows
        //! the aspect ratio of what is being exported.
        enum class DJV_API_TYPE ExportRenderSize
        {
            Default,
            _1920,
            _3840,
            _4096,
            Custom,

            Count,
            First = Default
        };
        TL_ENUM(ExportRenderSize);

        //! Get an export render width, or zero for the sizes that are not
        //! one of the presets.
        DJV_API int getWidth(ExportRenderSize);

        //! Export file type.
        enum class DJV_API_TYPE ExportFileType
        {
            Image,
            Seq,
            Movie,

            Count,
            First = Image
        };
        FTK_ENUM(ExportFileType);

        //! Export settings.
        struct DJV_API_TYPE ExportSettings
        {
            std::string dir;
            ExportRenderSize renderSize = ExportRenderSize::Default;
            // Only the width: the height follows from the aspect ratio of
            // what is being exported, which is known whenever there is
            // anything to export.
            int customWidth = 1920;
            ExportFileType fileType = ExportFileType::Image;

            std::string imageBase = "render.";
            size_t imageZeroPad = 0;
            std::string imageExt = ".tif";

            std::string seqBase = "render.";
            size_t seqZeroPad = 4;
            std::string seqExt = ".tif";

            std::string movieBase = "render";
            std::string movieExt = ".mov";
            std::string movieCodec = "mjpeg";
            std::string movieAudioCodec = "Auto";

            DJV_API bool operator == (const ExportSettings&) const;
            DJV_API bool operator != (const ExportSettings&) const;
        };

        //! File browser settings.
        struct DJV_API_TYPE FileBrowserSettings
        {
            bool nativeFileDialog = true;
            bool floating = false;
            bool pinned = false;
            std::string path;
            ftk::FileBrowserOptions options;
            std::string ext;

            DJV_API bool operator == (const FileBrowserSettings&) const;
            DJV_API bool operator != (const FileBrowserSettings&) const;
        };

        //! Image sequence settings.
        struct DJV_API_TYPE ImageSeqSettings
        {
            tl::ImageSeqAudio audio = tl::Options().imageSeqAudio;
            std::vector<std::string> audioExts = tl::Options().imageSeqAudioExts;
            std::string audioFileName = tl::Options().imageSeqAudioFileName;
            size_t maxDigits = 9;

            //! How many frames are decoded at once. This is a timeline
            //! setting rather than a per format one: the timeline is what
            //! decodes sequences.
            size_t readThreadCount = tl::getDefaultReadThreadCount();

            tl::SeqOptions io;

            DJV_API bool operator == (const ImageSeqSettings&) const;
            DJV_API bool operator != (const ImageSeqSettings&) const;
        };

        //! OTIO settings.
        struct DJV_API_TYPE OTIOSettings
        {
            tl::Spatial spatial = tl::Options().spatial;
            bool compat = tl::Options().compat;

            DJV_API bool operator == (const OTIOSettings&) const;
            DJV_API bool operator != (const OTIOSettings&) const;
        };

        //! Miscellaneous settings.
        struct DJV_API_TYPE MiscSettings
        {
            bool tooltipsEnabled = true;
            bool showSetup = true;

            DJV_API bool operator == (const MiscSettings&) const;
            DJV_API bool operator != (const MiscSettings&) const;
        };

        //! Mouse actions.
        enum class DJV_API_TYPE MouseAction
        {
            PanView,
            CompareWipe,
            Pick,
            FrameShuttle,

            Count,
            First = Pick
        };
        FTK_ENUM(MouseAction);

        //! Mouse action binding.
        struct DJV_API_TYPE MouseActionBinding
        {
            MouseActionBinding() = default;
            DJV_API MouseActionBinding(
                ftk::MouseButton,
                ftk::KeyModifier modifier = ftk::KeyModifier::None);

            ftk::MouseButton button = ftk::MouseButton::None;
            ftk::KeyModifier modifier = ftk::KeyModifier::None;

            DJV_API bool operator == (const MouseActionBinding&) const;
            DJV_API bool operator != (const MouseActionBinding&) const;
        };

        //! Mouse settings.
        struct DJV_API_TYPE MouseSettings
        {
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

            float wheelScale = 1.1F;
            float frameShuttleScale = 1.F;

            DJV_API bool operator == (const MouseSettings&) const;
            DJV_API bool operator != (const MouseSettings&) const;
        };

        //! Playback settings.
        struct DJV_API_TYPE PlaybackSettings
        {
            bool startPlayback = false;

            DJV_API bool operator == (const PlaybackSettings&) const;
            DJV_API bool operator != (const PlaybackSettings&) const;
        };

        //! Keyboard shortcuts settings.
        struct DJV_API_TYPE ShortcutsSettings
        {
            DJV_API ShortcutsSettings();

            std::vector<Shortcut> shortcuts;

            DJV_API bool operator == (const ShortcutsSettings&) const;
            DJV_API bool operator != (const ShortcutsSettings&) const;
        };

        //! Style settings.
        struct DJV_API_TYPE StyleSettings
        {
            DJV_API StyleSettings();

            float displayScale = 1.F;
            ftk::ColorControls colorControls;
            ftk::ColorStyle colorStyle = ftk::ColorStyle::Dark;
            std::map<ftk::ColorRole, ftk::Color4F> customColorRoles = ftk::getCustomColorRoles();
            std::map<ftk::FontType, std::string> fonts;
            std::vector<std::string> fontFiles;

            DJV_API bool operator == (const StyleSettings&) const;
            DJV_API bool operator != (const StyleSettings&) const;
        };

        //! Timeline thumbnails.
        enum class DJV_API_TYPE TimelineThumbnailSize
        {
            Small,
            Medium,
            Large,

            Count,
            First = Small
        };
        FTK_ENUM(TimelineThumbnailSize);

        //! Get the timeline thumbnails size.
        DJV_API int getTimelineThumbnailSize(TimelineThumbnailSize);

        //! Get the timeline waveform size.
        DJV_API int getTimelineWaveformSize(TimelineThumbnailSize);

        //! Timeline settings.
        struct DJV_API_TYPE TimelineSettings
        {
            bool minimize = true;
            bool frameView = true;
            bool scrollBars = true;
            bool autoScroll = true;
            bool stopOnScrub = false;
            // Whether media is shown in the timeline at all. Separate from
            // the two below rather than setting them, so that turning it off
            // and on again does not forget which of them was wanted.
            bool trackMedia = true;
            bool thumbnails = true;
            TimelineThumbnailSize thumbnailSize = TimelineThumbnailSize::Small;
            bool waveforms = true;
            TimelineThumbnailSize waveformSize = TimelineThumbnailSize::Small;

            DJV_API bool operator == (const TimelineSettings&) const;
            DJV_API bool operator != (const TimelineSettings&) const;
        };

        //! Window settings.
        struct DJV_API_TYPE WindowSettings
        {
            ftk::Size2I size = ftk::Size2I(1600, 960);
            bool fileToolBar = true;
            bool compareToolBar = true;
            bool windowToolBar = true;
            bool viewToolBar = true;
            bool toolsToolBar = true;
            bool tabBar = true;
            bool timeline = true;
            bool bottomToolBar = true;
            bool statusToolBar = true;
            bool tools = true;
            float splitter = .7F;
            float splitter2 = .7F;

            DJV_API bool operator == (const WindowSettings&) const;
            DJV_API bool operator != (const WindowSettings&) const;
        };

        //! Settings model.
        class DJV_API_TYPE SettingsModel : public std::enable_shared_from_this<SettingsModel>
        {
            FTK_NON_COPYABLE(SettingsModel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                float displayScaleDefault);

            SettingsModel();

        public:
            DJV_API ~SettingsModel();

            //! Create a new model.
            DJV_API static std::shared_ptr<SettingsModel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                float displayScaleDefault);

            //! Save the settings. Settings are also saved on exit.
            DJV_API void save();

            //! Reset to default values.
            DJV_API void reset();

            //! \name Audio
            ///@{

            DJV_API const AudioSettings& getAudio() const;
            DJV_API std::shared_ptr<ftk::IObservable<AudioSettings> > observeAudio() const;
            DJV_API void setAudio(const AudioSettings&);

            ///@}

            //! \name Cache
            ///@{

            DJV_API const tl::PlayerCacheOptions& getCache() const;
            DJV_API std::shared_ptr<ftk::IObservable<tl::PlayerCacheOptions> > observeCache() const;
            DJV_API void setCache(const tl::PlayerCacheOptions&);

            DJV_API const tl::ui::ThumbnailCacheOptions& getThumbnailCache() const;
            DJV_API std::shared_ptr<ftk::IObservable<tl::ui::ThumbnailCacheOptions> > observeThumbnailCache() const;
            DJV_API void setThumbnailCache(const tl::ui::ThumbnailCacheOptions&);

            ///@}

            //! \name Export
            ///@{

            DJV_API const ExportSettings& getExport() const;
            DJV_API std::shared_ptr<ftk::IObservable<ExportSettings> > observeExport() const;
            DJV_API void setExport(const ExportSettings&);

            ///@}

            //! \name File Browser
            ///@{

            DJV_API const FileBrowserSettings& getFileBrowser() const;
            DJV_API std::shared_ptr<ftk::IObservable<FileBrowserSettings> > observeFileBrowser() const;
            DJV_API void setFileBrowser(const FileBrowserSettings&);

            ///@}

            //! \name Image Sequences
            ///@{

            DJV_API const ImageSeqSettings& getImageSeq() const;
            DJV_API std::shared_ptr<ftk::IObservable<ImageSeqSettings> > observeImageSeq() const;
            DJV_API void setImageSeq(const ImageSeqSettings&);

            ///@}

            //! \name OTIO
            ///@{

            DJV_API const OTIOSettings& getOTIO() const;
            DJV_API std::shared_ptr<ftk::IObservable<OTIOSettings> > observeOTIO() const;
            DJV_API void setOTIO(const OTIOSettings&);

            ///@}

            //! \name Miscellaneous
            ///@{

            DJV_API const MiscSettings& getMisc() const;
            DJV_API std::shared_ptr<ftk::IObservable<MiscSettings> > observeMisc() const;
            DJV_API void setMisc(const MiscSettings&);

            ///@}

            //! \name Mouse
            ///@{

            DJV_API const MouseSettings& getMouse() const;
            DJV_API std::shared_ptr<ftk::IObservable<MouseSettings> > observeMouse() const;
            DJV_API void setMouse(const MouseSettings&);

            ///@}

            //! \name Playback
            ///@{

            DJV_API const PlaybackSettings& getPlayback() const;
            DJV_API std::shared_ptr<ftk::IObservable<PlaybackSettings> > observePlayback() const;
            DJV_API void setPlayback(const PlaybackSettings&);

            ///@}

            //! \name Keyboard Shortcuts
            ///@{

            DJV_API const ShortcutsSettings& getShortcuts() const;
            DJV_API std::shared_ptr<ftk::IObservable<ShortcutsSettings> > observeShortcuts() const;
            DJV_API void setShortcuts(const ShortcutsSettings&);

            //! Add keyboard shortcuts. Any saved key bindings are applied to
            //! the added shortcuts.
            DJV_API void addShortcuts(const std::vector<Shortcut>&);

            ///@}

            //! \name Style
            ///@{

            DJV_API const StyleSettings& getStyle() const;
            DJV_API std::shared_ptr<ftk::IObservable<StyleSettings> > observeStyle() const;
            DJV_API void setStyle(const StyleSettings&);

            ///@}

            //! \name Timeline
            ///@{

            DJV_API const TimelineSettings& getTimeline() const;
            DJV_API std::shared_ptr<ftk::IObservable<TimelineSettings> > observeTimeline() const;
            DJV_API void setTimeline(const TimelineSettings&);

            ///@}

            //! \name Window
            ///@{

            DJV_API const WindowSettings& getWindow() const;
            DJV_API std::shared_ptr<ftk::IObservable<WindowSettings> > observeWindow() const;
            DJV_API void setWindow(const WindowSettings&);

            ///@}

#if defined(TLRENDER_FFMPEG_PLUGIN)
            //! \name FFmpeg
            ///@{

            DJV_API const tl::ffmpeg::Options& getFFmpeg() const;
            DJV_API std::shared_ptr<ftk::IObservable<tl::ffmpeg::Options> > observeFFmpeg() const;
            DJV_API void setFFmpeg(const tl::ffmpeg::Options&);

            ///@}
#endif // TLRENDER_FFMPEG_PLUGIN

#if defined(TLRENDER_FFMPEG_CMD)
            //! \name FFmpeg Command
            ///@{

            DJV_API const tl::ffmpeg_cmd::Options& getFFmpegCmd() const;
            DJV_API std::shared_ptr<ftk::IObservable<tl::ffmpeg_cmd::Options> > observeFFmpegCmd() const;
            DJV_API void setFFmpegCmd(const tl::ffmpeg_cmd::Options&);

            ///@}
#endif // TLRENDER_FFMPEG_CMD

#if defined(TLRENDER_USD)
            //! \name USD
            ///@{

            DJV_API const tl::usd::Options& getUSD() const;
            DJV_API std::shared_ptr<ftk::IObservable<tl::usd::Options> > observeUSD() const;
            DJV_API void setUSD(const tl::usd::Options&);

            ///@}
#endif // TLRENDER_USD

            //! \name I/O Options
            ///@{

            DJV_API tl::IOOptions getIOOptions() const;

            ///@}

        private:
            FTK_PRIVATE();
        };

        //! \name Serialize
        ///@{

        DJV_API void to_json(nlohmann::json&, const AudioSettings&);
        DJV_API void to_json(nlohmann::json&, const ExportSettings&);
        DJV_API void to_json(nlohmann::json&, const FileBrowserSettings&);
        DJV_API void to_json(nlohmann::json&, const ImageSeqSettings&);
        DJV_API void to_json(nlohmann::json&, const OTIOSettings&);
        DJV_API void to_json(nlohmann::json&, const MiscSettings&);
        DJV_API void to_json(nlohmann::json&, const MouseActionBinding&);
        DJV_API void to_json(nlohmann::json&, const MouseSettings&);
        DJV_API void to_json(nlohmann::json&, const PlaybackSettings&);
        DJV_API void to_json(nlohmann::json&, const ShortcutsSettings&);
        DJV_API void to_json(nlohmann::json&, const StyleSettings&);
        DJV_API void to_json(nlohmann::json&, const TimelineSettings&);
        DJV_API void to_json(nlohmann::json&, const WindowSettings&);

        DJV_API void from_json(const nlohmann::json&, AudioSettings&);
        DJV_API void from_json(const nlohmann::json&, ExportSettings&);
        DJV_API void from_json(const nlohmann::json&, FileBrowserSettings&);
        DJV_API void from_json(const nlohmann::json&, ImageSeqSettings&);
        DJV_API void from_json(const nlohmann::json&, OTIOSettings&);
        DJV_API void from_json(const nlohmann::json&, MiscSettings&);
        DJV_API void from_json(const nlohmann::json&, MouseActionBinding&);
        DJV_API void from_json(const nlohmann::json&, MouseSettings&);
        DJV_API void from_json(const nlohmann::json&, PlaybackSettings&);
        DJV_API void from_json(const nlohmann::json&, ShortcutsSettings&);
        DJV_API void from_json(const nlohmann::json&, StyleSettings&);
        DJV_API void from_json(const nlohmann::json&, TimelineSettings&);
        DJV_API void from_json(const nlohmann::json&, WindowSettings&);

        ///@}
    }
}
