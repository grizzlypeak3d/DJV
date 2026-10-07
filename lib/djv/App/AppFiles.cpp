// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/AppPrivate.h>
#include <djv/App/MainWindow.h>
#include <djv/Models/AudioModel.h>
#include <djv/Models/FilesModel.h>
#include <opentimelineio/externalReference.h>
#include <djv/Models/Review.h>
#include <djv/Models/TimeUnitsModel.h>

#include <djv/UI/SeparateAudioDialog.h>
#include <djv/Models/Parse.h>
#include <djv/Models/Playlist.h>

#include <opentimelineio/clip.h>
#include <djv/Models/RecentFilesModel.h>

#include <tlRender/UI/ThumbnailSystem.h>
#include <tlRender/Timeline/CompareOptions.h>
#include <tlRender/Timeline/Util.h>
#include <tlRender/IO/Plugin.h>
#include <tlRender/IO/System.h>

#include <ftk/GL/Window.h>
#include <ftk/UI/Settings.h>
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
                    open(value, ftk::Path(), std::optional<ftk::RangeI64>(), gatherSeq);
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

                models::playlistApply(playlist, p.filesModel);
                p.recentPlaylistsModel->addRecent(path);
                p.recentFilesModel->addRecent(path);
                p.recentDirsModel->addRecent(ftk::Path(path.getDir()));

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
                return;
            }
            // The file as saved, with the extension it was given: the name
            // as typed was listed too, and opening it failed.
            p.recentPlaylistsModel->addRecent(ftk::Path(fileName));
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

        std::vector<std::shared_ptr<models::FilesModelItem> > App::_openItems(
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
            const std::optional<ftk::RangeI64> inputFrames = path.getFrames();
            bool first = true;
            std::vector<std::shared_ptr<models::FilesModelItem> > items;
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
                if (first &&
                    dirListOptions.seq &&
                    inputFrames.has_value() &&
                    inputFrames->min() == inputFrames->max() &&
                    i.isSeq() &&
                    i.getFrames().has_value() &&
                    i.getFrames()->min() <= inputFrames->min() &&
                    inputFrames->min() <= i.getFrames()->max() &&
                    i.getFrames().value() != inputFrames.value())
                {
                    // One image was named and the gather grew it into its
                    // sequence, so playback starts at the image that was
                    // named -- the frame someone double-clicked is the one
                    // they want to look at (#490). Sequence time is the
                    // frame number, at the sequence rate the player will
                    // use.
                    item->currentTime = OTIO_NS::RationalTime(
                        static_cast<double>(inputFrames->min()),
                        p.settingsModel->getImageSeq().io.defaultSpeed);
                }
                first = false;
                item->audioPath = audioPath;
                items.push_back(item);
            }

            return items;
        }

        void App::open(
            const ftk::Path& path,
            const ftk::Path& audioPath,
            const std::optional<ftk::RangeI64>& frames,
            bool gatherSeq)
        {
            FTK_P();
            // Added together rather than one at a time: each add makes its
            // file the current one, so a directory would be opened file by
            // file on the way to the last of them.
            p.filesModel->add(_openItems(path, audioPath, frames, gatherSeq));
        }

        void App::open(
            const std::vector<ftk::Path>& paths,
            const ftk::Path& audioPath,
            const std::optional<ftk::RangeI64>& frames,
            bool gatherSeq)
        {
            FTK_P();
            std::vector<std::shared_ptr<models::FilesModelItem> > items;
            std::optional<ftk::RangeI64> itemFrames = frames;
            for (const auto& path : paths)
            {
                const auto i = _openItems(path, audioPath, itemFrames, gatherSeq);
                // The range describes one sequence, so it belongs to the
                // first file named rather than to each of them.
                itemFrames.reset();
                items.insert(items.end(), i.begin(), i.end());
            }
            p.filesModel->add(items);
        }

        void App::closeFile(int index)
        {
            FTK_P();
            if (p.filesModel->getFiles().size() <= 1)
            {
                // The last one out closes the review: what is left otherwise
                // is a session with no files and a panel still full of
                // feedback about frames that are no longer open.
                closeAllFiles();
                return;
            }
            if (index >= 0)
            {
                p.filesModel->close(index);
            }
            else
            {
                p.filesModel->close();
            }
        }

        void App::closeAllFiles()
        {
            closeReview();
        }

        namespace
        {
            //! The clips a timeline cannot show: no media reference at all,
            //! or one naming nothing. Those frames play as black, and
            //! nothing else in the application says why.
            std::vector<std::string> missingMedia(
                const OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline>& otio)
            {
                std::vector<std::string> out;
                if (!otio)
                {
                    return out;
                }
                for (const auto& clip : otio->find_children<OTIO_NS::Clip>())
                {
                    bool missing = false;
                    if (const auto* reference = clip->media_reference())
                    {
                        if (const auto* external =
                            dynamic_cast<const OTIO_NS::ExternalReference*>(reference))
                        {
                            missing = external->target_url().empty();
                        }
                    }
                    else
                    {
                        missing = true;
                    }
                    if (missing)
                    {
                        const std::string& name = clip->name();
                        out.push_back(!name.empty() ? name : std::string("unnamed clip"));
                    }
                }
                return out;
            }

        }

        const std::shared_ptr<models::RecentFilesModel>& App::getRecentPlaylistsModel() const
        {
            return _p->recentPlaylistsModel;
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
                    const std::filesystem::path firstPath = ftk::toFileSystem(
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

                std::vector<ftk::Path> paths;
                for (const auto& input : p.cmdLine.inputs->getList())
                {
                    ftk::Path path(input);
                    if (path.hasSeqWildcard())
                    {
                        path = ftk::expandSeq(path, pathOptions);
                    }
                    paths.push_back(path);
                }

                // Opened as one change: a file becomes the current one
                // as it is added, and the current file is the one that
                // gets read, so opening them one at a time would read
                // every one of them on the way to the last.
                const size_t filesBefore = p.filesModel->getFiles().size();
                open(paths, ftk::Path(audioFileName), frameRange);
                if (p.cmdLine.mediaReference->found())
                {
                    // Set on each file rather than on the player, so that it
                    // stays with the file. One that has not been opened yet
                    // is checked when it is.
                    const std::string key = p.cmdLine.mediaReference->getValue();
                    const auto& files = p.filesModel->getFiles();
                    for (size_t i = filesBefore; i < files.size(); ++i)
                    {
                        const auto& item = files[i];
                        if (!item->mediaReferenceKeysKnown)
                        {
                            p.filesModel->setMediaReferenceKey(item, key);
                        }
                        else if (const auto found = models::findMediaReferenceKey(*item, key))
                        {
                            p.filesModel->setMediaReferenceKey(item, found.value());
                        }
                        else
                        {
                            _mediaReferenceWarning(*item, key);
                        }
                    }
                }

                if (auto player = p.player->get())
                {
                    if (p.cmdLine.speed->found())
                    {
                        player->setSpeed(p.cmdLine.speed->getValue());
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
            _closeFailedLater();
        }

        std::shared_ptr<tl::Timeline> App::_getTimeline(size_t index)
        {
            FTK_P();
            if (index >= p.files.size() || index >= p.timelines.size())
            {
                return nullptr;
            }
            if (p.timelines[index])
            {
                return p.timelines[index];
            }
            const auto& item = p.files[index];
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

                // A range that was asked for is used as it is. One that was
                // not is looked for on disk again here, so that reopening
                // picks up frames rendered since -- the path holds the frames
                // that were there when it was opened, and findSeq() is what
                // goes and looks.
                ftk::Path path = item->path;
                if (!item->framesStated && path.isSeq())
                {
                    const auto seq = ftk::findSeq(path, options.pathOptions);
                    if (!seq.empty())
                    {
                        // Only when something was found: a sequence that has
                        // gone from disk keeps the range it had rather than
                        // becoming a timeline of nothing.
                        path.setSeq(seq);
                    }
                }
                auto timeline = tl::Timeline::create(
                    _context,
                    path,
                    item->audioPath,
                    options);
                p.timelines[index] = timeline;

                // What the file turned out to be, which is worth more than
                // the information request's answer: the range here is the
                // one the timeline composed, so a still paired with an audio
                // file lasts as long as the audio.
                const std::optional<OTIO_NS::TimeRange> prevTimeRange =
                    item->timeRange;
                item->timeRange = timeline->getTimeRange();

                // An in/out range that was the whole file follows the file:
                // it was never narrowed, only saved when the file last lost
                // focus. Restoring it as it is would stop a reloaded sequence
                // at where it used to end, which reads as the reload not
                // finding the new frames at all. A narrowed range is kept;
                // those are the user's marks.
                if (item->inOutRange.has_value() &&
                    prevTimeRange.has_value() &&
                    tl::compareExact(
                        item->inOutRange.value(),
                        prevTimeRange.value()) &&
                    !tl::compareExact(
                        prevTimeRange.value(),
                        item->timeRange.value()))
                {
                    item->inOutRange.reset();
                }

                // Replaced rather than added to: a file that is reopened
                // comes back through here with its layers already listed
                // from the time before.
                item->videoLayers.clear();
                for (const auto& video : timeline->getIOInfo().video)
                {
                    item->videoLayers.push_back(video.name);
                }
                if (item->videoLayer >= item->videoLayers.size())
                {
                    item->videoLayer = 0;
                }
                item->mediaReferenceKeys = timeline->getMediaReferenceKeys();
                item->mediaReferenceKeysKnown = true;
                // Only a timeline with a choice to make has keys worth
                // listing: one whose clips all use the one reference lists
                // that reference's key, which is no choice at all.
                if (item->mediaReferenceKeys.size() < 2)
                {
                    item->mediaReferenceKeys.clear();
                }
                // A key from a review, a playlist, or the command line
                // that the file does not use.
                if (!item->mediaReferenceKey.empty())
                {
                    if (const auto found = models::findMediaReferenceKey(*item, item->mediaReferenceKey))
                    {
                        item->mediaReferenceKey = found.value();
                    }
                    else
                    {
                        _mediaReferenceWarning(*item, item->mediaReferenceKey);
                        item->mediaReferenceKey.clear();
                    }
                }
                // The item became the current file before it was opened, so
                // whatever was shown of it was shown without these. Say so
                // once the caller is done: from here is inside the update
                // that asked for the timeline.
                p.filesChanged = true;

                // What was opened and what it turned out to be. The log
                // otherwise records the systems that were created and not a
                // single thing the person at the keyboard did, so a report
                // of "it went wrong after I opened the third one" has
                // nothing to match against.
                {
                    const auto& ioInfo = timeline->getIOInfo();
                    std::string what;
                    if (!ioInfo.video.empty())
                    {
                        what = ftk::Format("{0} {1}").
                            arg(ioInfo.video[0].size).
                            arg(ioInfo.video[0].type);
                    }
                    if (ioInfo.audio.isValid())
                    {
                        what += ftk::Format("{0}{1} channels {2} {3}Hz").
                            arg(what.empty() ? "" : ", ").
                            arg(ioInfo.audio.channelCount).
                            arg(ioInfo.audio.type).
                            arg(ioInfo.audio.sampleRate);
                    }
                    _context->log(
                        "djv::app::App",
                        ftk::Format("Opened \"{0}\": {1}, {2}").
                            arg(item->path.get()).
                            arg(what.empty() ? "no video or audio" : what).
                            arg(item->timeRange.has_value() ?
                                ftk::Format("{0}").arg(*item->timeRange).str() :
                                std::string("no time range")));
                }

                ++p.filesOpened;

                // A timeline can be read perfectly and still have nothing to
                // show for some of its frames.
                if (const auto missing = missingMedia(timeline->getOTIOTimeline());
                    !missing.empty())
                {
                    _context->log(
                        "djv::app::App",
                        ftk::Format("\"{0}\": {1} of {2} clips have no media and play as black: {3}").
                            arg(item->path.get()).
                            arg(missing.size()).
                            arg(timeline->getOTIOTimeline()->find_children<OTIO_NS::Clip>().size()).
                            arg(ftk::join(missing, ", ")),
                        ftk::LogType::Warning);
                }

                // Recorded here rather than when the file is opened: one
                // that cannot be read should not be offered back in the
                // recent files.
                p.recentFilesModel->addRecent(item->path);
                // Its directory as well, which is what the file browser
                // offers: a file opened from the command line or dropped on
                // the window never went through the browser.
                p.recentDirsModel->addRecent(ftk::Path(item->path.getDir()));
            }
            catch (const std::exception& e)
            {
                _context->log("djv::app::App", e.what(), ftk::LogType::Error);
                // Only a file that has just been opened is taken back out.
                // Reloading runs through here too, and a file that has become
                // unreadable since it was opened -- a share that went away,
                // say -- should stay put rather than disappear from the
                // session.
                if (item->newFile)
                {
                    p.failedFiles.push_back(item);
                    _closeFailedLater();
                }
            }
            return p.timelines[index];
        }

        void App::_closeFailedLater()
        {
            FTK_P();
            if (p.failedFiles.empty())
            {
                return;
            }
            if (!p.closeFailedTimer)
            {
                p.closeFailedTimer = ftk::Timer::create(_context);
            }
            p.closeFailedTimer->start(
                std::chrono::milliseconds(0),
                [this] { _closeFailed(); });
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
                        if (auto timeline = _getTimeline(i - p.files.begin()))
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
                        if (auto timeline = _getTimeline(j - p.files.begin()))
                        {
                            compare.push_back(timeline);
                        }
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
            // Before the old one goes: dropped frames are counted by the
            // player, and a session that switched files would otherwise end
            // reporting only the last one's.
            if (auto previous = p.player->get(); previous && previous != player)
            {
                p.droppedFrames += previous->getDroppedFrames();
            }
            p.player->setIfChanged(player);

            // A file that is not being shown keeps its timeline, so coming
            // back to it does not open it again, but not its readers: each
            // holds a decoder, about half a gigabyte for a UHD movie. After
            // the player change, so the player that was reading them is gone.
            for (size_t i = 0; i < p.files.size() && i < p.timelines.size(); ++i)
            {
                if (p.timelines[i] &&
                    std::find(activeFiles.begin(), activeFiles.end(), p.files[i]) == activeFiles.end())
                {
                    p.timelines[i]->closeReaders();
                }
            }
            _colorModelUpdate();

            _layersUpdate(p.filesModel->observeLayers()->get());
            _mediaReferenceKeysUpdate(p.filesModel->observeMediaReferenceKeys()->get());
            _audioUpdate();

            // Opening a timeline above filled in what its item holds -- the
            // frame range and the layers -- and the list of items did not
            // change, so nothing else says so. Announced from here rather
            // than from where it was filled in, which is partway through
            // this update.
            if (p.filesChanged)
            {
                p.filesChanged = false;
                p.filesModel->refresh();
            }
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

        void App::_mediaReferenceWarning(
            const models::FilesModelItem& item,
            const std::string& key)
        {
            // A warning so the status bar shows it: the file is shown as
            // authored, which is not what was asked for.
            std::vector<std::string> names;
            for (const auto& i : item.mediaReferenceKeys)
            {
                names.push_back(models::getMediaReferenceLabel(i));
            }
            _context->log(
                "djv::app::App",
                names.empty() ?
                    ftk::Format("{0}: no media reference \"{1}\"; it has only the one").
                        arg(item.path.getFileName()).
                        arg(key).str() :
                    ftk::Format("{0}: no media reference \"{1}\"; it has {2}").
                        arg(item.path.getFileName()).
                        arg(key).
                        arg(ftk::join(names, ", ")).str(),
                ftk::LogType::Warning);
        }

        void App::_mediaReferenceKeysUpdate(const std::vector<std::string>& value)
        {
            FTK_P();
            if (auto player = p.player->get())
            {
                // The same way as the layers: the "A" file's key for the
                // player, and each "B" file's own for its comparison.
                std::string key;
                std::vector<std::string> compareKeys;
                if (value.size() == p.files.size() && !p.activeFiles.empty())
                {
                    auto i = std::find(p.files.begin(), p.files.end(), p.activeFiles.front());
                    if (i != p.files.end())
                    {
                        key = value[i - p.files.begin()];
                    }
                    for (size_t j = 1; j < p.activeFiles.size(); ++j)
                    {
                        i = std::find(p.files.begin(), p.files.end(), p.activeFiles[j]);
                        if (i != p.files.end())
                        {
                            compareKeys.push_back(value[i - p.files.begin()]);
                        }
                    }
                }
                player->setMediaReferenceKey(key);
                player->setCompareMediaReferenceKeys(compareKeys);
            }
        }
    }
}
