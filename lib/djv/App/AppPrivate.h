// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/App.h>

#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/SettingsKeys.h>
#include <djv/Models/AppInfoModel.h>
#include <djv/Models/AudioModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/Playlist.h>

#include <djv/Models/MarkersModel.h>
#include <djv/Models/RecentFilesModel.h>
#include <djv/Models/Review.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/CommandsModel.h>
#include <djv/Models/ToolsModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/Timeline/CompareOptions.h>
#if defined(TLRENDER_FFMPEG_PLUGIN)
#include <tlRender/IO/FFmpeg.h>
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_USD)
#include <tlRender/IO/USD.h>
#endif // TLRENDER_USD

#include <ftk/UI/SysLogModel.h>
#include <ftk/Core/CmdLine.h>
#include <ftk/Core/Timer.h>

#include <filesystem>
#include <optional>

namespace djv
{
    namespace ui
    {
        class ColorResetDialog;
        class SeparateAudioDialog;
    }

    namespace app
    {
        class MainWindow;
        class SecondaryWindow;
        class ToolWidgetFactory;

        struct CmdLine
        {
            std::shared_ptr<ftk::CmdLineListArg<std::string> > inputs;
            std::shared_ptr<ftk::CmdLineOption<std::string> > audioFileName;
            std::shared_ptr<ftk::CmdLineOption<std::string> > mediaReference;
            std::shared_ptr<ftk::CmdLineOption<std::string> > compareFileName;
            std::shared_ptr<ftk::CmdLineOption<tl::Compare> > compare;
            std::shared_ptr<ftk::CmdLineOption<ftk::V2F> > wipeCenter;
            std::shared_ptr<ftk::CmdLineOption<float> > wipeRotation;
            std::shared_ptr<ftk::CmdLineOption<std::string> > frameRange;
            std::shared_ptr<ftk::CmdLineOption<std::string> > dirFilter;
            std::shared_ptr<ftk::CmdLineOption<int> > dirDepth;
            std::shared_ptr<ftk::CmdLineOption<double> > speed;
            std::shared_ptr<ftk::CmdLineOption<tl::Playback> > playback;
            std::shared_ptr<ftk::CmdLineOption<tl::Loop> > loop;
            std::shared_ptr<ftk::CmdLineOption<tl::TimeUnits> > timeUnits;
            std::shared_ptr<ftk::CmdLineOption<std::string> > seek;
            std::shared_ptr<ftk::CmdLineOption<std::string> > inPoint;
            std::shared_ptr<ftk::CmdLineOption<std::string> > outPoint;
            std::shared_ptr<ftk::CmdLineOption<float> > cacheVideoGB;
            std::shared_ptr<ftk::CmdLineOption<float> > cacheAudioGB;
#if defined(TLRENDER_FFMPEG_PLUGIN)
            std::shared_ptr<ftk::CmdLineOption<int> > ffmpegThreadCount;
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_OCIO)
            std::shared_ptr<ftk::CmdLineOption<std::string> > ocioFileName;
            std::shared_ptr<ftk::CmdLineOption<std::string> > ocioInput;
            std::shared_ptr<ftk::CmdLineOption<std::string> > ocioDisplay;
            std::shared_ptr<ftk::CmdLineOption<std::string> > ocioView;
            std::shared_ptr<ftk::CmdLineOption<std::string> > ocioLook;
            std::shared_ptr<ftk::CmdLineOption<std::string> > lutFileName;
            std::shared_ptr<ftk::CmdLineOption<tl::LUTOrder> > lutOrder;
#endif // TLRENDER_OCIO
#if defined(TLRENDER_USD)
            std::shared_ptr<ftk::CmdLineOption<int> > usdRenderWidth;
            std::shared_ptr<ftk::CmdLineOption<float> > usdComplexity;
            std::shared_ptr<ftk::CmdLineOption<tl::usd::DrawMode> > usdDrawMode;
            std::shared_ptr<ftk::CmdLineOption<bool> > usdEnableLighting;
            std::shared_ptr<ftk::CmdLineOption<bool> > usdSRGB;
            std::shared_ptr<ftk::CmdLineOption<int> > usdStageCacheCount;
            std::shared_ptr<ftk::CmdLineOption<int> > usdDiskCacheGB;
#endif // TLRENDER_USD
            std::shared_ptr<ftk::CmdLineFlag> hideSetup;
            std::shared_ptr<ftk::CmdLineFlag> version;
            std::shared_ptr<ftk::CmdLineFlag> sysInfo;
            std::shared_ptr<ftk::CmdLineFlag> listCommands;
            std::shared_ptr<ftk::CmdLineListOption<std::string> > command;
            std::shared_ptr<ftk::CmdLineOption<int> > debugLoop;
            std::shared_ptr<ftk::CmdLineOption<double> > benchmark;
        };

        struct App::Private
        {
            CmdLine cmdLine;

            std::shared_ptr<models::AppInfoModel> appInfoModel;
            std::shared_ptr<models::SettingsModel> settingsModel;
            std::shared_ptr<ftk::SysLogModel> sysLogModel;
            std::shared_ptr<models::TimeUnitsModel> timeUnitsModel;
            std::shared_ptr<models::FilesModel> filesModel;
            std::vector<std::shared_ptr<models::FilesModelItem> > files;
            std::vector<std::shared_ptr<models::FilesModelItem> > activeFiles;
            std::shared_ptr<models::RecentFilesModel> recentFilesModel;
            std::shared_ptr<models::RecentFilesModel> recentReviewsModel;
            std::shared_ptr<models::RecentFilesModel> recentPlaylistsModel;
            std::shared_ptr<models::RecentFilesModel> recentDirsModel;
            std::filesystem::path reviewPath;
            nlohmann::json reviewRaw;
            //! What the open review could not be read from, carried alongside
            //! the raw document so that saving puts it back rather than
            //! overwriting it with the defaults we fell back to.
            std::vector<std::string> reviewUnreadSections;
            nlohmann::json reviewUnreadItems;
            bool reviewModified = false;
            //! What the session summary reports at exit.
            size_t filesOpened = 0;
            size_t droppedFrames = 0;
            std::optional<models::ReviewView> pendingReviewView;
            std::shared_ptr<ftk::Timer> reviewViewTimer;
            std::shared_ptr<ftk::Timer> autosaveTimer;
            std::optional<nlohmann::json> recoveredAutosave;
            std::vector<std::shared_ptr<tl::Timeline> > timelines;
            //! Whether opening a timeline has filled in what an item holds
            //! since the files were last announced.
            bool filesChanged = false;
            std::shared_ptr<ftk::Observable<std::shared_ptr<tl::Player> > > player;
            std::shared_ptr<models::ColorModel> colorModel;
            std::shared_ptr<ui::ColorResetDialog> colorResetDialog;
            std::shared_ptr<models::ViewportModel> viewportModel;
            std::shared_ptr<models::AudioModel> audioModel;
            bool audioDeviceMute = false;
            std::shared_ptr<models::ToolsModel> toolsModel;
            std::shared_ptr<models::CommandsModel> commandsModel;
            std::shared_ptr<models::MarkersModel> markersModel;
            std::shared_ptr<models::AnnotationsModel> annotationsModel;
            std::shared_ptr<models::DrawModel> drawModel;
            std::shared_ptr<ftk::ObservableList<int> > reviewMarkers;

            std::shared_ptr<ftk::Observable<bool> > secondaryWindowActive;
            std::shared_ptr<ToolWidgetFactory> toolWidgetFactory;
            std::shared_ptr<MainWindow> mainWindow;
            std::shared_ptr<SecondaryWindow> secondaryWindow;
            std::shared_ptr<ui::SeparateAudioDialog> separateAudioDialog;

            //! What marks the review modified. Everything the document holds
            //! and a person would call a change to it: the files, what is
            //! being compared, how the image is shown, and the feedback. Not
            //! the playhead, the view or the open panels, which move
            //! constantly and belong to whoever is looking rather than to
            //! the review. Every one of them suppresses its first callback:
            //! the state the models start in is the session as it was
            //! opened, not an edit to it.
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsModifiedObserver;
            std::shared_ptr<ftk::ListObserver<int> > bIndexesModifiedObserver;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesModifiedObserver;
            std::shared_ptr<ftk::Observer<int> > aIndexModifiedObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<models::FilesModelItem> > > windowTitleObserver;
            std::shared_ptr<ftk::ListObserver<int> > layersModifiedObserver;
            std::shared_ptr<ftk::ListObserver<std::string> > mediaReferenceKeysModifiedObserver;
            std::shared_ptr<ftk::Observer<tl::CompareTime> > compareTimeModifiedObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioLogObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutLogObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioModifiedObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutModifiedObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayModifiedObserver;
            std::shared_ptr<ftk::Observer<tl::BackgroundOptions> > backgroundModifiedObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > foregroundModifiedObserver;
            std::shared_ptr<ftk::Observer<models::AspectRatioOptions> > aspectRatioModifiedObserver;
            std::shared_ptr<ftk::Observer<models::HUDOptions> > hudModifiedObserver;
            std::shared_ptr<ftk::ListObserver<models::ReviewMarker> > markersModifiedObserver;
            std::shared_ptr<ftk::ListObserver<models::ReviewAnnotation> > annotationsModifiedObserver;
            std::shared_ptr<ftk::ListObserver<models::ReviewMarker> > markersObserver;
            std::shared_ptr<ftk::ListObserver<models::ReviewAnnotation> > annotationsObserver;
            std::shared_ptr<ftk::ListObserver<std::string> > drawToolsObserver;

            std::shared_ptr<ftk::Observer<tl::PlayerCacheOptions> > cacheObserver;
            std::shared_ptr<ftk::Observer<models::AudioSettings> > audioSettingsObserver;
            std::shared_ptr<ftk::Observer<models::ImageSeqSettings> > imageSeqObserver;
            std::shared_ptr<ftk::Observer<tl::IOOptions> > ioOptionsObserver;
            // The policy the open files were built with, so that a change to
            // or from Skip can be told apart from the rest.
            tl::MissingFrames missingFrames = tl::MissingFrames::First;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<models::FilesModelItem> > > reloadObserver;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > activeObserver;
            std::shared_ptr<ftk::ListObserver<int> > layersObserver;
            std::shared_ptr<ftk::ListObserver<std::string> > mediaReferenceKeysObserver;
            std::shared_ptr<ftk::Observer<tl::CompareTime> > compareTimeObserver;
            std::shared_ptr<ftk::Observer<std::pair<ftk::V2I, double> > > viewPosZoomObserver;
            std::shared_ptr<ftk::Observer<bool> > viewFramedObserver;
            std::shared_ptr<ftk::Observer<tl::AudioDeviceID> > audioDeviceObserver;
            std::shared_ptr<ftk::Observer<float> > volumeObserver;
            std::shared_ptr<ftk::Observer<bool> > muteObserver;
            std::shared_ptr<ftk::Observer<int> > channelObserver;
            std::shared_ptr<ftk::Observer<double> > syncOffsetObserver;
            std::shared_ptr<ftk::Observer<models::StyleSettings> > styleSettingsObserver;
            std::shared_ptr<ftk::Observer<models::MiscSettings> > miscSettingsObserver;

            std::shared_ptr<ftk::Timer> debugTimer;
            int debugInput = 0;
#if defined(__GLIBC__)
            std::shared_ptr<ftk::Timer> trimTimer;
#endif // __GLIBC__

            std::shared_ptr<ftk::Timer> commandTimer;
            //! Files whose timeline could not be created, closed on a
            //! later tick: closing publishes the file list again, and
            //! doing that while handling the list is what crashes.
            std::vector<std::shared_ptr<models::FilesModelItem> > failedFiles;
            std::shared_ptr<ftk::Timer> closeFailedTimer;
            int commandTicks = 0;
        };
    }
}
