// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/AppPrivate.h>
#include <tlRender/UI/TimelineWidget.h>
#include <djv/App/MainWindow.h>
#include <djv/UI/Viewport.h>
#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/AppInfoModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/Playlist.h>
#include <opentimelineio/externalReference.h>
#include <djv/Models/MarkersModel.h>
#include <djv/Models/ToolsModel.h>
#include <djv/Models/Version.h>
#include <djv/Models/ViewportModel.h>

#include <djv/Models/Parse.h>

#include <opentimelineio/clip.h>
#include <opentimelineio/track.h>
#include <djv/Models/RecentFilesModel.h>
#include <djv/Models/Review.h>

#include <tlRender/Timeline/CompareOptions.h>
#include <tlRender/Timeline/Util.h>
#include <tlRender/IO/Plugin.h>
#include <tlRender/IO/System.h>

#include <ftk/GL/Window.h>
#include <ftk/UI/DialogSystem.h>
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
            const std::string ext = ftk::toLower(ftk::fromFileSystem(path.extension()));
            if (".otio" == ext || ".otioz" == ext)
            {
                _importReviewTimeline(path);
                return;
            }

            models::Review review;
            try
            {
                review = models::reviewOpen(ftk::fromFileSystem(path));
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
                        arg(ftk::fromFileSystem(path)).
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
                    missing.push_back(ftk::fromFileSystem(resolved));
                }
                auto item = std::make_shared<models::FilesModelItem>();
                item->id = rf.id.empty() ? models::generateId() : rf.id;
                item->path = ftk::Path(ftk::fromFileSystem(resolved), pathOptions);
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
                        missing.push_back(ftk::fromFileSystem(audio));
                    }
                    item->audioPath = ftk::Path(ftk::fromFileSystem(audio), pathOptions);
                }
                item->videoLayer = static_cast<size_t>(std::max(0, rf.videoLayer));
                item->mediaReferenceKey = rf.mediaReferenceKey;
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
            p.recentReviewsModel->addRecent(ftk::Path(ftk::fromFileSystem(reviewPath)));
            p.reviewModified = false;
            _updateWindowTitle();
            // A freshly loaded review supersedes any earlier autosave.
            _deleteAutosave();

            if (!missing.empty())
            {
                _context->log(
                    "djv::app::App",
                    ftk::Format("Review \"{0}\": {1} file(s) not found: {2}").
                        arg(ftk::fromFileSystem(reviewPath)).
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
                                            ftk::toFileSystem(folder.get()));
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
                    ftk::fromFileSystem(p.reviewPath.filename());
            }
            options.extensions = { models::reviewExtension() };
            options.extensionsLabel = "Review Session";
            fileBrowserSystem->open(
                p.mainWindow,
                [callback](const ftk::Path& value)
                {
                    callback(ftk::toFileSystem(value.get()));
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
                rf.mediaReferenceKey = file->mediaReferenceKey;
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
                models::reviewSave(ftk::fromFileSystem(path), review);
            }
            catch (const std::exception& e)
            {
                _context->log("djv::app::App", e.what(), ftk::LogType::Error);
                return;
            }

            p.reviewPath = path;
            p.reviewRaw = review.raw;
            p.recentReviewsModel->addRecent(ftk::Path(ftk::fromFileSystem(path)));
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
            open(ftk::Path(ftk::fromFileSystem(path)));
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
                        ftk::toFileSystem(value.get()));
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
                ftk::fromFileSystem(p.reviewPath.stem()) + ".otio";
            options.extensions = { ".otio" };
            options.extensionsLabel = "Timeline";
            fileBrowserSystem->open(
                p.mainWindow,
                [this](const ftk::Path& value)
                {
                    std::filesystem::path path =
                        ftk::toFileSystem(value.get());
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
            if (!timeline->to_json_file(ftk::fromFileSystem(path), &errorStatus))
            {
                _context->log(
                    "djv::app::App",
                    ftk::Format("Cannot export markers \"{0}\": {1}").
                        arg(ftk::fromFileSystem(path)).
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
            // The directory the settings live in, taken from the settings
            // path rather than built again: built separately it used the
            // short name where the settings used the directory name, which
            // agree only on a file system that ignores case.
            return getSettingsPath().parent_path() /
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
                json["_autosaveReviewPath"] = ftk::fromFileSystem(p.reviewPath);
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
                    reviewPath = ftk::toFileSystem(
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
    }
}
