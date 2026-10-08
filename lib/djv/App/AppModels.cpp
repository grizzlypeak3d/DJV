// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/AppPrivate.h>
#include <djv/UI/Viewport.h>
#include <djv/App/MainWindow.h>
#include <djv/App/SecondaryWindow.h>
#include <djv/Models/SettingsKeys.h>
#include <djv/Models/Playlist.h>
#include <tlRender/IO/USD.h>
#include <ftk/UI/FileBrowser.h>

#include <djv/UI/ColorResetDialog.h>
#include <djv/Models/AnnotationsModel.h>
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

#include <tlRender/UI/FileBrowserThumbnails.h>
#include <tlRender/Timeline/AudioSystem.h>
#include <tlRender/Timeline/CompareOptions.h>
#include <tlRender/Timeline/Util.h>
#include <tlRender/IO/Plugin.h>
#include <tlRender/IO/System.h>
#if defined(TLRENDER_FFMPEG_PLUGIN)
#include <tlRender/IO/FFmpeg.h>
#endif // TLRENDER_FFMPEG_PLUGIN

#include <ftk/GL/Window.h>
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
        void App::colorResetDialog()
        {
            FTK_P();
            if (p.colorResetDialog)
            {
                return;
            }

            // What there is to reset. A section already at its defaults is
            // shown in the dialog but cannot be chosen, so what is offered
            // is what resetting would change (DJV #687).
            ui::ColorResetGroups set;
            const tl::DisplayOptions defaults;
            const tl::DisplayOptions display = p.viewportModel->getDisplayOptions();
#if defined(TLRENDER_OCIO)
            set.ocio =
                p.colorModel->getOCIOOptions() != models::ColorModel::getDefaultOCIOOptions() ||
                !p.colorModel->getExtColorSpaces().empty();
            set.lut = p.colorModel->getLUTOptions() != tl::LUTOptions();
#endif // TLRENDER_OCIO
            set.color =
                display.color != defaults.color ||
                display.exposure != defaults.exposure ||
                display.softClip != defaults.softClip;
            set.levels = display.levels != defaults.levels;

            p.colorResetDialog = ui::ColorResetDialog::create(_context, set);
            p.colorResetDialog->open(p.mainWindow);
            std::weak_ptr<models::CommandsModel> commandsWeak = p.commandsModel;
            p.colorResetDialog->setCallback(
                [commandsWeak](const ui::ColorResetGroups& value)
                {
                    if (auto commandsModel = commandsWeak.lock())
                    {
                        commandsModel->exec(
                            "Color/Reset",
                            {
                                { "ocio", value.ocio },
                                { "lut", value.lut },
                                { "color", value.color },
                                { "levels", value.levels }
                            });
                    }
                });
            p.colorResetDialog->setCloseCallback(
                [this]
                {
                    _p->colorResetDialog.reset();
                });
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
            p.recentReviewsModel->save();
            p.recentPlaylistsModel->save();
            p.recentDirsModel->save();
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
                getSettings());
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
#if defined(TLRENDER_USD)
            if (p.cmdLine.usdRenderWidth->found() ||
                p.cmdLine.usdComplexity->found() ||
                p.cmdLine.usdDrawMode->found() ||
                p.cmdLine.usdEnableLighting->found() ||
                p.cmdLine.usdSRGB->found() ||
                p.cmdLine.usdStageCacheCount->found() ||
                p.cmdLine.usdDiskCacheGB->found())
            {
                tl::usd::Options options = p.settingsModel->getUSD();
                if (p.cmdLine.usdRenderWidth->found())
                {
                    options.renderWidth = p.cmdLine.usdRenderWidth->getValue();
                }
                if (p.cmdLine.usdComplexity->found())
                {
                    options.complexity = p.cmdLine.usdComplexity->getValue();
                }
                if (p.cmdLine.usdDrawMode->found())
                {
                    options.drawMode = p.cmdLine.usdDrawMode->getValue();
                }
                if (p.cmdLine.usdEnableLighting->found())
                {
                    options.enableLighting = p.cmdLine.usdEnableLighting->getValue();
                }
                if (p.cmdLine.usdSRGB->found())
                {
                        options.sRGB = p.cmdLine.usdSRGB->getValue();
                }
                if (p.cmdLine.usdStageCacheCount->found())
                {
                    options.stageCacheCount = std::max(0, p.cmdLine.usdStageCacheCount->getValue());
                }
                if (p.cmdLine.usdDiskCacheGB->found())
                {
                    options.diskCacheGB = std::max(0, p.cmdLine.usdDiskCacheGB->getValue());
                }
                p.settingsModel->setUSD(options);
            }
#endif // TLRENDER_USD
            if (p.cmdLine.cacheVideoGB->found() ||
                p.cmdLine.cacheAudioGB->found())
            {
                tl::PlayerCacheOptions options = p.settingsModel->getCache();
                // No further than the machine has, as the settings are
                // held to; a cache larger than that takes the machine down
                // with it when a file fills it.
                const float maxGB = models::SettingsModel::getMaxCacheGB();
                const auto clamp = [this, maxGB](const std::string& name, float value)
                {
                    const float out = std::min(std::max(0.F, value), maxGB);
                    if (out != value)
                    {
                        _context->log(
                            "djv::app::App",
                            ftk::Format("{0}: {1} is more than this machine has; "
                                "using {2}").arg(name).arg(value).arg(out),
                            ftk::LogType::Warning);
                    }
                    return out;
                };
                if (p.cmdLine.cacheVideoGB->found())
                {
                    options.videoGB = clamp("-cacheVideoGB", p.cmdLine.cacheVideoGB->getValue());
                }
                if (p.cmdLine.cacheAudioGB->found())
                {
                    options.audioGB = clamp("-cacheAudioGB", p.cmdLine.cacheAudioGB->getValue());
                }
                p.settingsModel->setCache(options);
            }
#if defined(TLRENDER_FFMPEG_PLUGIN)
            if (p.cmdLine.ffmpegThreadCount->found())
            {
                tl::ffmpeg::Options options = p.settingsModel->getFFmpeg();
                options.threadCount = std::max(0, p.cmdLine.ffmpegThreadCount->getValue());
                p.settingsModel->setFFmpeg(options);
            }
#endif // TLRENDER_FFMPEG_PLUGIN

            p.sysLogModel = ftk::SysLogModel::create(_context);

            p.timeUnitsModel = models::TimeUnitsModel::create(_context, getSettings());
            // Applied here rather than with the files: the units are a
            // setting, not something of the file's, and without a file the
            // option was quietly ignored.
            if (p.cmdLine.timeUnits->found())
            {
                p.timeUnitsModel->setTimeUnits(p.cmdLine.timeUnits->getValue());
            }
            
            p.filesModel = models::FilesModel::create(getSettings());

            p.recentFilesModel = models::RecentFilesModel::create(_context, getSettings());
            // Reviews and playlists get their own recent lists, so opening
            // one does not push its media into the recent files.
            p.recentReviewsModel = models::RecentFilesModel::create(_context, getSettings(), models::settingsKeys::recentReviewsGroup);
            p.recentPlaylistsModel = models::RecentFilesModel::create(_context, getSettings(), models::settingsKeys::recentPlaylistsGroup);
            {
                // A playlist is always saved as ".otio", but saving one
                // under a name typed without the extension listed that name
                // as well, and it cannot be opened: drop those.
                std::vector<ftk::Path> recent;
                for (const auto& path : p.recentPlaylistsModel->getRecent())
                {
                    if (".otio" == ftk::toLower(path.getExt()))
                    {
                        recent.push_back(path);
                    }
                }
                if (recent.size() != p.recentPlaylistsModel->getRecent().size())
                {
                    p.recentPlaylistsModel->setRecent(recent);
                }
            }
            // What the file browser's recent list shows: the directories of
            // everything opened, and of anything chosen in the browser -- a
            // LUT, an export directory -- without those choices joining the
            // recent files. A group of its own: settingsKeys::fileBrowser is written
            // whole, and would take a list kept inside it along.
            p.recentDirsModel = models::RecentFilesModel::create(_context, getSettings(), models::settingsKeys::recentDirsGroup);
            auto fileBrowserSystem = _context->getSystem<ftk::FileBrowserSystem>();
            fileBrowserSystem->getModel()->setExts(tl::getExts(_context));
            // Offered by kind: one at a time there are too many to choose
            // from.
            std::vector<ftk::FileBrowserExtGroup> extGroups;
            for (const auto& group : tl::getExtGroups(_context))
            {
                extGroups.push_back({ group.label, group.exts });
            }
            fileBrowserSystem->getModel()->setExtGroups(extGroups);
            // From what the settings restored rather than from a fresh set:
            // the sequence extensions are the build's to say and cannot come
            // from a settings file, but everything else in there is the
            // user's and was just loaded.
            ftk::FileBrowserOptions fileBrowserOptions =
                fileBrowserSystem->getModel()->getOptions();
            fileBrowserOptions.dirList.seqExts = tl::getExts(_context, static_cast<int>(tl::FileType::Seq));
            fileBrowserSystem->getModel()->setOptions(fileBrowserOptions);
            fileBrowserSystem->setRecentDirsModel(p.recentDirsModel);

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
                    // The file only counts with the File source; otherwise
                    // the saved source (the built-in config by default)
                    // wins and the option is quietly ignored.
                    options.config = tl::OCIOConfig::File;
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
            p.commandsModel->add(
                "Export/Movie",
                "Export the current file as a movie with the Export tool's "
                "settings, changing any given first; e.g., { \"dir\": "
                "\"/tmp\", \"fileName\": \"out\", \"ext\": \".mov\", "
                "\"preset\": \"APV 422\", \"audioCodec\": \"Auto\", "
                "\"overwrite\": true, \"exit\": true }. \"exit\" quits once "
                "the movie is written or has failed.",
                [this](const nlohmann::json& args)
                {
                    _exportMovieCommand(args);
                });
        }

        void App::_observersInit()
        {
            FTK_P();

            p.player = ftk::Observable<std::shared_ptr<tl::Player> >::create();

            // The audio device's buffer size is the audio system's to give
            // the device, when it opens it: a player has no say in it.
            p.audioSettingsObserver = ftk::Observer<models::AudioSettings>::create(
                p.settingsModel->observeAudio(),
                [this](const models::AudioSettings& value)
                {
                    _context->getSystem<tl::AudioSystem>()->setBufferFrameCount(
                        value.bufferFrameCount);
                });

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

            // The file browser reads its thumbnails with the same I/O
            // settings the files are opened with. It read them with the
            // defaults, so with FFmpeg set to somewhere it is not otherwise
            // found the movies that need it opened and had no thumbnails.
            p.ioOptionsObserver = ftk::Observer<tl::IOOptions>::create(
                p.settingsModel->observeIOOptions(),
                [this](const tl::IOOptions& value)
                {
                    auto fileBrowserSystem = _context->getSystem<ftk::FileBrowserSystem>();
                    if (auto thumbnails = std::dynamic_pointer_cast<tl::ui::FileBrowserThumbnails>(
                        fileBrowserSystem->getThumbnails()))
                    {
                        thumbnails->setIOOptions(value);
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
                });
            p.markersModifiedObserver = ftk::ListObserver<models::ReviewMarker>::create(
                p.markersModel->observeMarkers(),
                [this](const std::vector<models::ReviewMarker>&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
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
                });
            p.annotationsModifiedObserver = ftk::ListObserver<models::ReviewAnnotation>::create(
                p.annotationsModel->observeAnnotations(),
                [this](const std::vector<models::ReviewAnnotation>&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            // What the review document holds, so that closing it asks
            // before dropping any of it. The open files above all: a review
            // with the wrong ones in it is not the review that was written.
            p.compareOptionsModifiedObserver = ftk::Observer<tl::CompareOptions>::create(
                p.filesModel->observeCompareOptions(),
                [this](const tl::CompareOptions&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            p.bIndexesModifiedObserver = ftk::ListObserver<int>::create(
                p.filesModel->observeBIndexes(),
                [this](const std::vector<int>&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            p.filesModifiedObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                p.filesModel->observeFiles(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            p.aIndexModifiedObserver = ftk::Observer<int>::create(
                p.filesModel->observeAIndex(),
                [this](int)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            // Which file is showing, rather than which index it sits at: a
            // file closed while another takes its place leaves the index
            // where it was, and the title named the file that had gone.
            p.windowTitleObserver = ftk::Observer<std::shared_ptr<models::FilesModelItem> >::create(
                p.filesModel->observeA(),
                [this](const std::shared_ptr<models::FilesModelItem>&)
                {
                    _updateWindowTitle();
                },
                ftk::ObserverAction::Suppress);
            p.layersModifiedObserver = ftk::ListObserver<int>::create(
                p.filesModel->observeLayers(),
                [this](const std::vector<int>&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            p.mediaReferenceKeysModifiedObserver = ftk::ListObserver<std::string>::create(
                p.filesModel->observeMediaReferenceKeys(),
                [this](const std::vector<std::string>&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            p.compareTimeModifiedObserver = ftk::Observer<tl::CompareTime>::create(
                p.filesModel->observeCompareTime(),
                [this](tl::CompareTime)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            // The color pipeline, which the log said nothing about: "why
            // does this look wrong" is the commonest report there is, and it
            // could not be answered from the file. The resolved options
            // rather than the settings as written, so the line says the
            // input color space the active file actually rendered through;
            // it only fires when something changes, so switching between
            // files that resolve the same way says nothing.
            p.ocioLogObserver = ftk::Observer<tl::OCIOOptions>::create(
                p.colorModel->observeResolvedOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                    _context->log(
                        "djv::app::App",
                        value.enabled ?
                            ftk::Format(
                                "OCIO: {0}, input \"{1}\", display \"{2}\", "
                                "view \"{3}\", look \"{4}\"").
                                arg(tl::OCIOConfig::BuiltIn == value.config ?
                                    std::string("built-in configuration") :
                                    ftk::Format("\"{0}\"").arg(value.fileName).str()).
                                arg(value.input).
                                arg(value.display).
                                arg(value.view).
                                arg(value.look).
                                str() :
                            std::string("OCIO: off"));
                });
            p.lutLogObserver = ftk::Observer<tl::LUTOptions>::create(
                p.colorModel->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                    _context->log(
                        "djv::app::App",
                        value.enabled && !value.fileName.empty() ?
                            ftk::Format("LUT: \"{0}\", {1}, {2}").
                                arg(value.fileName).
                                arg(value.direction).
                                arg(value.order).
                                str() :
                            std::string("LUT: off"));
                });

            // How the image is shown. A LUT turned off is a change to the
            // review in the same way a note is: the document carries it, so
            // closing without it asks first.
            p.ocioModifiedObserver = ftk::Observer<tl::OCIOOptions>::create(
                p.colorModel->observeOCIOOptions(),
                [this](const tl::OCIOOptions&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            p.lutModifiedObserver = ftk::Observer<tl::LUTOptions>::create(
                p.colorModel->observeLUTOptions(),
                [this](const tl::LUTOptions&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            p.displayModifiedObserver = ftk::Observer<tl::DisplayOptions>::create(
                p.viewportModel->observeDisplayOptions(),
                [this](const tl::DisplayOptions&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            p.backgroundModifiedObserver = ftk::Observer<tl::BackgroundOptions>::create(
                p.viewportModel->observeBackgroundOptions(),
                [this](const tl::BackgroundOptions&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            p.foregroundModifiedObserver = ftk::Observer<tl::ForegroundOptions>::create(
                p.viewportModel->observeForegroundOptions(),
                [this](const tl::ForegroundOptions&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            p.aspectRatioModifiedObserver = ftk::Observer<models::AspectRatioOptions>::create(
                p.viewportModel->observeAspectRatioOptions(),
                [this](const models::AspectRatioOptions&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            p.hudModifiedObserver = ftk::Observer<models::HUDOptions>::create(
                p.viewportModel->observeHUDOptions(),
                [this](const models::HUDOptions&)
                {
                    _markModified();
                },
                ftk::ObserverAction::Suppress);
            p.layersObserver = ftk::ListObserver<int>::create(
                p.filesModel->observeLayers(),
                [this](const std::vector<int>& value)
                {
                    _layersUpdate(value);
                });
            p.mediaReferenceKeysObserver = ftk::ListObserver<std::string>::create(
                p.filesModel->observeMediaReferenceKeys(),
                [this](const std::vector<std::string>& value)
                {
                    _mediaReferenceKeysUpdate(value);
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
            p.channelObserver = ftk::Observer<int>::create(
                p.filesModel->observeAudioChannel(),
                [this](int)
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
                    auto style = getStyle();
                    style->setColorControls(value.colorControls);
                    setColorStyle(value.colorStyle);
                    setDisplayScale(value.displayScale);
                });

            p.miscSettingsObserver = ftk::Observer<models::MiscSettings>::create(
                p.settingsModel->observeMisc(),
                [this](const models::MiscSettings& value)
                {
                    setTooltipsEnabled(value.tooltipsEnabled);
                });
        }

        void App::_setAudioDeviceMute(bool value)
        {
            FTK_P();
            if (value == p.audioDeviceMute)
                return;
            p.audioDeviceMute = value;
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
                // From the file the player is showing, which the "A" file
                // runs ahead of while a new one opens.
                const int channelCount = player->getIOInfo().audio.channelCount;
                const int channel = !p.activeFiles.empty() ?
                    p.activeFiles.front()->audioChannel :
                    -1;
                std::vector<bool> channelMute;
                if (channel >= 0 && channel < channelCount)
                {
                    channelMute.resize(channelCount, true);
                    channelMute[channel] = false;
                }
                player->setChannelMute(channelMute);
                player->setAudioOffset(p.audioModel->getSyncOffset());
            }
        }
    }
}
