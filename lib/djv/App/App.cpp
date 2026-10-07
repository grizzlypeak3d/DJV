// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/AppPrivate.h>
#include <djv/App/ShellAssociations.h>
#include <djv/UI/SysInfoDialog.h>
#include <djv/UI/Viewport.h>
#include <djv/Models/Version.h>

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
#include <djv/UI/StatusIndicator.h>
#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/AppInfoModel.h>
#include <djv/Models/AudioModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/Parse.h>

#include <djv/Models/MarkersModel.h>
#include <djv/Models/RecentFilesModel.h>
#include <djv/Models/Review.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/CommandsModel.h>
#include <djv/Models/ToolsModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/Timeline/CompareOptions.h>
#include <tlRender/Timeline/Util.h>
#include <tlRender/IO/Plugin.h>
#include <tlRender/IO/System.h>
#if defined(TLRENDER_FFMPEG_PLUGIN)
#include <tlRender/IO/FFmpeg.h>
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_USD)
#include <tlRender/IO/USD.h>
#endif // TLRENDER_USD

#include <ftk/GL/Window.h>
#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/Settings.h>
#include <ftk/UI/SysLogModel.h>
#include <ftk/Core/CmdLine.h>
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
            p.cmdLine.mediaReference = ftk::CmdLineOption<std::string>::create(
                { "-mediaReference", "-mr" },
                "Media reference to open OTIO timelines with, for clips that "
                "carry several versions of their media (e.g., \"Proxy\" or "
                "\"Full\"). Applies to the files opened, not the \"B\" file.",
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
            p.cmdLine.cacheVideoGB = ftk::CmdLineOption<float>::create(
                { "-cacheVideoGB" },
                "Video cache size in gigabytes.",
                "Cache");
            p.cmdLine.cacheAudioGB = ftk::CmdLineOption<float>::create(
                { "-cacheAudioGB" },
                "Audio cache size in gigabytes.",
                "Cache");
#if defined(TLRENDER_FFMPEG_PLUGIN)
            p.cmdLine.ffmpegThreadCount = ftk::CmdLineOption<int>::create(
                { "-ffmpegThreadCount" },
                "Number of FFmpeg decoding threads. Zero lets FFmpeg choose.",
                "FFmpeg");
#endif // TLRENDER_FFMPEG_PLUGIN
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
#if defined(TLRENDER_USD)
            p.cmdLine.usdRenderWidth = ftk::CmdLineOption<int>::create(
                { "-usdRenderWidth" },
                "Render width.",
                "USD",
                1920);
            p.cmdLine.usdComplexity = ftk::CmdLineOption<float>::create(
                { "-usdComplexity" },
                "Render complexity setting.",
                "USD",
                1.F);
            p.cmdLine.usdDrawMode = ftk::CmdLineOption<tl::usd::DrawMode>::create(
                { "-usdDrawMode" },
                "Draw mode.",
                "USD",
                tl::usd::DrawMode::ShadedSmooth,
                ftk::quotes(tl::usd::getDrawModeLabels()));
            p.cmdLine.usdEnableLighting = ftk::CmdLineOption<bool>::create(
                { "-usdEnableLighting" },
                "Enable lighting.",
                "USD",
                true);
            p.cmdLine.usdSRGB = ftk::CmdLineOption<bool>::create(
                { "-usdSRGB" },
                "Enable sRGB color space.",
                "USD",
                true);
            p.cmdLine.usdStageCacheCount = ftk::CmdLineOption<int>::create(
                { "-usdStageCache" },
                "Number of USD stages to cache.",
                "USD",
                10);
            p.cmdLine.usdDiskCacheGB = ftk::CmdLineOption<int>::create(
                { "-usdDiskCache" },
                "Disk cache size in gigabytes. A size of zero disables the cache.",
                "USD",
                0);
#endif // TLRENDER_USD
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
                p.cmdLine.mediaReference,
                p.cmdLine.inPoint,
                p.cmdLine.outPoint,
                p.cmdLine.dirFilter,
                p.cmdLine.dirDepth,
                p.cmdLine.cacheVideoGB,
                p.cmdLine.cacheAudioGB,
#if defined(TLRENDER_FFMPEG_PLUGIN)
                p.cmdLine.ffmpegThreadCount,
#endif // TLRENDER_FFMPEG_PLUGIN
#if defined(TLRENDER_OCIO)
                p.cmdLine.ocioFileName,
                p.cmdLine.ocioInput,
                p.cmdLine.ocioDisplay,
                p.cmdLine.ocioView,
                p.cmdLine.ocioLook,
                p.cmdLine.lutFileName,
                p.cmdLine.lutOrder,
#endif // TLRENDER_OCIO
#if defined(TLRENDER_USD)
                p.cmdLine.usdRenderWidth,
                p.cmdLine.usdComplexity,
                p.cmdLine.usdDrawMode,
                p.cmdLine.usdEnableLighting,
                p.cmdLine.usdSRGB,
                p.cmdLine.usdStageCacheCount,
                p.cmdLine.usdDiskCacheGB,
#endif // TLRENDER_USD
                p.cmdLine.hideSetup,
                p.cmdLine.version,
                p.cmdLine.sysInfo,
                p.cmdLine.listCommands,
                p.cmdLine.command,
                p.cmdLine.debugLoop,
                p.cmdLine.benchmark
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

        bool App::getHideSetup() const
        {
            return
                _p->cmdLine.hideSetup->found() ||
                _p->cmdLine.listCommands->found() ||
                _p->cmdLine.command->found() ||
                isCaptureRun() ||
                _p->cmdLine.benchmark->found();
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

        void App::_updateWindowTitle()
        {
            FTK_P();
            if (!p.mainWindow)
            {
                return;
            }
            std::string title = p.appInfoModel->getTitle();
            // What this window is showing, ahead of the application: several
            // players on a desktop are told apart by their titles, and a task
            // bar has room for the start of one. The review when there is one,
            // since that is what the session is, and otherwise the "A" file.
            // The name rather than the path, the way document titles usually
            // read; the Recent Reviews menu is where the whole paths are.
            if (!p.reviewPath.empty())
            {
                std::string review = ftk::fromFileSystem(p.reviewPath.filename());
                if (p.reviewModified)
                {
                    // Beside the name rather than at the end, where the
                    // application title would push it out of sight.
                    review += " *";
                }
                title = review + " - " + title;
            }
            else if (auto a = p.filesModel->getA())
            {
                title = a->path.getFileName() + " - " + title;
            }
            p.mainWindow->setTitle(title);
        }

        std::filesystem::path App::_previousLogPath() const
        {
            std::filesystem::path out = getLogFilePath();
            if (!out.empty())
            {
                out.replace_extension("prev.log");
            }
            return out;
        }

        std::shared_ptr<ftk::IObservable<std::shared_ptr<tl::Player> > > App::observePlayer() const
        {
            return _p->player;
        }

        const std::shared_ptr<ToolWidgetFactory>& App::getToolWidgetFactory() const
        {
            return _p->toolWidgetFactory;
        }

        bool App::isSetupReady() const
        {
            return true;
        }

        std::vector<std::shared_ptr<ftk::IWidget> > App::createSetupPages()
        {
            return {};
        }

        std::shared_ptr<ui::StatusIndicator> App::createIndicator()
        {
            FTK_P();
            return ui::StatusIndicator::create(
                _context,
                p.viewportModel,
                p.colorModel,
                p.audioModel,
                p.filesModel,
                p.toolsModel);
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
                    std::vector<std::pair<std::string, std::string> >(),
                {
                    { "Settings", ftk::fromFileSystem(getSettingsPath()) },
                    { "Log", ftk::fromFileSystem(getLogFilePath()) },
                    // The run before this one, which is the file to ask for
                    // after a crash: the log is written from the start each
                    // time, so the interesting one is not the one named
                    // above.
                    { "Previous log", ftk::fromFileSystem(_previousLogPath()) }
                });
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
                // The application's version, not this library's. They are
                // the same number in DJV and are not in an application built
                // on it, which then reported the version of the thing it was
                // built with instead of its own.
                std::cout << p.appInfoModel->getVersion() << std::endl;
                return;
            }
            else if (p.cmdLine.sysInfo->found())
            {
                std::cout << ftk::join(getSysInfo(), '\n') << std::endl;
                return;
            }

            // Here rather than first: a run that only prints something and
            // exits has no business changing the user's file associations.
            repairShellAssociations(_context);

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
                _logSessionSummary();
                _saveSettings();
                if (!benchmark->succeeded())
                {
                    throw std::runtime_error("The benchmark produced no measurement");
                }
                return;
            }

            ftk::App::run();
            _logSessionSummary();
            _saveSettings();
        }

        void App::_logSessionSummary()
        {
            FTK_P();

            // What the session amounted to, at the one point where it is all
            // known. Read errors are counted by the timelines and were not
            // shown anywhere; dropped frames are the playback complaint
            // people report and could not be answered from the log.
            size_t readErrorCount = 0;
            std::string readError;
            std::string readErrorPath;
            for (const auto& timeline : p.timelines)
            {
                if (timeline)
                {
                    readErrorCount += timeline->getReadErrorCount();
                    if (readError.empty())
                    {
                        readError = timeline->getReadError();
                        if (!readError.empty())
                        {
                            readErrorPath = timeline->getPath().get();
                        }
                    }
                }
            }
            size_t droppedFrames = p.droppedFrames;
            if (auto player = p.player->get())
            {
                droppedFrames += player->getDroppedFrames();
            }

            std::vector<std::string> lines;
            lines.push_back(std::string());
            lines.push_back(ftk::Format("    * Files opened: {0}").arg(p.filesOpened));
            lines.push_back(readErrorCount > 0 ?
                ftk::Format("    * Read errors: {0}, first \"{1}\" in \"{2}\"").
                    arg(readErrorCount).
                    arg(readError).
                    arg(readErrorPath).
                    str() :
                std::string("    * Read errors: none"));
            lines.push_back(ftk::Format("    * Dropped frames: {0}").arg(droppedFrames));
            _context->log("djv::app::App session", ftk::join(lines, "\n"));
        }

        std::shared_ptr<ftk::Capture> App::_createCapture(
            const std::filesystem::path& manifest,
            const std::string& shotId,
            const std::filesystem::path& outputDir)
        {
            return Capture::create(
                _context,
                std::dynamic_pointer_cast<App>(shared_from_this()),
                manifest,
                shotId,
                outputDir);
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

            // Whether the session would ask before closing, which is
            // otherwise only visible as a "*" in the window title and a
            // dialog that a scripted run never sees.
            nlohmann::json review;
            review["path"] = ftk::fromFileSystem(p.reviewPath);
            review["modified"] = p.reviewModified;
            review["markers"] = p.markersModel->getMarkers().size();
            review["annotations"] = p.annotationsModel->getAnnotations().size();
            out["review"] = review;

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
                j["mediaReferenceKey"] = player->getMediaReferenceKey();
                j["compareMediaReferenceKeys"] = player->getCompareMediaReferenceKeys();
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
                std::ofstream f(ftk::toFileSystem(file));
                f << out.dump(2) << std::endl;
            }
            else
            {
                std::cout << out.dump(2) << std::endl;
            }
        }

        void App::_exportMovieCommand(const nlohmann::json& args)
        {
            FTK_P();
            const auto get = [&args](const std::string& key, std::string& value)
            {
                if (args.is_object() && args.contains(key) && args.at(key).is_string())
                {
                    value = args.at(key).get<std::string>();
                }
            };
            const auto getBool = [&args](const std::string& key)
            {
                return
                    args.is_object() &&
                    args.contains(key) &&
                    args.at(key).is_boolean() &&
                    args.at(key).get<bool>();
            };

            // The tool first: opening it names the output after the file
            // being exported, which would otherwise undo a name given here.
            if (p.mainWindow)
            {
                p.toolsModel->setToolOpen("Export", true);
            }
            auto settings = p.settingsModel->getExport();
            settings.fileType = models::ExportFileType::Movie;
            get("dir", settings.dir);
            get("fileName", settings.movieFileName);
            get("ext", settings.movieExt);
            get("preset", settings.moviePreset);
            get("audioCodec", settings.movieAudioCodec);

            const std::string path = ftk::Path(
                settings.dir,
                settings.movieFileName + settings.movieExt).get();
            const bool exitWhenDone = getBool("exit");
            const auto done = [this, path, exitWhenDone](bool value)
            {
                _context->log(
                    "djv::app::App",
                    ftk::Format("Export/Movie: {0} \"{1}\"").
                        arg(value ? "wrote" : "did not write").
                        arg(path),
                    value ? ftk::LogType::Message : ftk::LogType::Error);
                if (exitWhenDone)
                {
                    exit();
                }
            };
            if (!p.mainWindow)
            {
                done(false);
                return;
            }
#if defined(TLRENDER_FFMPEG_PLUGIN)
            // The tool falls back to the first preset for a name it does not
            // know, which is right for a setting carried over from another
            // build and wrong for one a script just asked for.
            {
                std::vector<std::string> names;
                if (auto plugin = _context->getSystem<tl::WriteSystem>()->
                    getPlugin<tl::ffmpeg::WritePlugin>())
                {
                    for (const auto& preset : plugin->getWritePresets())
                    {
                        names.push_back(preset.name);
                    }
                }
                if (std::find(names.begin(), names.end(), settings.moviePreset) ==
                    names.end())
                {
                    _context->log(
                        "djv::app::App",
                        ftk::Format("Export/Movie: no preset \"{0}\"; this build "
                            "has: {1}").
                            arg(settings.moviePreset).
                            arg(ftk::join(names, ", ")),
                        ftk::LogType::Error);
                    done(false);
                    return;
                }

                // The extension likewise: the export tool offers the ones
                // the preset is written to, and a script asking for another
                // is told rather than quietly given one of them.
                for (const auto& preset : _context->getSystem<tl::WriteSystem>()->
                    getPlugin<tl::ffmpeg::WritePlugin>()->getWritePresets())
                {
                    if (preset.name == settings.moviePreset &&
                        !preset.exts.empty() &&
                        std::find(
                            preset.exts.begin(),
                            preset.exts.end(),
                            settings.movieExt) == preset.exts.end())
                    {
                        _context->log(
                            "djv::app::App",
                            ftk::Format("Export/Movie: \"{0}\" is not written to "
                                "\"{1}\"; it is written to: {2}").
                                arg(settings.moviePreset).
                                arg(settings.movieExt).
                                arg(ftk::join(preset.exts, ", ")),
                            ftk::LogType::Error);
                        done(false);
                        return;
                    }
                }
            }
#endif // TLRENDER_FFMPEG_PLUGIN
            p.settingsModel->setExport(settings);
            p.mainWindow->exportMovie(getBool("overwrite"), done);
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
    }
}
