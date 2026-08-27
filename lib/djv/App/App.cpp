// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/App.h>

#include <djv/App/AudioTool.h>
#include <djv/App/Benchmark.h>
#include <djv/App/Capture.h>
#include <djv/App/ColorPickerTool.h>
#include <djv/App/ColorTool.h>
#include <djv/App/DiagTool.h>
#include <djv/App/ExportTool.h>
#include <djv/App/FilesTool.h>
#include <djv/App/InfoTool.h>
#include <djv/App/MagnifyTool.h>
#include <djv/App/MainWindow.h>
#include <djv/App/MessagesTool.h>
#include <djv/App/ReviewTool.h>
#include <djv/App/SecondaryWindow.h>
#include <djv/App/SettingsTool.h>
#include <djv/App/SysLogTool.h>
#include <djv/App/ViewTool.h>
#include <djv/UI/Viewport.h>
#include <djv/UI/SeparateAudioDialog.h>
#include <djv/UI/StatusIndicator.h>
#include <djv/UI/SysInfoDialog.h>
#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/AppInfoModel.h>
#include <djv/Models/AudioModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/Parse.h>
#include <djv/Models/Playlist.h>

#include <opentimelineio/clip.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/stack.h>
#include <opentimelineio/track.h>
#include <djv/Models/MarkersModel.h>
#include <djv/Models/RecentFilesModel.h>
#include <djv/Models/Review.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/CommandsModel.h>
#include <djv/Models/ToolsModel.h>
#include <djv/Models/Version.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/UI/ThumbnailSystem.h>
#include <tlRender/UI/TimelineWidget.h>
#include <tlRender/Timeline/ColorOptions.h>
#include <tlRender/Timeline/CompareOptions.h>
#include <tlRender/Timeline/Util.h>
#include <tlRender/IO/Plugin.h>
#include <tlRender/IO/System.h>
#if defined(TLRENDER_FFMPEG_PLUGIN)
#include <tlRender/IO/FFmpeg.h>
#endif // TLRENDER_FFMPEG_PLUGIN

#include <ftk/GL/Window.h>
#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/FileBrowser.h>
#include <ftk/UI/Settings.h>
#include <ftk/UI/SysLogModel.h>
#include <ftk/Core/CmdLine.h>
#include <ftk/Core/FileLogSystem.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/OS.h>
#include <ftk/Core/Timer.h>

#include <ctime>
#include <ftk/Core/Path.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <optional>

#if defined(__GLIBC__)
#include <malloc.h>
#endif // __GLIBC__

namespace djv
{
    namespace app
    {
        struct CmdLine
        {
            std::shared_ptr<ftk::CmdLineListArg<std::string> > inputs;
            std::shared_ptr<ftk::CmdLineOption<std::string> > audioFileName;
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
#if defined(TLRENDER_OCIO)
            std::shared_ptr<ftk::CmdLineOption<std::string> > ocioFileName;
            std::shared_ptr<ftk::CmdLineOption<std::string> > ocioInput;
            std::shared_ptr<ftk::CmdLineOption<std::string> > ocioDisplay;
            std::shared_ptr<ftk::CmdLineOption<std::string> > ocioView;
            std::shared_ptr<ftk::CmdLineOption<std::string> > ocioLook;
            std::shared_ptr<ftk::CmdLineOption<std::string> > lutFileName;
            std::shared_ptr<ftk::CmdLineOption<tl::LUTOrder> > lutOrder;
#endif // TLRENDER_OCIO
            std::shared_ptr<ftk::CmdLineFlag> hideSetup;
            std::shared_ptr<ftk::CmdLineFlag> version;
            std::shared_ptr<ftk::CmdLineFlag> sysInfo;
            std::shared_ptr<ftk::CmdLineFlag> listCommands;
            std::shared_ptr<ftk::CmdLineListOption<std::string> > command;
            std::shared_ptr<ftk::CmdLineOption<int> > debugLoop;
            std::shared_ptr<ftk::CmdLineOption<double> > benchmark;
            std::shared_ptr<ftk::CmdLineOption<std::string> > captureManifest;
            std::shared_ptr<ftk::CmdLineOption<std::string> > captureShot;
            std::shared_ptr<ftk::CmdLineOption<std::string> > captureOutput;
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
            std::filesystem::path reviewPath;
            nlohmann::json reviewRaw;
            //! What the open review could not be read from, carried alongside
            //! the raw document so that saving puts it back rather than
            //! overwriting it with the defaults we fell back to.
            std::vector<std::string> reviewUnreadSections;
            nlohmann::json reviewUnreadItems;
            bool reviewModified = false;
            std::optional<models::ReviewView> pendingReviewView;
            std::shared_ptr<ftk::Timer> reviewViewTimer;
            std::shared_ptr<ftk::Timer> autosaveTimer;
            std::optional<nlohmann::json> recoveredAutosave;
            std::vector<std::shared_ptr<tl::Timeline> > timelines;
            std::shared_ptr<ftk::Observable<std::shared_ptr<tl::Player> > > player;
            std::shared_ptr<models::ColorModel> colorModel;
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

            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsModifiedObserver;
            std::shared_ptr<ftk::ListObserver<int> > bIndexesModifiedObserver;
            std::shared_ptr<ftk::ListObserver<models::ReviewMarker> > markersObserver;
            std::shared_ptr<ftk::ListObserver<models::ReviewAnnotation> > annotationsObserver;
            std::shared_ptr<ftk::ListObserver<std::string> > drawToolsObserver;

            std::shared_ptr<ftk::Observer<tl::PlayerCacheOptions> > cacheObserver;
            std::shared_ptr<ftk::Observer<models::ImageSeqSettings> > imageSeqObserver;
            // The policy the open files were built with, so that a change to
            // or from Skip can be told apart from the rest.
            tl::MissingFrames missingFrames = tl::MissingFrames::First;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<models::FilesModelItem> > > reloadObserver;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > activeObserver;
            std::shared_ptr<ftk::ListObserver<int> > layersObserver;
            std::shared_ptr<ftk::Observer<tl::CompareTime> > compareTimeObserver;
            std::shared_ptr<ftk::Observer<std::pair<ftk::V2I, double> > > viewPosZoomObserver;
            std::shared_ptr<ftk::Observer<bool> > viewFramedObserver;
            std::shared_ptr<ftk::Observer<tl::AudioDeviceID> > audioDeviceObserver;
            std::shared_ptr<ftk::Observer<float> > volumeObserver;
            std::shared_ptr<ftk::Observer<bool> > muteObserver;
            std::shared_ptr<ftk::ListObserver<bool> > channelMuteObserver;
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

        void App::_init(
            const std::shared_ptr<ftk::Context>& context,
            std::vector<std::string>& argv,
            const std::shared_ptr<models::AppInfoModel>& appInfoModel,
            const std::vector<std::shared_ptr<ftk::ICmdLineOption> >& options)
        {
            FTK_P();

            p.appInfoModel = appInfoModel ? appInfoModel : models::AppInfoModel::create();

            p.cmdLine.inputs = ftk::CmdLineListArg<std::string>::create(
                "input",
                "One or more timelines, movies, image sequences, or directories.",
                true);
            p.cmdLine.audioFileName = ftk::CmdLineOption<std::string>::create(
                { "-audio", "-a" },
                "Audio file name.",
                "Audio");
            p.cmdLine.compareFileName = ftk::CmdLineOption<std::string>::create(
                { "-compare", "-b" },
                "Compare \"B\" file name.",
                "Compare");
            p.cmdLine.compare = ftk::CmdLineOption<tl::Compare>::create(
                { "-compareMode", "-c" },
                "Compare mode.",
                "Compare",
                std::optional<tl::Compare>(),
                ftk::quotes(tl::getCompareLabels()));
            p.cmdLine.wipeCenter = ftk::CmdLineOption<ftk::V2F>::create(
                { "-wipeCenter", "-wc" },
                "Wipe center.",
                "Compare",
                tl::CompareOptions().wipeCenter);
            p.cmdLine.wipeRotation = ftk::CmdLineOption<float>::create(
                { "-wipeRotation", "-wr" },
                "Wipe rotation.",
                "Compare",
                0.F);
            p.cmdLine.frameRange = ftk::CmdLineOption<std::string>::create(
                { "-frameRange", "-fr" },
                "Frame range of an image sequence (e.g., 1-100). This is the "
                "range the sequence is meant to cover, which need not be the "
                "frames on disk: a render in progress can be watched over the "
                "range it will end up with, the frames that are not there yet "
                "following the missing frames setting. Applies to the first "
                "file opened.",
                "Playback");
            p.cmdLine.dirFilter = ftk::CmdLineOption<std::string>::create(
                { "-dirFilter" },
                "Filter the files when opening a directory: a "
                "case-insensitive substring, or a wildcard pattern with "
                "\"*\" and \"?\" (e.g., \"*.mov\").",
                "Directories");
            p.cmdLine.dirDepth = ftk::CmdLineOption<int>::create(
                { "-dirDepth" },
                "How many directory levels to open: 1 opens the directory "
                "alone.",
                "Directories",
                1);
            p.cmdLine.speed = ftk::CmdLineOption<double>::create(
                { "-speed" },
                "Playback speed.",
                "Playback");
            p.cmdLine.playback = ftk::CmdLineOption<tl::Playback>::create(
                { "-playback", "-p" },
                "Playback mode.",
                "Playback",
                std::optional<tl::Playback>(),
                ftk::quotes(tl::getPlaybackLabels()));
            p.cmdLine.loop = ftk::CmdLineOption<tl::Loop>::create(
                { "-loop" },
                "Loop mode.",
                "Playback",
                std::optional<tl::Loop>(),
                ftk::quotes(tl::getLoopLabels()));
            p.cmdLine.timeUnits = ftk::CmdLineOption<tl::TimeUnits>::create(
                { "-timeUnits", "-tu" },
                "Set the time units.",
                "Playback",
                std::optional<tl::TimeUnits>(),
                ftk::quotes(tl::getTimeUnitsLabels()));
            p.cmdLine.seek = ftk::CmdLineOption<std::string>::create(
                { "-seek" },
                "Seek to the given time.",
                "Playback");
            p.cmdLine.inPoint = ftk::CmdLineOption<std::string>::create(
                { "-inPoint", "-in" },
                "Set the in point.",
                "Playback");
            p.cmdLine.outPoint = ftk::CmdLineOption<std::string>::create(
                { "-outPoint", "-out" },
                "Set the out point.",
                "Playback");
            // Offered only where there is something behind them: without
            // OCIO the color options are accepted and then quietly do
            // nothing, which reads as a broken build rather than one made
            // without color management.
#if defined(TLRENDER_OCIO)
            p.cmdLine.ocioFileName = ftk::CmdLineOption<std::string>::create(
                { "-ocio" },
                "OCIO configuration file name (e.g., config.ocio).",
                "Color");
            p.cmdLine.ocioInput = ftk::CmdLineOption<std::string>::create(
                { "-ocioInput" },
                "OCIO input name.",
                "Color");
            p.cmdLine.ocioDisplay = ftk::CmdLineOption<std::string>::create(
                { "-ocioDisplay" },
                "OCIO display name.",
                "Color");
            p.cmdLine.ocioView = ftk::CmdLineOption<std::string>::create(
                { "-ocioView" },
                "OCIO view name.",
                "Color");
            p.cmdLine.ocioLook = ftk::CmdLineOption<std::string>::create(
                { "-ocioLook" },
                "OCIO look name.",
                "Color");
            p.cmdLine.lutFileName = ftk::CmdLineOption<std::string>::create(
                { "-lut" },
                "LUT file name.",
                "Color");
            p.cmdLine.lutOrder = ftk::CmdLineOption<tl::LUTOrder>::create(
                { "-lutOrder" },
                "LUT operation order.",
                "Color",
                std::optional<tl::LUTOrder>(),
                ftk::quotes(tl::getLUTOrderLabels()));
#endif // TLRENDER_OCIO
            p.cmdLine.hideSetup = ftk::CmdLineFlag::create(
                { "-hideSetup" },
                "Hide the setup dialog that is shown on the first run.");
            p.cmdLine.version = ftk::CmdLineFlag::create(
                { "-version" },
                "Print the version and exit.");
            p.cmdLine.sysInfo = ftk::CmdLineFlag::create(
                { "-sysInfo" },
                "Print the system information and exit.");
            p.cmdLine.listCommands = ftk::CmdLineFlag::create(
                { "-listCommands" },
                "Print the list of commands and exit.");
            p.cmdLine.command = ftk::CmdLineListOption<std::string>::create(
                { "-command" },
                "Execute a command after startup. The command name may be "
                "followed by JSON arguments; e.g., \"Playback/Forward\" or "
                "\"Playback/Seek { \\\"frame\\\": 100 }\". This option may be "
                "repeated to execute multiple commands in order. Use "
                "-listCommands to see the available commands.",
                "Commands");
            p.cmdLine.debugLoop = ftk::CmdLineOption<int>::create(
                { "-debugLoop" },
                "Load the command line inputs in a loop. This value is the number of seconds for each cycle.",
                "Testing",
                10);
            p.cmdLine.benchmark = ftk::CmdLineOption<double>::create(
                { "-benchmark" },
                "Play headlessly for this many seconds and report the frame "
                "rate achieved.",
                "Benchmark",
                5.0);
            p.cmdLine.captureManifest = ftk::CmdLineOption<std::string>::create(
                { "-captureManifest" },
                "Screenshot manifest (JSON).",
                "Capture");
            p.cmdLine.captureShot = ftk::CmdLineOption<std::string>::create(
                { "-captureShot" },
                "Id of the single shot to capture.",
                "Capture");
            p.cmdLine.captureOutput = ftk::CmdLineOption<std::string>::create(
                { "-captureOutput" },
                "Output directory for PNG + JSON.", "Capture",
                std::string("."));

            std::vector<std::shared_ptr<ftk::ICmdLineOption> > cmdLineOptions =
            {
                p.cmdLine.audioFileName,
                p.cmdLine.compareFileName,
                p.cmdLine.compare,
                p.cmdLine.wipeCenter,
                p.cmdLine.wipeRotation,
                p.cmdLine.speed,
                p.cmdLine.playback,
                p.cmdLine.loop,
                p.cmdLine.timeUnits,
                p.cmdLine.seek,
                p.cmdLine.frameRange,
                p.cmdLine.inPoint,
                p.cmdLine.outPoint,
                p.cmdLine.dirFilter,
                p.cmdLine.dirDepth,
#if defined(TLRENDER_OCIO)
                p.cmdLine.ocioFileName,
                p.cmdLine.ocioInput,
                p.cmdLine.ocioDisplay,
                p.cmdLine.ocioView,
                p.cmdLine.ocioLook,
                p.cmdLine.lutFileName,
                p.cmdLine.lutOrder,
#endif // TLRENDER_OCIO
                p.cmdLine.hideSetup,
                p.cmdLine.version,
                p.cmdLine.sysInfo,
                p.cmdLine.listCommands,
                p.cmdLine.command,
                p.cmdLine.debugLoop,
                p.cmdLine.benchmark,
                p.cmdLine.captureManifest,
                p.cmdLine.captureShot,
                p.cmdLine.captureOutput
            };
            cmdLineOptions.insert(
                cmdLineOptions.end(), options.begin(), options.end());

            ftk::App::_init(
                context,
                argv,
                p.appInfoModel->getShortName(),
                "Media playback and review.",
                { p.cmdLine.inputs },
                cmdLineOptions,
                ftk::AppFiles{
                    p.appInfoModel->getDocsDirName(),
                    p.appInfoModel->getShortName(),
                    p.appInfoModel->getVersionMajor() });
        }

        App::App() :
            _p(new Private)
        {}

        App::~App()
        {}

        std::shared_ptr<App> App::create(
            const std::shared_ptr<ftk::Context>& context,
            std::vector<std::string>& argv,
            const std::shared_ptr<models::AppInfoModel>& appInfoModel)
        {
            auto out = std::shared_ptr<App>(new App);
            out->_init(context, argv, appInfoModel);
            return out;
        }

        const std::shared_ptr<models::AppInfoModel>& App::getAppInfoModel() const
        {
            return _p->appInfoModel;
        }


        const std::shared_ptr<models::SettingsModel>& App::getSettingsModel() const
        {
            return _p->settingsModel;
        }

        const std::shared_ptr<ftk::SysLogModel>& App::getSysLogModel() const
        {
            return _p->sysLogModel;
        }

        const std::shared_ptr<models::TimeUnitsModel>& App::getTimeUnitsModel() const
        {
            return _p->timeUnitsModel;
        }

        const std::shared_ptr<models::FilesModel>& App::getFilesModel() const
        {
            return _p->filesModel;
        }

        const std::shared_ptr<models::RecentFilesModel>& App::getRecentFilesModel() const
        {
            return _p->recentFilesModel;
        }

        const std::shared_ptr<models::ColorModel>& App::getColorModel() const
        {
            return _p->colorModel;
        }

        const std::shared_ptr<models::ViewportModel>& App::getViewportModel() const
        {
            return _p->viewportModel;
        }

        const std::shared_ptr<models::AudioModel>& App::getAudioModel() const
        {
            return _p->audioModel;
        }

        const std::shared_ptr<models::ToolsModel>& App::getToolsModel() const
        {
            return _p->toolsModel;
        }

        const std::shared_ptr<models::CommandsModel>& App::getCommandsModel() const
        {
            return _p->commandsModel;
        }

        const std::shared_ptr<models::MarkersModel>& App::getMarkersModel() const
        {
            return _p->markersModel;
        }

        const std::shared_ptr<models::AnnotationsModel>& App::getAnnotationsModel() const
        {
            return _p->annotationsModel;
        }

        const std::shared_ptr<models::DrawModel>& App::getDrawModel() const
        {
            return _p->drawModel;
        }

        std::shared_ptr<ftk::IObservableList<int> > App::observeReviewMarkers() const
        {
            return _p->reviewMarkers;
        }

        void App::seekReviewMarker(bool next)
        {
            FTK_P();
            // The list is sorted and deduplicated by _markersUpdate().
            const auto& markers = p.reviewMarkers->get();
            auto player = p.player->get();
            if (markers.empty() || !player)
            {
                return;
            }
            const OTIO_NS::RationalTime currentTime = player->getCurrentTime();
            const int current = static_cast<int>(currentTime.value());
            int target = 0;
            if (next)
            {
                // The first marker strictly after the playhead, or wrap around
                // to the first one so the button never becomes a dead end.
                const auto i = std::upper_bound(markers.begin(), markers.end(), current);
                target = i != markers.end() ? *i : markers.front();
            }
            else
            {
                const auto i = std::lower_bound(markers.begin(), markers.end(), current);
                target = i != markers.begin() ? *(i - 1) : markers.back();
            }
            player->stop();
            const OTIO_NS::RationalTime targetTime(target, currentTime.rate());
            // Going to feedback wins over a narrower in/out range: with the
            // target outside it, the seek would move the clock into a span
            // the player cannot show.
            if (!player->getInOutRange().contains(targetTime))
            {
                player->resetInPoint();
                player->resetOutPoint();
            }
            player->seek(targetTime);
        }

        bool App::getHideSetup() const
        {
            return
                _p->cmdLine.hideSetup->found() ||
                _p->cmdLine.listCommands->found() ||
                _p->cmdLine.command->found() ||
                _p->cmdLine.captureShot->found() ||
                _p->cmdLine.benchmark->found();
        }

        void App::openDialog()
        {
            FTK_P();

            // More than one at a time: opening a shot and the two versions
            // beside it is one trip to the browser rather than three, and
            // each arrives as its own file the way it would have alone.
            ftk::FileBrowserOpenOptions options;
            options.multiple = true;
            auto fileBrowserSystem = _context->getSystem<ftk::FileBrowserSystem>();
            fileBrowserSystem->open(
                p.mainWindow,
                [this, fileBrowserSystem](const std::vector<ftk::Path>& value)
                {
                    // A browser listing the frames of a sequence one by one
                    // is one to choose a frame from, so opening one opens
                    // that file rather than the sequence it belongs to.
                    const bool gatherSeq =
                        fileBrowserSystem->getModel()->getOptions().dirList.seq;
                    for (const auto& i : value)
                    {
                        open(i, ftk::Path(), std::optional<ftk::RangeI64>(), gatherSeq);
                    }
                },
                options);
        }

        void App::openPlaylist(const ftk::Path& path)
        {
            FTK_P();
            try
            {
                std::vector<std::string> report;
                const models::Playlist playlist = models::playlistOpen(
                    path.getFileName(true),
                    report);

                // Added to what is open rather than replacing it, the same
                // as opening anything else; the playlist's own A/B indexes
                // are offsets into what it added.
                const int offset = static_cast<int>(
                    p.filesModel->getFiles().size());
                p.filesModel->add(playlist.items);
                p.recentPlaylistsModel->addRecent(path);
                if (playlist.aIndex >= 0)
                {
                    p.filesModel->setA(offset + playlist.aIndex);
                }
                for (int b : playlist.bIndexes)
                {
                    p.filesModel->setB(offset + b, true);
                }
                p.filesModel->setCompareOptions(playlist.compareOptions);
                p.filesModel->setCompareTime(playlist.compareTime);
                p.recentFilesModel->addRecent(path);

                if (!report.empty())
                {
                    // A warning so the status bar shows it: what the file
                    // list could not carry was dropped, and saying nothing
                    // would look like it was.
                    _context->log(
                        "djv::app::App",
                        ftk::Format("{0}: {1}").
                            arg(path.getFileName()).
                            arg(ftk::join(report, ", ")),
                        ftk::LogType::Warning);
                }
            }
            catch (const std::exception& e)
            {
                _context->log("djv::app::App", e.what(), ftk::LogType::Error);
            }
        }

        void App::openPlaylistDialog()
        {
            FTK_P();
            ftk::FileBrowserOpenOptions options;
            options.title = "Open Playlist";
            options.extensions.push_back(".otio");
            options.extensionsLabel = "Playlists";
            auto fileBrowserSystem = _context->getSystem<ftk::FileBrowserSystem>();
            fileBrowserSystem->open(
                p.mainWindow,
                [this](const ftk::Path& value)
                {
                    openPlaylist(value);
                },
                options);
        }

        void App::savePlaylist(const ftk::Path& path)
        {
            FTK_P();

            models::Playlist playlist;
            playlist.items = p.filesModel->getFiles();

            // The active file's position and in/out points live in the
            // player until the file loses focus, so bring its item up to
            // date before it is written.
            if (!p.activeFiles.empty())
            {
                if (auto player = p.player->get())
                {
                    p.activeFiles.front()->speed = player->getSpeed();
                    p.activeFiles.front()->currentTime = player->getCurrentTime();
                    p.activeFiles.front()->inOutRange = player->getInOutRange();
                }
            }

            playlist.aIndex = p.filesModel->getAIndex();
            playlist.bIndexes = p.filesModel->getBIndexes();
            playlist.compareOptions = p.filesModel->getCompareOptions();
            playlist.compareTime = p.filesModel->getCompareTime();

            std::string fileName = path.getFileName(true);
            if (".otio" != ftk::toLower(path.getExt()))
            {
                fileName += ".otio";
            }
            try
            {
                models::playlistSave(
                    fileName,
                    playlist,
                    p.settingsModel->getImageSeq().io.defaultSpeed);
            }
            catch (const std::exception& e)
            {
                _context->log("djv::app::App", e.what(), ftk::LogType::Error);
            }
            p.recentPlaylistsModel->addRecent(path);
        }

        void App::savePlaylistDialog()
        {
            FTK_P();
            ftk::FileBrowserOpenOptions options;
            options.title = "Save Playlist";
            options.mode = ftk::FileBrowserMode::Save;
            options.fileName = "playlist.otio";
            options.extensions.push_back(".otio");
            options.extensionsLabel = "Playlists";
            auto fileBrowserSystem = _context->getSystem<ftk::FileBrowserSystem>();
            fileBrowserSystem->open(
                p.mainWindow,
                [this](const ftk::Path& value)
                {
                    savePlaylist(value);
                },
                options);
        }

        void App::openSeparateAudioDialog()
        {
            FTK_P();
            p.separateAudioDialog = ui::SeparateAudioDialog::create(_context);
            p.separateAudioDialog->open(p.mainWindow);
            p.separateAudioDialog->setCallback(
                [this](const ftk::Path& value, const ftk::Path& audio)
                {
                    open(value, audio);
                    _p->separateAudioDialog->close();
                });
            p.separateAudioDialog->setCloseCallback(
                [this]
                {
                    _p->separateAudioDialog.reset();
                });
        }

        void App::open(
            const ftk::Path& path,
            const ftk::Path& audioPath,
            const std::optional<ftk::RangeI64>& frames,
            bool gatherSeq)
        {
            FTK_P();
            ftk::DirListOptions dirListOptions;
            dirListOptions.seqExts = tl::getExts(_context, static_cast<int>(tl::FileType::Seq));
            dirListOptions.seqMaxDigits = p.settingsModel->getImageSeq().maxDigits;
            // The command line said how directories are read; that holds
            // for the whole session, dialogs included.
            if (p.cmdLine.dirFilter->found())
            {
                dirListOptions.filter = p.cmdLine.dirFilter->getValue();
            }
            if (p.cmdLine.dirDepth->found())
            {
                dirListOptions.depth = std::max(1, p.cmdLine.dirDepth->getValue());
            }
            // Gathering a directory's frames into sequences and taking one
            // frame to name its sequence are the same thing said twice; a
            // stated range has already said what it wants.
            dirListOptions.seq = gatherSeq && !frames.has_value();
            bool first = true;
            for (const auto& i : tl::getPaths(_context, path, dirListOptions))
            {
                auto item = std::make_shared<models::FilesModelItem>();
                // Annotations reference their source by this identity, so it has
                // to exist from the moment the file is opened.
                item->id = models::generateId();
                item->path = i;
                if (first && frames.has_value())
                {
                    // Stated, so the frames on disk are not looked for and
                    // the range is used as it is. A directory gives several
                    // sequences and one range cannot describe them all, so
                    // only the first takes it.
                    item->path.setFrames(frames.value());
                    item->framesStated = true;
                }
                first = false;
                item->audioPath = audioPath;
                p.filesModel->add(item);
            }
        }

        namespace
        {
            //! Resolve a review file entry to a path on disk.
            std::filesystem::path resolveReviewFile(
                const models::ReviewFile& rf,
                const std::filesystem::path& base,
                const std::filesystem::path& substituteRoot,
                const ftk::PathOptions& pathOptions,
                bool& exists)
            {
                return models::resolveReviewPath(
                    rf.path, rf.pathAbsolute, base, substituteRoot, pathOptions, exists);
            }
        }

        void App::openReview(const std::filesystem::path& path)
        {
            FTK_P();

            // A timeline imports rather than opens: it becomes the review's
            // "A" source by reference, and its markers copy into the
            // feedback.
            const std::string ext = ftk::toLower(path.extension().u8string());
            if (".otio" == ext || ".otioz" == ext)
            {
                _importReviewTimeline(path);
                return;
            }

            models::Review review;
            try
            {
                review = models::reviewOpen(path.u8string());
            }
            catch (const std::exception& e)
            {
                _context->log("djv::app::App", e.what(), ftk::LogType::Error);
                return;
            }
            _logUnreadSections(review, path);

            _applyReview(review, path.parent_path(), path, std::filesystem::path());
        }

        void App::_logUnreadSections(
            const models::Review& review,
            const std::filesystem::path& path)
        {
            for (const auto& section : review.unreadSections)
            {
                _context->log(
                    "djv::app::App",
                    ftk::Format(
                        "Review \"{0}\": the \"{1}\" section could not be read "
                        "and is left at its defaults. It is kept as it stands "
                        "when the review is saved, not overwritten.").
                        arg(path.u8string()).
                        arg(section),
                    ftk::LogType::Warning);
            }
        }

        void App::_applyReview(
            const models::Review& review,
            const std::filesystem::path& base,
            const std::filesystem::path& reviewPath,
            const std::filesystem::path& substituteRoot)
        {
            FTK_P();

            ftk::PathOptions pathOptions;
            pathOptions.seqMaxDigits = p.settingsModel->getImageSeq().maxDigits;
            const std::vector<std::string> seqExts =
                tl::getExts(_context, static_cast<int>(tl::FileType::Seq));

            // Replace the current session.
            p.filesModel->closeAll();

            std::vector<std::string> missing;
            for (const auto& rf : review.files)
            {
                bool exists = false;
                const std::filesystem::path resolved =
                    resolveReviewFile(rf, base, substituteRoot, pathOptions, exists);
                if (!exists)
                {
                    missing.push_back(resolved.u8string());
                }
                auto item = std::make_shared<models::FilesModelItem>();
                item->id = rf.id.empty() ? models::generateId() : rf.id;
                item->path = ftk::Path(resolved.u8string(), pathOptions);
                // The review stores one frame's path, so the sequence is
                // gathered from the disk the way opening the frame would
                // gather it; without this the file restores as that one
                // frame.
                if (exists && item->path.testExt(seqExts))
                {
                    item->path = ftk::expandSeq(item->path, pathOptions);
                }
                if (!rf.audioPath.empty() || !rf.audioPathAbsolute.empty())
                {
                    bool audioExists = false;
                    const std::filesystem::path audio = models::resolveReviewPath(
                        rf.audioPath,
                        rf.audioPathAbsolute,
                        base,
                        substituteRoot,
                        pathOptions,
                        audioExists);
                    if (!audioExists)
                    {
                        missing.push_back(audio.u8string());
                    }
                    item->audioPath = ftk::Path(audio.u8string(), pathOptions);
                }
                item->videoLayer = static_cast<size_t>(std::max(0, rf.videoLayer));
                item->speed = rf.speed;
                item->currentTime = rf.currentTime;
                item->inOutRange = rf.inOutRange;
                // Add directly rather than through open(), which would re-expand a
                // directory entry into multiple files.
                p.filesModel->add(item);
            }

            // Rebuild the comparison. Order matters: setCompareOptions may pick a
            // "B" of its own when none is set, so clear and rebuild "B" after it,
            // then set "A".
            const auto& files = p.filesModel->getFiles();
            auto indexOfId = [&files](const std::string& id) -> int
            {
                for (int i = 0; i < static_cast<int>(files.size()); ++i)
                {
                    if (files[i]->id == id)
                    {
                        return i;
                    }
                }
                return -1;
            };
            p.filesModel->setCompareOptions(review.compare.options);
            p.filesModel->clearB();
            for (const auto& bId : review.compare.bIds)
            {
                const int index = indexOfId(bId);
                if (index >= 0)
                {
                    p.filesModel->setB(index, true);
                }
            }
            int aIndex = indexOfId(review.compare.aId);
            if (aIndex < 0 && !files.empty())
            {
                aIndex = 0;
            }
            if (aIndex >= 0)
            {
                p.filesModel->setA(aIndex);
            }
            p.filesModel->setCompareTime(review.compare.time);

            // Color and image display.
            p.colorModel->setOCIOOptions(review.color.ocio);
            p.colorModel->setLUTOptions(review.color.lut);
            p.viewportModel->setDisplayOptions(review.color.display);
            p.viewportModel->setBackgroundOptions(review.color.background);
            p.viewportModel->setForegroundOptions(review.color.foreground);
            p.viewportModel->setAspectRatioOptions(review.color.aspectRatio);
            p.viewportModel->setHUDOptions(review.color.hud);

            // Interface.
            p.toolsModel->closeTools();
            for (const auto& tool : review.ui.openTools)
            {
                p.toolsModel->setToolOpen(tool, true);
            }

            p.markersModel->setMarkers(review.markers);
            p.annotationsModel->setAnnotations(review.annotations);

            // View state is applied once the viewport exists and the new player's
            // initial auto-frame has settled.
            p.pendingReviewView = review.view;
            if (p.mainWindow)
            {
                _applyReviewView();
            }

            p.reviewPath = reviewPath;
            p.reviewRaw = review.raw;
            p.reviewUnreadSections = review.unreadSections;
            p.reviewUnreadItems = review.unreadItems;
            p.recentReviewsModel->addRecent(ftk::Path(reviewPath.u8string()));
            p.reviewModified = false;
            _updateWindowTitle();
            // A freshly loaded review supersedes any earlier autosave.
            _deleteAutosave();

            if (!missing.empty())
            {
                _context->log(
                    "djv::app::App",
                    ftk::Format("Review \"{0}\": {1} file(s) not found: {2}").
                        arg(reviewPath.u8string()).
                        arg(missing.size()).
                        arg(ftk::join(missing, ", ")),
                    ftk::LogType::Warning);

                // Offer to relocate on the first pass only (a substitute root
                // already tried means we shouldn't loop).
                if (substituteRoot.empty() && p.mainWindow)
                {
                    auto dialogSystem = _context->getSystem<ftk::DialogSystem>();
                    // Pre-wrap with newlines: ftk::Label renders "\n" but cannot
                    // auto-wrap, so a long single line would scroll horizontally.
                    dialogSystem->confirm(
                        "Relocate Files",
                        ftk::Format("{0} file(s) from this review\n"
                            "could not be found.\n"
                            "\n"
                            "Locate the folder that contains them?").
                            arg(missing.size()),
                        p.mainWindow,
                        [this, review, base, reviewPath](bool value)
                        {
                            if (value)
                            {
                                auto fileBrowserSystem = _context->getSystem<ftk::FileBrowserSystem>();
                                ftk::FileBrowserOpenOptions options;
                                options.title = "Locate Files";
                                options.path = base;
                                options.mode = ftk::FileBrowserMode::Dir;
                                fileBrowserSystem->open(
                                    _p->mainWindow,
                                    [this, review, base, reviewPath](const ftk::Path& folder)
                                    {
                                        _applyReview(
                                            review,
                                            base,
                                            reviewPath,
                                            std::filesystem::u8path(folder.get()));
                                    },
                                    options);
                            }
                        },
                        "Locate...",
                        "Ignore");
                }
            }
        }

        void App::_reviewFileDialog(
            ftk::FileBrowserMode mode,
            const std::string& title,
            const std::function<void(const std::filesystem::path&)>& callback)
        {
            FTK_P();
            // Use the shared file browser system so reviews get the same (native
            // by default) dialog as media, now filtered to ".djvr" with a real
            // save dialog.
            auto fileBrowserSystem = _context->getSystem<ftk::FileBrowserSystem>();
            const std::filesystem::path startPath = p.reviewPath.empty() ?
                std::filesystem::path() : p.reviewPath.parent_path();
            ftk::FileBrowserOpenOptions options;
            options.title = title;
            options.path = startPath;
            options.mode = mode;
            if (ftk::FileBrowserMode::Save == mode)
            {
                // The review's own name where there is one, the way the
                // playlists suggest "playlist.otio".
                options.fileName = p.reviewPath.empty() ?
                    std::string("review") + models::reviewExtension() :
                    p.reviewPath.filename().u8string();
            }
            options.extensions = { models::reviewExtension() };
            options.extensionsLabel = "Review Session";
            fileBrowserSystem->open(
                p.mainWindow,
                [callback](const ftk::Path& value)
                {
                    callback(std::filesystem::u8path(value.get()));
                },
                options);
        }

        void App::openReviewDialog()
        {
            _reviewFileDialog(
                ftk::FileBrowserMode::Open,
                "Open Review",
                [this](const std::filesystem::path& path)
                {
                    openReview(path);
                });
        }

        void App::saveReview()
        {
            FTK_P();
            if (p.reviewPath.empty())
            {
                saveReviewAs();
            }
            else
            {
                saveReview(p.reviewPath);
            }
        }

        models::Review App::_buildReview(const std::filesystem::path& base)
        {
            FTK_P();

            models::Review review;
            review.version = models::reviewVersion;
            review.app = ftk::Format("{0} {1}").
                arg(p.appInfoModel->getFullName()).
                arg(p.appInfoModel->getVersion());
            review.created = models::timestamp();
            // Carry what the review we last loaded held but we could not use:
            // the sections we do not know, and the ones we failed to read. The
            // document is rebuilt from the models, so without this the save
            // would replace them with whatever we fell back to.
            review.raw = p.reviewRaw;
            review.unreadSections = p.reviewUnreadSections;
            review.unreadItems = p.reviewUnreadItems;

            for (const auto& file : p.filesModel->getFiles())
            {
                models::ReviewFile rf;
                rf.id = file->id;
                rf.pathAbsolute = models::reviewGenericPath(file->path.get());
                rf.path = models::reviewRelativePath(file->path.get(), base);
                // The separate audio travels with the review like the file does:
                // stored absolute only, it would not survive the move.
                rf.audioPath = models::reviewRelativePath(file->audioPath.get(), base);
                rf.audioPathAbsolute = models::reviewGenericPath(file->audioPath.get());
                rf.videoLayer = static_cast<int>(file->videoLayer);
                rf.speed = file->speed;
                rf.currentTime = file->currentTime;
                rf.inOutRange = file->inOutRange;
                review.files.push_back(rf);
            }

            // Persist the live playback state of the active file, which the model
            // item only receives when the file is switched away from.
            if (auto player = p.player->get())
            {
                const int aIndex = p.filesModel->getAIndex();
                if (aIndex >= 0 && aIndex < static_cast<int>(review.files.size()))
                {
                    review.files[aIndex].speed = player->getSpeed();
                    review.files[aIndex].currentTime = player->getCurrentTime();
                    review.files[aIndex].inOutRange = player->getInOutRange();
                }
            }

            const auto& files = p.filesModel->getFiles();
            const int aIndex = p.filesModel->getAIndex();
            if (aIndex >= 0 && aIndex < static_cast<int>(files.size()))
            {
                review.compare.aId = files[aIndex]->id;
            }
            for (const int bIndex : p.filesModel->getBIndexes())
            {
                if (bIndex >= 0 && bIndex < static_cast<int>(files.size()))
                {
                    review.compare.bIds.push_back(files[bIndex]->id);
                }
            }
            review.compare.options = p.filesModel->getCompareOptions();
            review.compare.time = p.filesModel->getCompareTime();

            if (p.mainWindow)
            {
                auto viewport = p.mainWindow->getViewport();
                review.view.frameView = viewport->hasFrameView();
                review.view.pos = viewport->getViewPos();
                review.view.zoom = viewport->getZoom();
            }

            review.color.ocio = p.colorModel->getOCIOOptions();
            review.color.lut = p.colorModel->getLUTOptions();
            review.color.display = p.viewportModel->getDisplayOptions();
            review.color.background = p.viewportModel->getBackgroundOptions();
            review.color.foreground = p.viewportModel->getForegroundOptions();
            review.color.aspectRatio = p.viewportModel->getAspectRatioOptions();
            review.color.hud = p.viewportModel->getHUDOptions();

            review.ui.openTools = p.toolsModel->getOpenTools();

            review.markers = p.markersModel->getMarkers();
            review.annotations = p.annotationsModel->getAnnotations();

            return review;
        }

        void App::saveReview(const std::filesystem::path& path)
        {
            FTK_P();

            models::Review review = _buildReview(path.parent_path());

            try
            {
                models::reviewSave(path.u8string(), review);
            }
            catch (const std::exception& e)
            {
                _context->log("djv::app::App", e.what(), ftk::LogType::Error);
                return;
            }

            p.reviewPath = path;
            p.reviewRaw = review.raw;
            p.recentReviewsModel->addRecent(ftk::Path(path.u8string()));
            p.reviewModified = false;
            _updateWindowTitle();
            // The work is safely on disk; drop any crash-recovery backup.
            _deleteAutosave();
        }

        void App::saveReviewAs()
        {
            _saveReviewAs(nullptr);
        }

        void App::_saveReviewAs(const std::function<void()>& onSaved)
        {
            _reviewFileDialog(
                ftk::FileBrowserMode::Save,
                "Save Review",
                [this, onSaved](const std::filesystem::path& value)
                {
                    std::filesystem::path path = value;
                    if (path.extension() != models::reviewExtension())
                    {
                        // Auto-complete the extension when the user types a bare
                        // name.
                        path.replace_extension(models::reviewExtension());
                    }
                    saveReview(path);
                    if (onSaved)
                    {
                        onSaved();
                    }
                });
        }

        void App::_importReviewTimeline(const std::filesystem::path& path)
        {
            FTK_P();
            // The file is never modified, the playlist rule carried
            // forward, and the review path stays unset: saving asks where
            // to write DJV's own document. A review that includes a
            // timeline references it, the same way a timeline references
            // its media.
            _closeReview();
            open(ftk::Path(path.u8string()));
            if (!p.timelines.empty() && p.timelines.front())
            {
                p.markersModel->setMarkers(
                    models::reviewMarkersFromTimeline(
                        p.timelines.front()->getOTIOTimeline()));
            }
            // The feedback still lives in the source file, so quitting
            // straight away has nothing to lose; the first change made
            // here marks the session the usual way.
            p.reviewModified = false;
            _updateWindowTitle();
        }

        void App::importReviewDialog()
        {
            FTK_P();
            auto fileBrowserSystem = _context->getSystem<ftk::FileBrowserSystem>();
            ftk::FileBrowserOpenOptions options;
            options.title = "Import";
            options.mode = ftk::FileBrowserMode::Open;
            options.extensions = { ".otio", ".otioz" };
            options.extensionsLabel = "Timeline";
            fileBrowserSystem->open(
                p.mainWindow,
                [this](const ftk::Path& value)
                {
                    _importReviewTimeline(
                        std::filesystem::u8path(value.get()));
                },
                options);
        }

        void App::exportReviewMarkers()
        {
            FTK_P();
            auto fileBrowserSystem = _context->getSystem<ftk::FileBrowserSystem>();
            ftk::FileBrowserOpenOptions options;
            options.title = "Export";
            options.mode = ftk::FileBrowserMode::Save;
            options.path = p.reviewPath.empty() ?
                std::filesystem::path() : p.reviewPath.parent_path();
            options.fileName = p.reviewPath.empty() ?
                std::string("markers.otio") :
                p.reviewPath.stem().u8string() + ".otio";
            options.extensions = { ".otio" };
            options.extensionsLabel = "Timeline";
            fileBrowserSystem->open(
                p.mainWindow,
                [this](const ftk::Path& value)
                {
                    std::filesystem::path path =
                        std::filesystem::u8path(value.get());
                    if (path.extension() != ".otio")
                    {
                        path.replace_extension(".otio");
                    }
                    _exportReviewMarkers(path);
                },
                options);
        }

        void App::_exportReviewMarkers(const std::filesystem::path& path)
        {
            FTK_P();
            auto player = p.player->get();
            if (!player)
            {
                return;
            }
            // The shape follows what "A" is. A timeline exports as a copy
            // of itself with the markers written in, the editorial round
            // trip; plain media exports as a minimal timeline with one clip
            // referencing it, so the document always says what the feedback
            // is about. The live timeline is never touched: the export
            // builds its own document and discards it.
            OTIO_NS::ErrorStatus errorStatus;
            OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline> timeline;
            const ftk::Path& aPath = player->getPath();
            const std::string ext = ftk::toLower(aPath.getExt());
            if (".otio" == ext || ".otioz" == ext)
            {
                timeline = dynamic_cast<OTIO_NS::Timeline*>(
                    player->getTimeline()->getOTIOTimeline()->clone(&errorStatus));
            }
            else
            {
                const OTIO_NS::TimeRange timeRange = player->getTimeRange();
                auto clip = new OTIO_NS::Clip(
                    aPath.getFileName(),
                    new OTIO_NS::ExternalReference(aPath.getFileName(true)),
                    timeRange);
                auto track = new OTIO_NS::Track(
                    "Video",
                    std::nullopt,
                    OTIO_NS::Track::Kind::video);
                track->append_child(clip, &errorStatus);
                timeline = new OTIO_NS::Timeline;
                timeline->tracks()->append_child(track, &errorStatus);
            }
            if (!timeline || OTIO_NS::is_error(errorStatus))
            {
                _context->log(
                    "djv::app::App",
                    ftk::Format("Cannot export markers: {0}").
                        arg(errorStatus.details),
                    ftk::LogType::Error);
                return;
            }
            models::reviewMarkersToTimeline(
                p.markersModel->getMarkers(), timeline);
            if (!timeline->to_json_file(path.u8string(), &errorStatus))
            {
                _context->log(
                    "djv::app::App",
                    ftk::Format("Cannot export markers \"{0}\": {1}").
                        arg(path.u8string()).
                        arg(errorStatus.details),
                    ftk::LogType::Error);
            }
        }

        void App::closeReview()
        {
            confirmClose(
                [this]
                {
                    _closeReview();
                });
        }

        void App::_closeReview()
        {
            FTK_P();
            // Reset to the empty startup state.
            p.filesModel->closeAll();
            tl::CompareOptions compareOptions;
            compareOptions.compare = tl::Compare::None;
            p.filesModel->setCompareOptions(compareOptions);
            p.markersModel->clear();
            p.annotationsModel->clear();
            p.reviewPath.clear();
            p.reviewRaw = nlohmann::json();
            p.reviewUnreadSections.clear();
            p.reviewUnreadItems = nlohmann::json();
            // closeAll / setCompareOptions marked the session modified; clear it
            // last so the empty state is clean.
            p.reviewModified = false;
            _updateWindowTitle();
            _deleteAutosave();
        }

        const std::filesystem::path& App::getReviewPath() const
        {
            return _p->reviewPath;
        }

        const std::shared_ptr<models::RecentFilesModel>& App::getRecentReviewsModel() const
        {
            return _p->recentReviewsModel;
        }

        const std::shared_ptr<models::RecentFilesModel>& App::getRecentPlaylistsModel() const
        {
            return _p->recentPlaylistsModel;
        }

        void App::exit()
        {
            // A headless run has nobody to answer the question, and the
            // capture harness exits the application itself when it is done.
            if (getHideSetup())
            {
                ftk::App::exit();
                return;
            }
            // The base class exit is what actually stops the event loop, so
            // nothing may call it while the question is still open.
            confirmClose(
                [this]
                {
                    ftk::App::exit();
                });
        }

        void App::confirmClose(const std::function<void()>& onProceed)
        {
            FTK_P();
            // Prompt for a review that is already saved (has a path) and has
            // unsaved changes, and for a session that authored feedback --
            // markers or drawings, which exist nowhere but here -- without
            // ever saving. Opening loose media and only looking never asks.
            const bool hasFeedback =
                !p.markersModel->getMarkers().empty() ||
                !p.annotationsModel->getAnnotations().empty();
            if (!p.reviewModified ||
                (p.reviewPath.empty() && !hasFeedback) ||
                p.filesModel->getFiles().empty() ||
                !p.mainWindow)
            {
                onProceed();
                return;
            }
            _context->getSystem<ftk::DialogSystem>()->choice(
                "Save Review",
                p.reviewPath.empty() ?
                    "Save the session as a review before closing?" :
                    "Save changes to the review before closing?",
                { "Save", "Don't Save", "Cancel" },
                p.mainWindow,
                [this, onProceed](int value)
                {
                    switch (value)
                    {
                    case 0:
                        if (_p->reviewPath.empty())
                        {
                            // Never saved: ask where. Cancelling the file
                            // dialog cancels the close, the same as Cancel
                            // here.
                            _saveReviewAs(onProceed);
                        }
                        else
                        {
                            saveReview(_p->reviewPath);
                            onProceed();
                        }
                        break;
                    case 1:
                        // A deliberate discard is not a crash: drop the backup.
                        _deleteAutosave();
                        onProceed();
                        break;
                    default:
                        // Cancel, and dismissing the dialog means the same.
                        break;
                    }
                });
        }

        void App::_markModified()
        {
            FTK_P();
            if (!p.reviewModified)
            {
                p.reviewModified = true;
                // Reflect the change with a "*" in the title.
                _updateWindowTitle();
            }
        }

        void App::_markersUpdate()
        {
            FTK_P();
            // The frames worth jumping to: a marker's start, a drawing's
            // frame. This is what the previous/next marker actions walk, and
            // it follows the timeline, which shows "A".
            std::vector<int> jumps;
            // The undifferentiated ticks -- "there is something here" --
            // now carry only the drawings; the markers draw as themselves.
            std::vector<int> ticks;
            for (const auto& marker : p.markersModel->getMarkers())
            {
                if (marker.range.has_value())
                {
                    jumps.push_back(
                        static_cast<int>(marker.range->start_time().value()));
                }
            }
            // Every annotation is stamped with the player's time, which is the
            // timeline's own clock, so a drawing made on a "B" source still
            // marks the right place. Filtering on the source would drop those.
            for (const auto& annotation : p.annotationsModel->getAnnotations())
            {
                if (annotation.time.has_value())
                {
                    const int frame =
                        static_cast<int>(annotation.time->value());
                    jumps.push_back(frame);
                    ticks.push_back(frame);
                }
            }
            std::sort(jumps.begin(), jumps.end());
            jumps.erase(std::unique(jumps.begin(), jumps.end()), jumps.end());
            p.reviewMarkers->setIfChanged(jumps);
            std::sort(ticks.begin(), ticks.end());
            ticks.erase(std::unique(ticks.begin(), ticks.end()), ticks.end());
            // The window can still be missing here: the observers fire while a
            // review passed on the command line is applied. The lists are kept
            // above regardless, and pushed again once the window exists.
            if (p.mainWindow)
            {
                p.mainWindow->getTimelineWidget()->setFrameMarkers(ticks);
                _timelineMarkersUpdate();
            }
        }

        void App::_timelineMarkersUpdate()
        {
            FTK_P();
            // The review markers draw on the timeline as ranged, colored
            // markers; the undifferentiated ticks above stay for the
            // drawings. Markers about no frame in particular have nowhere
            // to draw.
            std::vector<tl::ui::Marker> markers;
            for (const auto& marker : p.markersModel->getMarkers())
            {
                if (marker.range.has_value())
                {
                    tl::ui::Marker m;
                    m.name = marker.name;
                    m.color = marker.color;
                    m.range = *marker.range;
                    markers.push_back(m);
                }
            }
            p.mainWindow->getTimelineWidget()->setMarkers(markers);
        }

        void App::_updateWindowTitle()
        {
            FTK_P();
            if (!p.mainWindow)
            {
                return;
            }
            std::string title = p.appInfoModel->getTitle();
            if (!p.reviewPath.empty())
            {
                // Show the active review so the user can tell which one is
                // open, with a trailing "*" while it has unsaved changes. The
                // name rather than the path, the way document titles usually
                // read; the Recent Reviews menu is where the whole paths are.
                title += " - " + p.reviewPath.filename().u8string();
                if (p.reviewModified)
                {
                    title += " *";
                }
            }
            p.mainWindow->setTitle(title);
        }

        void App::_applyReviewView()
        {
            FTK_P();
            if (!p.pendingReviewView.has_value() || !p.mainWindow)
            {
                return;
            }
            if (p.pendingReviewView->frameView)
            {
                p.mainWindow->getViewport()->setFrameView(true);
                p.pendingReviewView.reset();
            }
            else
            {
                // Defer past the initial auto-frame that the new player triggers
                // on the next layout pass. setViewPosAndZoom disables frame view,
                // so no later re-frame overrides it.
                if (!p.reviewViewTimer)
                {
                    p.reviewViewTimer = ftk::Timer::create(_context);
                }
                p.reviewViewTimer->start(
                    std::chrono::milliseconds(200),
                    [this]
                    {
                        FTK_P();
                        if (p.pendingReviewView.has_value() && p.mainWindow)
                        {
                            p.mainWindow->getViewport()->setViewPosAndZoom(
                                p.pendingReviewView->pos,
                                p.pendingReviewView->zoom);
                            p.pendingReviewView.reset();
                        }
                    });
            }
        }

        std::filesystem::path App::_autosavePath()
        {
            FTK_P();
            // The same directory the settings and log live in.
            return ftk::getUserPath(ftk::UserPath::Documents) /
                p.appInfoModel->getShortName() /
                ftk::Format("{0}.{1}.autosave.djvr").
                arg(p.appInfoModel->getShortName()).
                arg(p.appInfoModel->getVersionMajor()).
                str();
        }

        void App::_writeAutosave()
        {
            FTK_P();
            // Only a saved review with unsaved changes is worth backing up --
            // and never from a headless run, which would plant its scratch
            // state in the user's one recovery slot.
            if (!p.reviewModified || p.reviewPath.empty() || getHideSetup())
            {
                return;
            }
            try
            {
                nlohmann::json json = _buildReview(p.reviewPath.parent_path());
                // Remember which review this backs up, for recovery.
                json["_autosaveReviewPath"] = p.reviewPath.u8string();
                std::ofstream f(_autosavePath());
                if (f.is_open())
                {
                    f << std::setw(4) << json << std::endl;
                }
            }
            catch (const std::exception& e)
            {
                _context->log(
                    "djv::app::App",
                    ftk::Format("Cannot write autosave: {0}").arg(e.what()),
                    ftk::LogType::Warning);
            }
        }

        void App::_deleteAutosave()
        {
            std::error_code ec;
            std::filesystem::remove(_autosavePath(), ec);
        }

        void App::_recoverAutosave()
        {
            FTK_P();
            if (!p.recoveredAutosave.has_value())
            {
                return;
            }
            const nlohmann::json json = *p.recoveredAutosave;
            p.recoveredAutosave.reset();
            try
            {
                models::Review review = json.get<models::Review>();
                if (!models::reviewVersionSupported(review.version))
                {
                    _context->log(
                        "djv::app::App",
                        ftk::Format(
                            "Cannot recover autosave: it is format version {0}, "
                            "and this build of DJV reads up to version {1}.").
                            arg(review.version).
                            arg(models::reviewVersion),
                        ftk::LogType::Error);
                    return;
                }
                std::filesystem::path reviewPath;
                if (json.contains("_autosaveReviewPath"))
                {
                    reviewPath = std::filesystem::u8path(
                        json.at("_autosaveReviewPath").get<std::string>());
                }
                // Keep the internal marker out of any later saved ".djvr".
                review.raw.erase("_autosaveReviewPath");
                _logUnreadSections(review, reviewPath);
                _applyReview(review, reviewPath.parent_path(), reviewPath, std::filesystem::path());
                // The recovered state is, by definition, unsaved.
                p.reviewModified = true;
                _updateWindowTitle();
            }
            catch (const std::exception& e)
            {
                _context->log(
                    "djv::app::App",
                    ftk::Format("Cannot recover autosave: {0}").arg(e.what()),
                    ftk::LogType::Error);
            }
        }

        void App::reload()
        {
            _reload(false);
        }

        void App::_reload(bool restructured)
        {
            FTK_P();
            const auto activeFiles = p.activeFiles;
            const auto files = p.files;
            for (const auto& i : activeFiles)
            {
                const auto j = std::find(p.files.begin(), p.files.end(), i);
                if (j != p.files.end())
                {
                    const size_t index = j - p.files.begin();
                    p.files.erase(j);
                    p.timelines.erase(p.timelines.begin() + index);
                }
            }
            p.activeFiles.clear();
            std::optional<int64_t> frame;
            if (!activeFiles.empty())
            {
                if (auto player = p.player->get())
                {
                    activeFiles.front()->speed = player->getSpeed();
                    if (restructured)
                    {
                        // The position and the in/out range are both in
                        // timeline time, and the timeline is about to be a
                        // different length, so neither means the same thing
                        // afterwards. What does carry over is the frame being
                        // looked at, which the media names in its own time.
                        frame = player->getTimeline()->getMediaFrame(
                            player->getCurrentTime());
                        if (!frame.has_value())
                        {
                            // Sitting in a hole, where there is no clip to name
                            // the frame. The time itself is the best guess, and
                            // under Gaps it is exactly right.
                            frame = static_cast<int64_t>(
                                player->getCurrentTime().value());
                        }
                        activeFiles.front()->currentTime.reset();
                        activeFiles.front()->inOutRange.reset();
                    }
                    else
                    {
                        activeFiles.front()->currentTime = player->getCurrentTime();
                        activeFiles.front()->inOutRange = player->getInOutRange();
                    }
                }
            }

            auto thumbnailSytem = _context->getSystem<tl::ui::ThumbnailSystem>();
            thumbnailSytem->clearCache();

            _filesUpdate(files);
            _activeUpdate(activeFiles);

            // The items are the same objects holding different things now --
            // a reload finds the frames again, so the range can have changed
            // -- and the list of them did not change, so say so.
            p.filesModel->refresh();

            if (frame.has_value())
            {
                if (auto player = p.player->get())
                {
                    // Asked against the start rather than where playback was
                    // left, which may be past the end of a timeline that has
                    // just become shorter. A frame the new timeline does not
                    // hold snaps to one it does.
                    const auto& timeRange = player->getTimeRange();
                    if (const auto time =
                        player->getTimeline()->getTimelineTime(
                            timeRange.start_time(),
                            OTIO_NS::RationalTime(
                                static_cast<double>(frame.value()),
                                timeRange.duration().rate())))
                    {
                        player->seek(time.value());
                    }
                }
            }
        }

        std::shared_ptr<ftk::IObservable<std::shared_ptr<tl::Player> > > App::observePlayer() const
        {
            return _p->player;
        }

        const std::shared_ptr<ToolWidgetFactory>& App::getToolWidgetFactory() const
        {
            return _p->toolWidgetFactory;
        }

        std::shared_ptr<ui::StatusIndicator> App::createIndicator()
        {
            FTK_P();
            return ui::StatusIndicator::create(
                _context,
                p.viewportModel,
                p.colorModel,
                p.audioModel);
        }

        const std::shared_ptr<MainWindow>& App::getMainWindow() const
        {
            return _p->mainWindow;
        }

        std::shared_ptr<ftk::IObservable<bool> > App::observeSecondaryWindow() const
        {
            return _p->secondaryWindowActive;
        }

        void App::setSecondaryWindow(bool value)
        {
            FTK_P();
            if (p.secondaryWindowActive->setIfChanged(value))
            {
                if (value)
                {
                    p.secondaryWindow = SecondaryWindow::create(
                        _context,
                        std::dynamic_pointer_cast<App>(shared_from_this()));
                    p.secondaryWindow->setCloseCallback(
                        [this]
                        {
                            FTK_P();
                            p.secondaryWindowActive->setIfChanged(false);
                            p.secondaryWindow.reset();
                        });
                    p.secondaryWindow->show();
                }
                else if (p.secondaryWindow)
                {
                    p.secondaryWindow->close();
                    p.secondaryWindow.reset();
                }
            }
        }

        std::vector<std::string> App::getSysInfo() const
        {
            FTK_P();
            return ui::getSysInfo(
                _context,
                p.appInfoModel,
                p.settingsModel,
                p.mainWindow ?
                    p.mainWindow->getWindowInfo() :
                    std::vector<std::pair<std::string, std::string> >());
        }

        void App::run()
        {
            FTK_P();

            // Capture any autosave left by a crashed session before anything can
            // overwrite or delete it; the recovery prompt is offered once the
            // main window exists.
            {
                std::ifstream f(_autosavePath());
                if (f.is_open())
                {
                    try
                    {
                        nlohmann::json json;
                        f >> json;
                        p.recoveredAutosave = json;
                    }
                    catch (const std::exception&)
                    {}
                }
            }

            _modelsInit();
            _observersInit();
            _inputFilesInit();
            
            _uiInit();

            if (p.cmdLine.version->found())
            {
                std::cout << DJV_VERSION_FULL << std::endl;
                return;
            }
            else if (p.cmdLine.sysInfo->found())
            {
                std::cout << ftk::join(getSysInfo(), '\n') << std::endl;
                return;
            }
            
            _mainWindowInit();

            if (p.cmdLine.listCommands->found())
            {
                for (const auto& command : p.commandsModel->getCommands())
                {
                    std::cout << command.name << " - " << command.doc << std::endl;
                }
                return;
            }

            if (p.cmdLine.command->found())
            {
                // Wait for the command line inputs to be opened before
                // executing the command.
                p.commandTimer = ftk::Timer::create(_context);
                p.commandTimer->setRepeating(true);
                p.commandTimer->start(
                    std::chrono::milliseconds(100),
                    [this]
                    {
                        FTK_P();
                        ++p.commandTicks;
                        const bool timeout = p.commandTicks > 100;
                        if (p.cmdLine.inputs->getList().empty() ||
                            p.player->get() ||
                            timeout)
                        {
                            p.commandTimer->stop();
                            for (const std::string& value : p.cmdLine.command->getList())
                            {
                                // Split the command name from the optional JSON
                                // arguments at the first '{', so that command
                                // names may contain spaces (e.g.,
                                // "Tools/Color Picker").
                                const size_t i = value.find_first_of('{');
                                std::string name = value.substr(0, i);
                                const size_t end = name.find_last_not_of(" \t");
                                name = name.substr(
                                    0,
                                    end != std::string::npos ? (end + 1) : 0);
                                nlohmann::json args;
                                bool argsOK = true;
                                if (i != std::string::npos)
                                {
                                    try
                                    {
                                        args = nlohmann::json::parse(value.substr(i));
                                    }
                                    catch (const std::exception& e)
                                    {
                                        argsOK = false;
                                        _context->getLogSystem()->print(
                                            "djv::app::App",
                                            ftk::Format("Cannot parse command arguments: {0}").
                                            arg(e.what()),
                                            ftk::LogType::Error);
                                    }
                                }
                                if (argsOK)
                                {
                                    p.commandsModel->exec(name, args);
                                }
                            }
                        }
                    });
            }

            if (p.cmdLine.debugLoop->found() &&
                !p.cmdLine.inputs->getList().empty())
            {
                p.debugTimer = ftk::Timer::create(_context);
                p.debugTimer->setRepeating(true);
                p.debugTimer->start(
                    std::chrono::seconds(p.cmdLine.debugLoop->getValue()),
                    [this]
                    {
                        FTK_P();
                        if (!p.filesModel->getFiles().empty())
                        {
                            p.filesModel->closeAll();
                        }
                        else
                        {
                            ftk::Path path(p.cmdLine.inputs->getList()[p.debugInput]);
                            if (path.hasSeqWildcard())
                            {
                                path = ftk::expandSeq(path);
                            }
                            open(path);
                            if (auto player = p.player->get())
                            {
                                player->forward();
                            }
                            ++p.debugInput;
                            if (p.debugInput >= static_cast<int>(p.cmdLine.inputs->getList().size()))
                            {
                                p.debugInput = 0;
                            }
                        }
                    });
            }

            if (p.cmdLine.benchmark->found())
            {
                auto benchmark = Benchmark::create(
                    _context, std::dynamic_pointer_cast<App>(shared_from_this()),
                    p.cmdLine.benchmark->getValue());
                if (!benchmark->begin())
                {
                    throw std::runtime_error("Cannot set up the benchmark");
                }
                ftk::App::run();
            _saveSettings();
                if (!benchmark->succeeded())
                {
                    throw std::runtime_error("The benchmark produced no measurement");
                }
                return;
            }

            if (p.cmdLine.captureShot->found())
            {
                auto capture = Capture::create(
                    _context, std::dynamic_pointer_cast<App>(shared_from_this()),
                    p.cmdLine.captureManifest->getValue(),
                    p.cmdLine.captureShot->getValue(),
                    p.cmdLine.captureOutput->getValue());
                if (!capture->begin())
                {
                    throw std::runtime_error(ftk::Format(
                        "Cannot set up capture: {0}").arg(p.cmdLine.captureShot->getValue()));
                }
                ftk::App::run();
            _saveSettings();
                if (!capture->succeeded())
                {
                    throw std::runtime_error(ftk::Format(
                        "Cannot capture shot: {0}").arg(p.cmdLine.captureShot->getValue()));
                }
                return;
            }

            ftk::App::run();
            _saveSettings();
        }

        void App::_debugState(nlohmann::json& out)
        {
            FTK_P();

            nlohmann::json viewport;
            to_json(viewport["displayOptions"], p.viewportModel->getDisplayOptions());
            to_json(viewport["backgroundOptions"], p.viewportModel->getBackgroundOptions());
            to_json(viewport["foregroundOptions"], p.viewportModel->getForegroundOptions());
            out["viewport"] = viewport;

            nlohmann::json color;
            to_json(color["ocioOptions"], p.colorModel->getOCIOOptions());
            to_json(color["lutOptions"], p.colorModel->getLUTOptions());
            out["color"] = color;

            nlohmann::json audio;
            audio["volume"] = p.audioModel->getVolume();
            audio["mute"] = p.audioModel->isMuted();
            audio["syncOffset"] = p.audioModel->getSyncOffset();
            out["audio"] = audio;

            nlohmann::json files = nlohmann::json::array();
            for (const auto& item : p.filesModel->getFiles())
            {
                files.push_back(item->path.get());
            }
            out["files"] = files;
            out["aIndex"] = p.filesModel->getAIndex();

            if (auto player = p.player->get())
            {
                nlohmann::json j;
                j["path"] = player->getPath().get();
                j["speed"] = player->getSpeed();
                j["defaultSpeed"] = player->getDefaultSpeed();
                j["playback"] = tl::to_string(player->getPlayback());
                j["loop"] = tl::to_string(player->getLoop());
                const OTIO_NS::RationalTime& time = player->getCurrentTime();
                j["currentTime"] = { time.value(), time.rate() };
                const OTIO_NS::TimeRange& range = player->getInOutRange();
                j["inOutRange"] = {
                    range.start_time().value(),
                    range.duration().value(),
                    range.duration().rate() };
                j["videoLayer"] = player->getVideoLayer();
                j["audioOffset"] = player->getAudioOffset();
                out["player"] = j;
            }
        }

        void App::_debugStateCommand(const nlohmann::json& args)
        {
            nlohmann::json out;
            _debugState(out);

            const std::string file =
                args.is_object() && args.contains("file") ?
                args.at("file").get<std::string>() :
                std::string();
            if (!file.empty())
            {
                std::ofstream f(std::filesystem::u8path(file));
                f << out.dump(2) << std::endl;
            }
            else
            {
                std::cout << out.dump(2) << std::endl;
            }
        }

        void App::_saveSettings()
        {
            FTK_P();
            // Everything is written at the clean quit; the writes in the
            // destructors are a backstop. A leak that keeps a model alive
            // no longer loses its settings.
            if (p.mainWindow)
            {
                p.mainWindow->saveSettings();
            }
            p.timeUnitsModel->save();
            p.filesModel->save();
            p.recentFilesModel->save();
            p.viewportModel->save();
            p.colorModel->save();
            p.audioModel->save();
            p.toolsModel->save();
            p.settingsModel->save();
            getSettings()->save();
        }

        void App::_modelsInit()
        {
            FTK_P();

            p.settingsModel = models::SettingsModel::create(
                _context,
                getSettings(),
                getDefaultDisplayScale());
            if (getColorStyleCmdLineOption()->found() ||
                getDisplayScaleCmdLineOption()->found())
            {
                // Override settings with the command line.
                auto style = p.settingsModel->getStyle();
                if (getColorStyleCmdLineOption()->found())
                {
                    style.colorStyle = getColorStyleCmdLineOption()->getValue();
                }
                if (getDisplayScaleCmdLineOption()->found())
                {
                    style.displayScale = getDisplayScaleCmdLineOption()->getValue();
                }
                p.settingsModel->setStyle(style);
            }

            p.sysLogModel = ftk::SysLogModel::create(_context);

            p.timeUnitsModel = models::TimeUnitsModel::create(_context, getSettings());
            
            p.filesModel = models::FilesModel::create(getSettings());

            p.recentFilesModel = models::RecentFilesModel::create(_context, getSettings());
            // Reviews and playlists get their own recent lists, so opening
            // one does not push its media into the recent files.
            p.recentReviewsModel = models::RecentFilesModel::create(_context, getSettings(), "Review");
            p.recentPlaylistsModel = models::RecentFilesModel::create(_context, getSettings(), "Playlist");
            auto fileBrowserSystem = _context->getSystem<ftk::FileBrowserSystem>();
            fileBrowserSystem->getModel()->setExts(tl::getExts(_context));
            // From what the settings restored rather than from a fresh set:
            // the sequence extensions are the build's to say and cannot come
            // from a settings file, but everything else in there is the
            // user's and was just loaded.
            ftk::FileBrowserOptions fileBrowserOptions =
                fileBrowserSystem->getModel()->getOptions();
            fileBrowserOptions.dirList.seqExts = tl::getExts(_context, static_cast<int>(tl::FileType::Seq));
            fileBrowserSystem->getModel()->setOptions(fileBrowserOptions);
            fileBrowserSystem->setRecentFilesModel(p.recentFilesModel);

            p.colorModel = models::ColorModel::create(_context, getSettings());
#if defined(TLRENDER_OCIO)
            if (p.cmdLine.ocioFileName->found() ||
                p.cmdLine.ocioInput->found() ||
                p.cmdLine.ocioDisplay->found() ||
                p.cmdLine.ocioView->found() ||
                p.cmdLine.ocioLook->found())
            {
                tl::OCIOOptions options = p.colorModel->getOCIOOptions();
                options.enabled = true;
                if (p.cmdLine.ocioFileName->found())
                {
                    options.fileName = p.cmdLine.ocioFileName->getValue();
                }
                if (p.cmdLine.ocioInput->found())
                {
                    options.input = p.cmdLine.ocioInput->getValue();
                }
                if (p.cmdLine.ocioDisplay->found())
                {
                    options.display = p.cmdLine.ocioDisplay->getValue();
                }
                if (p.cmdLine.ocioView->found())
                {
                    options.view = p.cmdLine.ocioView->getValue();
                }
                if (p.cmdLine.ocioLook->found())
                {
                    options.look = p.cmdLine.ocioLook->getValue();
                }
                p.colorModel->setOCIOOptions(options);
            }
            if (p.cmdLine.lutFileName->found() ||
                p.cmdLine.lutOrder->found())
            {
                tl::LUTOptions options = p.colorModel->getLUTOptions();
                options.enabled = true;
                if (p.cmdLine.lutFileName->found())
                {
                    options.fileName = p.cmdLine.lutFileName->getValue();
                }
                if (p.cmdLine.lutOrder->found())
                {
                    options.order = p.cmdLine.lutOrder->getValue();
                }
                p.colorModel->setLUTOptions(options);
            }
#endif // TLRENDER_OCIO

            p.viewportModel = models::ViewportModel::create(_context, getSettings());

            p.audioModel = models::AudioModel::create(_context, getSettings());

            p.toolsModel = models::ToolsModel::create(getSettings());

            p.commandsModel = models::CommandsModel::create(_context);

            p.markersModel = models::MarkersModel::create();
            p.annotationsModel = models::AnnotationsModel::create();
            p.drawModel = models::DrawModel::create(getSettings());
            p.reviewMarkers = ftk::ObservableList<int>::create();
            // Introspection: what the models hold right now, as opposed to
            // the settings file, which holds what survived to the last
            // clean quit. Comparing this against a widget dump is how a
            // UI-versus-model desync is seen directly.
            p.commandsModel->add(
                "Debug/State",
                "Write the live model state as JSON, to a file or standard "
                "output; e.g., { \"file\": \"state.json\" }.",
                [this](const nlohmann::json& args)
                {
                    _debugStateCommand(args);
                });
        }

        void App::_observersInit()
        {
            FTK_P();

            p.player = ftk::Observable<std::shared_ptr<tl::Player> >::create();

            p.cacheObserver = ftk::Observer<tl::PlayerCacheOptions>::create(
                p.settingsModel->observeCache(),
                [this](const tl::PlayerCacheOptions& value)
                {
                    if (auto player = _p->player->get())
                    {
                        player->setCacheOptions(value);
                    }
                });

            // Most image sequence settings are read when a file is opened,
            // but the missing frame policy is one to change while looking at
            // a render in progress, so it is pushed to what is already open.
            // Setting the options clears the cache, so frames that were read
            // under the old policy are read again.
            //
            // A structural policy is the exception: it decides what clips the
            // timeline is built from, so it is settled when the file is opened
            // and the file has to be opened again.
            p.missingFrames = p.settingsModel->getImageSeq().io.missingFrames;
            p.imageSeqObserver = ftk::Observer<models::ImageSeqSettings>::create(
                p.settingsModel->observeImageSeq(),
                [this](const models::ImageSeqSettings& value)
                {
                    FTK_P();
                    const bool reopen =
                        value.io.missingFrames != p.missingFrames &&
                        (tl::isStructural(value.io.missingFrames) ||
                            tl::isStructural(p.missingFrames));
                    p.missingFrames = value.io.missingFrames;
                    if (reopen)
                    {
                        _reload(true);
                    }
                    else if (auto player = p.player->get())
                    {
                        player->setIOOptions(p.settingsModel->getIOOptions());
                    }
                });

            p.filesObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                p.filesModel->observeFiles(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    _filesUpdate(value);
                });
            p.reloadObserver = ftk::Observer<std::shared_ptr<models::FilesModelItem> >::create(
                p.filesModel->observeReload(),
                [this](const std::shared_ptr<models::FilesModelItem>& value)
                {
                    _reloadUpdate(value);
                });

            p.activeObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                p.filesModel->observeActive(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    _activeUpdate(value);
                });
            p.markersObserver = ftk::ListObserver<models::ReviewMarker>::create(
                p.markersModel->observeMarkers(),
                [this](const std::vector<models::ReviewMarker>&)
                {
                    _markersUpdate();
                    _markModified();
                });
            // Drawing lives with the Review tool: closing the tool disarms
            // the pen, or an invisible mode is left painting over playback
            // and the color picker.
            p.drawToolsObserver = ftk::ListObserver<std::string>::create(
                p.toolsModel->observeOpenTools(),
                [this](const std::vector<std::string>& value)
                {
                    FTK_P();
                    if (std::find(value.begin(), value.end(), "Review") ==
                        value.end())
                    {
                        p.drawModel->setEnabled(false);
                    }
                });
            p.annotationsObserver = ftk::ListObserver<models::ReviewAnnotation>::create(
                p.annotationsModel->observeAnnotations(),
                [this](const std::vector<models::ReviewAnnotation>&)
                {
                    _markersUpdate();
                    _markModified();
                });
            p.compareOptionsModifiedObserver = ftk::Observer<tl::CompareOptions>::create(
                p.filesModel->observeCompareOptions(),
                [this](const tl::CompareOptions&)
                {
                    _markModified();
                });
            p.bIndexesModifiedObserver = ftk::ListObserver<int>::create(
                p.filesModel->observeBIndexes(),
                [this](const std::vector<int>&)
                {
                    _markModified();
                });
            p.layersObserver = ftk::ListObserver<int>::create(
                p.filesModel->observeLayers(),
                [this](const std::vector<int>& value)
                {
                    _layersUpdate(value);
                });
            p.compareTimeObserver = ftk::Observer<tl::CompareTime>::create(
                p.filesModel->observeCompareTime(),
                [this](tl::CompareTime value)
                {
                    if (auto player = _p->player->get())
                    {
                        player->setCompareTime(value);
                    }
                });

            p.audioDeviceObserver = ftk::Observer<tl::AudioDeviceID>::create(
                p.audioModel->observeDevice(),
                [this](const tl::AudioDeviceID& value)
                {
                    if (auto player = _p->player->get())
                    {
                        player->setAudioDevice(value);
                    }
                });
            p.volumeObserver = ftk::Observer<float>::create(
                p.audioModel->observeVolume(),
                [this](float)
                {
                    _audioUpdate();
                });
            p.muteObserver = ftk::Observer<bool>::create(
                p.audioModel->observeMute(),
                [this](bool)
                {
                    _audioUpdate();
                });
            p.channelMuteObserver = ftk::ListObserver<bool>::create(
                p.audioModel->observeChannelMute(),
                [this](const std::vector<bool>&)
                {
                    _audioUpdate();
                });
            p.syncOffsetObserver = ftk::Observer<double>::create(
                p.audioModel->observeSyncOffset(),
                [this](double)
                {
                    _audioUpdate();
                });

            p.styleSettingsObserver = ftk::Observer<models::StyleSettings>::create(
                p.settingsModel->observeStyle(),
                [this](const models::StyleSettings& value)
                {
                    auto fontSystem = getFontSystem();
                    const auto& fonts = fontSystem->getFonts();
                    for (const auto& font : value.fontFiles)
                    {
                        if (!font.empty())
                        {
                            ftk::Path path(font);
                            const std::string fontName = path.getBase() + path.getNum();
                            const auto i = std::find(fonts.begin(), fonts.end(), fontName);
                            if (i == fonts.end())
                            {
                                fontSystem->addFont(fontName, font);
                            }
                        }
                    }
                    auto style = getStyle();
                    style->setColorControls(value.colorControls);
                    style->setFonts(value.fonts);
                    setColorStyle(value.colorStyle);
                    setCustomColorRoles(value.customColorRoles);
                    setDisplayScale(value.displayScale);
                });

            p.miscSettingsObserver = ftk::Observer<models::MiscSettings>::create(
                p.settingsModel->observeMisc(),
                [this](const models::MiscSettings& value)
                {
                    setTooltipsEnabled(value.tooltipsEnabled);
                });
        }

        void App::_inputFilesInit()
        {
            FTK_P();
            if (!p.cmdLine.inputs->getList().empty())
            {
                ftk::PathOptions pathOptions;
                pathOptions.seqMaxDigits = p.settingsModel->getImageSeq().maxDigits;

                // A review (".djvr") describes an entire session; open it and
                // ignore any other inputs.
                {
                    const std::filesystem::path firstPath = std::filesystem::u8path(
                        p.cmdLine.inputs->getList().front());
                    if (firstPath.extension() == models::reviewExtension())
                    {
                        openReview(firstPath);
                        return;
                    }
                }

                if (p.cmdLine.compareFileName->found())
                {
                    ftk::Path path(p.cmdLine.compareFileName->getValue());
                    if (path.hasSeqWildcard())
                    {
                        path = ftk::expandSeq(path, pathOptions);
                    }
                    open(path);
                    tl::CompareOptions options;
                    if (p.cmdLine.compare->found())
                    {
                        options.compare = p.cmdLine.compare->getValue();
                    }
                    if (p.cmdLine.wipeCenter->found())
                    {
                        options.wipeCenter = p.cmdLine.wipeCenter->getValue();
                    }
                    if (p.cmdLine.wipeRotation->found())
                    {
                        options.wipeRotation = p.cmdLine.wipeRotation->getValue();
                    }
                    p.filesModel->setCompareOptions(options);
                    p.filesModel->setB(0, true);
                }

                std::string audioFileName;
                if (p.cmdLine.audioFileName->found())
                {
                    audioFileName = p.cmdLine.audioFileName->getValue();
                }

                std::optional<ftk::RangeI64> frameRange;
                if (p.cmdLine.frameRange->found())
                {
                    frameRange = models::parseFrameRange(p.cmdLine.frameRange->getValue());
                }

                for (const auto& input : p.cmdLine.inputs->getList())
                {
                    ftk::Path path(input);
                    if (path.hasSeqWildcard())
                    {
                        path = ftk::expandSeq(path, pathOptions);
                    }
                    open(path, ftk::Path(audioFileName), frameRange);
                    // Only the first file opened takes the range.
                    frameRange.reset();

                    if (auto player = p.player->get())
                    {
                        if (p.cmdLine.speed->found())
                        {
                            player->setSpeed(p.cmdLine.speed->getValue());
                        }
                        if (p.cmdLine.timeUnits->found())
                        {
                            p.timeUnitsModel->setTimeUnits(p.cmdLine.timeUnits->getValue());
                        }
                        const double speed = player->getSpeed();
                        const tl::TimeUnits timeUnits = p.timeUnitsModel->getTimeUnits();

                        if (p.cmdLine.inPoint->found())
                        {
                            const auto inOutRange = OTIO_NS::TimeRange::range_from_start_end_time_inclusive(
                                models::parseTime(
                                    "in point",
                                    p.cmdLine.inPoint->getValue(),
                                    speed,
                                    timeUnits),
                                player->getInOutRange().end_time_inclusive());
                            player->setInOutRange(inOutRange);
                            player->seek(inOutRange.start_time());
                        }
                        if (p.cmdLine.outPoint->found())
                        {
                            const auto inOutRange = OTIO_NS::TimeRange::range_from_start_end_time_inclusive(
                                player->getInOutRange().start_time(),
                                models::parseTime(
                                    "out point",
                                    p.cmdLine.outPoint->getValue(),
                                    speed,
                                    timeUnits));
                            player->setInOutRange(inOutRange);
                            player->seek(inOutRange.start_time());
                        }
                        if (p.cmdLine.seek->found())
                        {
                            player->seek(models::parseTime(
                                "seek time",
                                p.cmdLine.seek->getValue(),
                                speed,
                                timeUnits));
                        }
                        if (p.cmdLine.loop->found())
                        {
                            player->setLoop(p.cmdLine.loop->getValue());
                        }
                        if (p.cmdLine.playback->found())
                        {
                            player->setPlayback(p.cmdLine.playback->getValue());
                        }
                    }
                }
            }
        }

        void App::_uiInit()
        {
            FTK_P();

            p.secondaryWindowActive = ftk::Observable<bool>::create(false);

            p.toolWidgetFactory = ToolWidgetFactory::create();
            p.toolWidgetFactory->addTool("Audio", &AudioTool::create);
            p.toolWidgetFactory->addTool("Color Picker", &ColorPickerTool::create);
            p.toolWidgetFactory->addTool("Color", &ColorTool::create);
            p.toolWidgetFactory->addTool("Diagnostics", &DiagTool::create);
            p.toolWidgetFactory->addTool("Export", &ExportTool::create);
            p.toolWidgetFactory->addTool("Files", &FilesTool::create);
            p.toolWidgetFactory->addTool("Information", &InfoTool::create);
            p.toolWidgetFactory->addTool("Magnify", &MagnifyTool::create);
            p.toolWidgetFactory->addTool("Messages", &MessagesTool::create);
            p.toolWidgetFactory->addTool("Review", &ReviewTool::create);
            p.toolWidgetFactory->addTool("Settings", &SettingsTool::create);
            p.toolWidgetFactory->addTool("System Log", &SysLogTool::create);
            p.toolWidgetFactory->addTool("View", &ViewTool::create);
        }

        void App::_mainWindowInit()
        {
            FTK_P();
            p.mainWindow = MainWindow::create(
                _context,
                std::dynamic_pointer_cast<App>(shared_from_this()));
            p.mainWindow->setCloseCallback(
                [this]
                {
                    FTK_P();
                    if (p.secondaryWindow)
                    {
                        p.secondaryWindow->close();
                        p.secondaryWindow.reset();
                    }
                    // A floating file browser is a window of its own, and
                    // the application runs until every window is gone.
                    _context->getSystem<ftk::FileBrowserSystem>()->close();
                });

            p.viewPosZoomObserver = ftk::Observer<std::pair<ftk::V2I, double> >::create(
                p.mainWindow->getViewport()->observeViewPosAndZoom(),
                [this](const std::pair<ftk::V2I, double>& value)
                {
                    _viewUpdate(
                        value.first,
                        value.second,
                        _p->mainWindow->getViewport()->hasFrameView());
                });
            p.viewFramedObserver = ftk::Observer<bool>::create(
                p.mainWindow->getViewport()->observeFramed(),
                [this](bool value)
                {
                    _viewUpdate(
                        _p->mainWindow->getViewport()->getViewPos(),
                        _p->mainWindow->getViewport()->getZoom(),
                        value);
                });

            // Apply any view state from a review opened before the window existed
            // (e.g. a ".djvr" passed on the command line).
            _applyReviewView();
            // Reflect a review opened before the window existed in the title.
            _updateWindowTitle();
            // Same for the timeline markers: the notes and annotations observers
            // fired while there was no window, so _markersUpdate() bailed out.
            _markersUpdate();

            // Start periodic crash-recovery autosave.
            p.autosaveTimer = ftk::Timer::create(_context);
            p.autosaveTimer->setRepeating(true);
            p.autosaveTimer->start(
                std::chrono::seconds(30),
                [this]
                {
                    _writeAutosave();
                });

            // Offer to recover a review from a crashed session, unless files were
            // already opened this launch (e.g. from the command line). A headless
            // run neither offers nor discards: the backup is the user's, and it
            // waits for them.
            if (p.recoveredAutosave.has_value() &&
                p.filesModel->getFiles().empty() &&
                !getHideSetup())
            {
                auto dialogSystem = _context->getSystem<ftk::DialogSystem>();
                dialogSystem->confirm(
                    "Recover Review",
                    "Unsaved review changes from a previous\n"
                    "session were found.\n"
                    "\n"
                    "Recover them?",
                    p.mainWindow,
                    [this](bool value)
                    {
                        FTK_P();
                        if (value)
                        {
                            _recoverAutosave();
                        }
                        else
                        {
                            p.recoveredAutosave.reset();
                            _deleteAutosave();
                        }
                    },
                    "Recover",
                    "Discard");
            }
            else
            {
                p.recoveredAutosave.reset();
            }
        }

        void App::_setAudioDeviceMute(bool value)
        {
            FTK_P();
            if (value == p.audioDeviceMute)
                return;
            p.audioDeviceMute = value;
            _audioUpdate();
        }


        void App::_filesUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >& files)
        {
            FTK_P();

            std::vector<std::shared_ptr<tl::Timeline> > timelines(files.size());
            for (size_t i = 0; i < files.size(); ++i)
            {
                const auto j = std::find(p.files.begin(), p.files.end(), files[i]);
                if (j != p.files.end())
                {
                    timelines[i] = p.timelines[j - p.files.begin()];
                }
            }

            for (size_t i = 0; i < files.size(); ++i)
            {
                if (!timelines[i])
                {
                    try
                    {
                        tl::Options options;
                        const models::ImageSeqSettings imageSeq = p.settingsModel->getImageSeq();
                        options.imageSeqAudio = imageSeq.audio;
                        options.imageSeqAudioExts = imageSeq.audioExts;
                        options.imageSeqAudioFileName = imageSeq.audioFileName;
                        const models::OTIOSettings otio = p.settingsModel->getOTIO();
                        options.spatial = otio.spatial;
                        options.compat = otio.compat;
                        options.ioOptions = p.settingsModel->getIOOptions();
                        options.pathOptions.seqMaxDigits = imageSeq.maxDigits;
                        options.readThreadCount = imageSeq.readThreadCount;

                        // A range that was asked for is used as it is. One
                        // that was not is looked for on disk again here, so
                        // that reopening picks up frames rendered since --
                        // the path holds the frames that were there when it
                        // was opened, and findSeq() is what goes and looks.
                        ftk::Path path = files[i]->path;
                        if (!files[i]->framesStated && path.isSeq())
                        {
                            const auto seq = ftk::findSeq(path, options.pathOptions);
                            if (!seq.empty())
                            {
                                // Only when something was found: a sequence
                                // that has gone from disk keeps the range it
                                // had rather than becoming a timeline of
                                // nothing.
                                path.setSeq(seq);
                            }
                        }
                        timelines[i] = tl::Timeline::create(
                            _context,
                            path,
                            files[i]->audioPath,
                            options);

                        // Opening a sequence finds the frames on disk, which
                        // the path does not know about when it names one
                        // file. Kept beside the path rather than folded into
                        // it: a path carrying a range is taken as a range
                        // that was asked for, and reopening would stop
                        // looking for frames that have arrived since.
                        const std::optional<OTIO_NS::TimeRange> prevTimeRange =
                            files[i]->timeRange;
                        files[i]->timeRange = timelines[i]->getTimeRange();

                        // An in/out range that was the whole file follows the
                        // file: it was never narrowed, only saved when the
                        // file last lost focus. Restoring it as it is would
                        // stop a reloaded sequence at where it used to end,
                        // which reads as the reload not finding the new
                        // frames at all. A narrowed range is kept; those are
                        // the user's marks.
                        if (files[i]->inOutRange.has_value() &&
                            prevTimeRange.has_value() &&
                            tl::compareExact(
                                files[i]->inOutRange.value(),
                                prevTimeRange.value()) &&
                            !tl::compareExact(
                                prevTimeRange.value(),
                                files[i]->timeRange.value()))
                        {
                            files[i]->inOutRange.reset();
                        }

                        // Replaced rather than added to: a file that is
                        // reopened comes back through here with its layers
                        // already listed from the time before.
                        files[i]->videoLayers.clear();
                        for (const auto& video : timelines[i]->getIOInfo().video)
                        {
                            files[i]->videoLayers.push_back(video.name);
                        }
                        if (files[i]->videoLayer >= files[i]->videoLayers.size())
                        {
                            files[i]->videoLayer = 0;
                        }

                        // Recorded here rather than when the file is opened:
                        // one that cannot be read should not be offered back
                        // in the recent files.
                        p.recentFilesModel->addRecent(files[i]->path);
                    }
                    catch (const std::exception& e)
                    {
                        _context->log("djv::app::App", e.what(), ftk::LogType::Error);
                        // Only a file that has just been opened is taken
                        // back out. Reloading runs through here too, and a
                        // file that has become unreadable since it was opened
                        // -- a share that went away, say -- should stay put
                        // rather than disappear from the session.
                        if (files[i]->newFile)
                        {
                            p.failedFiles.push_back(files[i]);
                        }
                    }
                }
            }

#if defined(__GLIBC__)
            // Closing a file frees its memory, but glibc keeps what was
            // freed in the allocator rather than returning it to the
            // system, so the process still appears to be holding it. Ask
            // for it back once the closed file's teardown has settled.
            if (files.size() < p.files.size())
            {
                if (!p.trimTimer)
                {
                    p.trimTimer = ftk::Timer::create(_context);
                }
                p.trimTimer->start(
                    std::chrono::seconds(2),
                    []
                    {
                        malloc_trim(0);
                    });
            }
#endif // __GLIBC__

            p.files = files;
            p.timelines = timelines;
            _colorModelUpdate();

            // A file that could not be opened should not sit in the tab bar
            // and the files tool as though it had.
            if (!p.failedFiles.empty())
            {
                if (!p.closeFailedTimer)
                {
                    p.closeFailedTimer = ftk::Timer::create(_context);
                }
                p.closeFailedTimer->start(
                    std::chrono::milliseconds(0),
                    [this] { _closeFailed(); });
            }
        }

        void App::_closeFailed()
        {
            FTK_P();
            auto failed = p.failedFiles;
            p.failedFiles.clear();
            for (const auto& item : failed)
            {
                const auto& files = p.filesModel->getFiles();
                const auto i = std::find(files.begin(), files.end(), item);
                if (i != files.end())
                {
                    p.filesModel->close(static_cast<int>(i - files.begin()));
                }
            }
        }

        void App::_reloadUpdate(const std::shared_ptr<models::FilesModelItem>& item)
        {
            FTK_P();
            if (!item)
            {
                return;
            }

            // Keep where playback had got to. _activeUpdate saves this for a
            // file that is losing focus, which this one is not.
            if (!p.activeFiles.empty() && p.activeFiles.front() == item)
            {
                if (auto player = p.player->get())
                {
                    item->speed = player->getSpeed();
                    item->currentTime = player->getCurrentTime();
                }
            }
            if (item->path.getFrames().has_value() &&
                item->currentTime.has_value())
            {
                const ftk::RangeI64& frames = item->path.getFrames().value();
                item->currentTime = OTIO_NS::RationalTime(
                    ftk::clamp(
                        item->currentTime->value(),
                        static_cast<double>(frames.min()),
                        static_cast<double>(frames.max())),
                    item->currentTime->rate());
            }

            const auto i = std::find(p.files.begin(), p.files.end(), item);
            if (i != p.files.end())
            {
                p.timelines[i - p.files.begin()].reset();
            }

            // Both updates decide what can be kept by comparing item
            // pointers, and the pointer has not changed, so the timeline and
            // the player it belongs to have to be taken out of the way first.
            p.activeFiles.clear();
            _filesUpdate(p.filesModel->getFiles());
            _activeUpdate(p.filesModel->getActive());

            // The item is the same object holding different things now -- its
            // range and its layers were filled in by the update above -- and
            // the list of them never changed, so say so, or the tools go on
            // showing what was there before the file was reopened.
            p.filesModel->refresh();
        }

        void App::_activeUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >& activeFiles)
        {
            FTK_P();

            if (!p.activeFiles.empty())
            {
                if (auto player = p.player->get())
                {
                    p.activeFiles.front()->speed = player->getSpeed();
                    p.activeFiles.front()->currentTime = player->getCurrentTime();
                    p.activeFiles.front()->inOutRange = player->getInOutRange();
                }
            }

            std::shared_ptr<tl::Player> player;
            if (!activeFiles.empty())
            {
                if (!p.activeFiles.empty() && activeFiles[0] == p.activeFiles[0])
                {
                    player = p.player->get();
                }
                else
                {
                    auto i = std::find(p.files.begin(), p.files.end(), activeFiles[0]);
                    if (i != p.files.end())
                    {
                        if (auto timeline = p.timelines[i - p.files.begin()])
                        {
                            try
                            {
                                tl::PlayerOptions playerOptions;
                                playerOptions.audioDevice = p.audioModel->getDevice();
                                playerOptions.cache = p.settingsModel->getCache();
                                playerOptions.audioBufferFrameCount =
                                    p.settingsModel->getAudio().bufferFrameCount;
                                player = tl::Player::create(_context, timeline, playerOptions);
                            }
                            catch (const std::exception& e)
                            {
                                _context->log("djv::app::App", e.what(), ftk::LogType::Error);
                            }
                        }
                    }
                }
            }
            if (player)
            {
                const double speed = activeFiles.front()->speed;
                if (speed >= 0.0)
                {
                    player->setSpeed(speed);
                }
                // Copied rather than referenced: the calls below are observed
                // back into the item they came from.
                const std::optional<OTIO_NS::TimeRange> inOutRange =
                    activeFiles.front()->inOutRange;
                if (inOutRange.has_value())
                {
                    player->setInOutRange(*inOutRange);
                }
                const std::optional<OTIO_NS::RationalTime> currentTime =
                    activeFiles.front()->currentTime;
                if (currentTime.has_value())
                {
                    player->seek(*currentTime);
                }
                std::vector<std::shared_ptr<tl::Timeline> > compare;
                for (size_t i = 1; i < activeFiles.size(); ++i)
                {
                    auto j = std::find(p.files.begin(), p.files.end(), activeFiles[i]);
                    if (j != p.files.end())
                    {
                        auto timeline = p.timelines[j - p.files.begin()];
                        compare.push_back(timeline);
                    }
                }
                player->setCompare(compare);
                player->setCompareTime(p.filesModel->getCompareTime());
                if (p.settingsModel->getPlayback().startPlayback &&
                    activeFiles.front()->newFile)
                {
                    player->forward();
                }
            }

            for (auto& file : p.files)
            {
                file->newFile = false;
            }

            p.activeFiles = activeFiles;
            p.player->setIfChanged(player);
            _colorModelUpdate();

            _layersUpdate(p.filesModel->observeLayers()->get());
            _audioUpdate();
        }

        void App::_colorModelUpdate()
        {
            FTK_P();
            // The paths and what each file itself says about its colors,
            // for resolving the input color spaces: the active file first,
            // then the compare files. Called from both the file and active
            // updates: whichever runs second has both the files and their
            // timelines.
            std::vector<std::pair<std::string, ftk::ImageTags> > activeFiles;
            for (const auto& file : p.activeFiles)
            {
                std::pair<std::string, ftk::ImageTags> item;
                item.first = file->path.get();
                const auto i = std::find(p.files.begin(), p.files.end(), file);
                if (i != p.files.end())
                {
                    if (const auto& timeline = p.timelines[i - p.files.begin()])
                    {
                        item.second = timeline->getIOInfo().tags;
                    }
                }
                activeFiles.push_back(item);
            }
            p.colorModel->setActiveFiles(activeFiles);
        }

        void App::_layersUpdate(const std::vector<int>& value)
        {
            FTK_P();
            if (auto player = p.player->get())
            {
                int videoLayer = 0;
                std::vector<int> compareVideoLayers;
                if (!value.empty() && value.size() == p.files.size() && !p.activeFiles.empty())
                {
                    auto i = std::find(p.files.begin(), p.files.end(), p.activeFiles.front());
                    if (i != p.files.end())
                    {
                        videoLayer = value[i - p.files.begin()];
                    }
                    for (size_t j = 1; j < p.activeFiles.size(); ++j)
                    {
                        i = std::find(p.files.begin(), p.files.end(), p.activeFiles[j]);
                        if (i != p.files.end())
                        {
                            compareVideoLayers.push_back(value[i - p.files.begin()]);
                        }
                    }
                }
                player->setVideoLayer(videoLayer);
                player->setCompareVideoLayers(compareVideoLayers);
            }
        }

        void App::_viewUpdate(const ftk::V2I& pos, double zoom, bool frame)
        {
            FTK_P();
            const ftk::Box2I& g = p.mainWindow->getViewport()->getGeometry();
            float scale = 1.F;
            if (p.secondaryWindow)
            {
                const ftk::Size2I& secondarySize = p.secondaryWindow->getViewport()->getGeometry().size();
                if (g.isValid() && secondarySize.isValid())
                {
                    scale = secondarySize.w / static_cast<float>(g.w());
                }
                p.secondaryWindow->setView(pos * scale, zoom * scale, frame);
            }
        }

        void App::_audioUpdate()
        {
            FTK_P();
            if (auto player = p.player->get())
            {
                player->setVolume(p.audioModel->getVolume());
                player->setMute(p.audioModel->isMuted() || p.audioDeviceMute);
                player->setChannelMute(p.audioModel->getChannelMute());
                player->setAudioOffset(p.audioModel->getSyncOffset());
            }
        }
    }
}
