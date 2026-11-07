// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/C/Models.h>

#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/AudioModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>

#include <opentimelineio/clip.h>
#include <djv/Models/HUD.h>
#include <djv/Models/Info.h>
#include <djv/Models/MarkersModel.h>
#include <djv/Models/OCIOModel.h>
#include <djv/Models/ExportRender.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/Shuttle.h>
#include <djv/Models/Stroke.h>
#include <djv/Models/Wipe.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ViewportModel.h>

#include <djv/Models/StrokeDraw.h>
#include <djv/UI/WipeDraw.h>

#include <tlRender/C/EngineCpp.h>

#include <ftk/UI/Settings.h>
#include <ftk/Core/Observable.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

using namespace djv;

struct djv_models
{
    std::shared_ptr<ftk::Context> context;
    std::shared_ptr<ftk::Settings> settings;
    std::shared_ptr<models::ColorModel> colorModel;
    std::shared_ptr<models::OCIOModel> ocioModel;
    std::shared_ptr<models::AudioModel> audioModel;
    std::shared_ptr<models::ViewportModel> viewportModel;
    std::shared_ptr<models::SettingsModel> settingsModel;
    std::shared_ptr<models::TimeUnitsModel> timeUnitsModel;
    std::shared_ptr<models::DrawModel> drawModel;
    std::shared_ptr<models::AnnotationsModel> annotationsModel;
    std::shared_ptr<models::MarkersModel> markersModel;
    std::shared_ptr<models::FilesModel> filesModel;

    // The name of a still, and why one was not written.
    std::string stillName;
    std::string stillError;

    // The shortcuts as last counted, and their keys' names.
    std::vector<models::Shortcut> shortcuts;
    std::vector<std::pair<std::string, std::string> > shortcutKeys;

    // The review open, and the file in it the player shows.
    models::Review review;
    std::string reviewPath;
    std::string fileId;
    std::string reviewError;

    // The media reference keys as last read, "As Authored" first; the
    // storage of a label and the key in use; and whether one was chosen
    // here, for the review to keep.
    std::vector<std::string> mediaReferenceKeys;
    std::string mediaReferenceLabel;
    std::string mediaReferenceKey;
    bool mediaReferenceChanged = false;

    // The review's media reference for the file shown, onto the player.
    void applyReviewMediaReference()
    {
        auto p = tl::c::getPlayer(player);
        if (!p)
            return;
        for (const auto& file : review.files)
        {
            if (file.id == fileId)
            {
                if (p->getMediaReferenceKey() != file.mediaReferenceKey)
                {
                    p->setMediaReferenceKey(file.mediaReferenceKey);
                }
                break;
            }
        }
    }

    // The stroke being drawn, or the eraser at work.
    bool drawing = false;
    bool erasing = false;
    bool erasingPen = false;
    // How many undo steps the eraser has made since it began, to take back
    // if the stroke is cancelled.
    size_t eraseSteps = 0;
    models::ReviewStroke stroke;
    OTIO_NS::RationalTime strokeTime;
    std::vector<double> frames;

    // The text being typed, which stays open after the tap that began it
    // until it is finished; and whether it is the selected stroke written
    // again rather than a new one.
    bool typing = false;
    bool typingSelection = false;

    // The selected stroke: the frame it is on and its place among that
    // frame's strokes. While it is moved, a copy with the move applied is
    // drawn in its place; the handle is the point of a shape dragged, or
    // none while the whole stroke moves.
    struct Selection
    {
        OTIO_NS::RationalTime time;
        size_t index = 0;
    };
    std::optional<Selection> selection;
    std::optional<models::ReviewStroke> moving;
    ftk::V2F movePressPos;
    int handle = -1;

    tl_view* view = nullptr;
    //! A second view, on a second display: the picture as the view has
    //! it, without what is there to be touched.
    tl_view* output = nullptr;
    tl_player* player = nullptr;
    bool compareFile = false;
    // The file compared with; whether the comparison changed since the
    // review was opened, which is when the review's is written over; and
    // the changes the models make themselves, which are not.
    std::string compareFilePath;
    bool compareChanged = false;
    bool compareQuiet = false;
    std::string reviewCompareFile;

    // The part of the wipe being dragged, and where it was taken from
    // the center.
    models::WipeHandle wipeDrag = models::WipeHandle::None;
    ftk::V2F wipeGrab;
    uint64_t generation = 0;

    // What the strings handed out point into.
    models::OCIOModelData ocioData;
    std::string resolvedInput;
    std::vector<std::string> aspectRatios;
    std::vector<models::InfoItem> info;
    std::string hudText;
    std::string timeLabel;
    std::string noteId;
    std::string author;
    std::vector<std::string> reviewFileNames;

    std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
    std::shared_ptr<ftk::Observer<tl::OCIOOptions> > colorOCIOOptionsObserver;
    std::shared_ptr<ftk::Observer<tl::OCIOOptions> > resolvedOCIOOptionsObserver;
    std::shared_ptr<ftk::Observer<models::OCIOModelData> > ocioDataObserver;
    std::shared_ptr<ftk::Observer<std::string> > resolvedInputObserver;
    std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
    std::shared_ptr<ftk::Observer<float> > volumeObserver;
    std::shared_ptr<ftk::Observer<bool> > muteObserver;
    std::shared_ptr<ftk::Observer<double> > syncOffsetObserver;
    std::shared_ptr<ftk::Observer<ftk::ImageOptions> > imageOptionsObserver;
    std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
    std::shared_ptr<ftk::Observer<tl::BackgroundOptions> > backgroundOptionsObserver;
    std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > foregroundOptionsObserver;
    std::shared_ptr<ftk::Observer<tl::PlayerCacheOptions> > cacheObserver;
    std::shared_ptr<ftk::ListObserver<models::ReviewAnnotation> > annotationsObserver;
    std::shared_ptr<ftk::ListObserver<models::ReviewMarker> > markersObserver;
    std::shared_ptr<ftk::Observer<models::ReviewSettings> > reviewSettingsObserver;
    std::shared_ptr<ftk::Observer<models::DrawTool> > drawToolObserver;
    std::shared_ptr<ftk::Observer<ftk::Color4F> > drawColorObserver;
    std::shared_ptr<ftk::Observer<float> > drawSizeObserver;
    std::shared_ptr<ftk::Observer<float> > drawTextSizeObserver;
    std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
    std::shared_ptr<ftk::Observer<tl::CompareTime> > compareTimeObserver;

    // Where the picture is: its box and image, and the view's pixels to
    // its render space.
    models::ImageMapping mapping() const
    {
        models::ImageMapping out;
        const auto picture = tl::c::getViewPicture(view);
        out.box = picture.box;
        out.imageSize = picture.imageSize;
        const auto& display = viewportModel->getDisplayOptions();
        out.mirrorX = display.mirror.x;
        out.mirrorY = display.mirror.y;
        return out;
    }

    ftk::V2F toRender(float x, float y) const
    {
        const auto picture = tl::c::getViewPicture(view);
        return picture.zoom > 0.F ?
            ftk::V2F((x - picture.pos.x) / picture.zoom, (y - picture.pos.y) / picture.zoom) :
            ftk::V2F();
    }

    std::optional<OTIO_NS::RationalTime> currentTime() const
    {
        std::optional<OTIO_NS::RationalTime> out;
        if (auto p = tl::c::getPlayer(player))
        {
            out = p->getCurrentTime();
        }
        return out;
    }

    void erase(const ftk::V2F& render)
    {
        const auto time = currentTime();
        const models::ImageMapping m = mapping();
        if (!time.has_value() || !m.contains(render))
            return;
        const auto picture = tl::c::getViewPicture(view);
        // A fingertip's reach for a finger, a pointer's for a pen, as the
        // desktop's viewport has them.
        const float reach = (erasingPen ? 8.F : 22.F) * picture.displayScale;
        const size_t before = annotationsModel->getStrokes(fileId, *time).size();
        annotationsModel->eraseStrokes(
            fileId,
            *time,
            m.toImage(render),
            models::eraserRadius(drawModel->getSize(), reach, m.scale() * picture.zoom));
        if (annotationsModel->getStrokes(fileId, *time).size() < before)
        {
            ++eraseSteps;
        }
    }

    // The strokes of the selection's frame, and the selected one, while
    // the frame shown is the one it is on and it is still there: a frame
    // changed, an undo or an erase takes it away.
    std::optional<models::ReviewStroke> selected() const
    {
        std::optional<models::ReviewStroke> out;
        const auto time = currentTime();
        if (selection.has_value() && time.has_value() && !fileId.empty() &&
            models::sameTime(selection->time, *time))
        {
            const auto strokes = annotationsModel->getStrokes(fileId, selection->time);
            if (selection->index < strokes.size())
            {
                out = strokes[selection->index];
            }
        }
        return out;
    }

    void selectionClear()
    {
        if (typingSelection)
        {
            textEnd(true);
        }
        if (selection.has_value())
        {
            selection.reset();
            moving.reset();
            handle = -1;
            needsDraw();
            ++generation;
        }
    }

    // The color or a size changed, for the selected stroke too, as on
    // the desktop: one undo step.
    void selectionUpdate(const std::function<void(models::ReviewStroke&)>& change)
    {
        if (moving.has_value() || typing)
            return;
        if (auto stroke = selected())
        {
            change(*stroke);
            annotationsModel->setStroke(fileId, selection->time, selection->index, *stroke);
        }
    }

    // A press with the Select tool, in the view's pixels: a handle of
    // the selected shape, or the stroke under it, as the desktop's
    // viewport takes them; false, and nothing selected, where there is
    // none.
    bool selectPress(float x, float y, bool pen)
    {
        const auto time = currentTime();
        const models::ImageMapping m = mapping();
        const ftk::V2F render = toRender(x, y);
        if (!time.has_value() || !m.isValid())
            return false;
        const auto picture = tl::c::getViewPicture(view);
        // A fingertip's reach for a finger, a pointer's for a pen.
        const float reach = (pen ? 8.F : 22.F) * picture.displayScale;
        handle = -1;
        if (const auto stroke = selected())
        {
            const auto handles = models::strokeHandles(*stroke);
            for (size_t i = 0; i < handles.size(); ++i)
            {
                const ftk::V2F r = m.toRender(handles[i]);
                const ftk::V2F s(picture.pos.x + r.x * picture.zoom, picture.pos.y + r.y * picture.zoom);
                if (std::abs(s.x - x) <= reach && std::abs(s.y - y) <= reach)
                {
                    handle = static_cast<int>(i);
                    moving = *stroke;
                    needsDraw();
                    return true;
                }
            }
        }
        std::optional<size_t> found;
        if (m.contains(render))
        {
            const float radius = reach / std::max(.001F, m.scale() * picture.zoom);
            found = annotationsModel->findStroke(fileId, *time, m.toImage(render), radius);
        }
        if (!found.has_value())
        {
            // A press away from the selection takes it away, and nothing
            // more; with none, the press is not the tool's.
            const bool had = selection.has_value();
            selectionClear();
            return had;
        }
        selection = Selection{ *time, *found };
        moving = annotationsModel->getStrokes(fileId, *time)[*found];
        movePressPos = m.toImage(render);
        needsDraw();
        ++generation;
        return true;
    }

    void selectMove(float x, float y)
    {
        const auto stroke = selected();
        const models::ImageMapping m = mapping();
        if (!stroke.has_value() || !moving.has_value() || !m.isValid())
            return;
        const ftk::V2F pos = m.clamp(m.toImage(toRender(x, y)));
        if (handle >= 0)
        {
            moving = models::moveHandle(*stroke, handle, pos, drawModel->isConstrain());
        }
        else
        {
            moving = models::moveStroke(
                *stroke,
                ftk::V2F(pos.x - movePressPos.x, pos.y - movePressPos.y),
                m.imageSize);
        }
        needsDraw();
    }

    // The text finished: kept, or dropped. The selected text written
    // again is kept as it is now, or removed when nothing is left of it.
    void textEnd(bool commit)
    {
        if (!typing)
            return;
        typing = false;
        if (typingSelection)
        {
            typingSelection = false;
            if (commit && selection.has_value())
            {
                if (!stroke.text.empty())
                {
                    annotationsModel->setStroke(fileId, selection->time, selection->index, stroke);
                }
                else
                {
                    annotationsModel->removeStroke(fileId, selection->time, selection->index);
                    selection.reset();
                }
            }
        }
        else if (commit && !stroke.text.empty())
        {
            annotationsModel->addStroke(fileId, strokeTime, stroke);
        }
        stroke = models::ReviewStroke();
        needsDraw();
        ++generation;
    }

    // The drawings of the frame shown, and the stroke being drawn, over
    // the picture with the desktop's stroke drawing.
    // On the output the selection is not framed and the text has no
    // caret: they are for whoever is drawing.
    void drawAnnotations(const std::shared_ptr<ftk::IRender>& render, tl_view* target)
    {
        const auto time = currentTime();
        const models::ImageMapping m = mapping();
        if (!time.has_value() || fileId.empty() || !m.isValid() || !isAShown())
            return;
        const bool touched = target == view;
        const auto picture = tl::c::getViewPicture(target);
        const auto toScreen = [&m, &picture](const ftk::V2F& point)
        {
            const ftk::V2F r = m.toRender(point);
            return ftk::V2F(picture.pos.x + r.x * picture.zoom, picture.pos.y + r.y * picture.zoom);
        };
        const float scale = m.scale() * picture.zoom;
        const auto fontSystem = context->getSystem<ftk::FontSystem>();
        const bool selectedHere = selection.has_value() && models::sameTime(selection->time, *time);
        const auto strokes = annotationsModel->getStrokes(fileId, *time);
        for (size_t i = 0; i < strokes.size(); ++i)
        {
            const bool isSelected = selectedHere && selection->index == i;
            // A stroke being moved or written again is drawn from the
            // copy below, not from here.
            if (isSelected && (moving.has_value() || typingSelection))
                continue;
            models::drawStroke(render, fontSystem, strokes[i], toScreen, scale);
            if (isSelected && touched)
            {
                models::drawSelection(render, fontSystem, strokes[i], toScreen, scale, picture.displayScale);
            }
        }
        if (selectedHere && moving.has_value())
        {
            models::drawStroke(render, fontSystem, *moving, toScreen, scale);
            if (touched)
            {
                models::drawSelection(render, fontSystem, *moving, toScreen, scale, picture.displayScale);
            }
        }
        if ((drawing || typing) && models::sameTime(strokeTime, *time))
        {
            models::drawStroke(render, fontSystem, stroke, toScreen, scale, 1.F, typing && touched);
        }
    }

    // The wipe's handles, in the view's pixels, while wiping.
    std::optional<models::WipeWidget> wipeWidget() const
    {
        std::optional<models::WipeWidget> out;
        const auto& options = filesModel->getCompareOptions();
        if (view && compareFile && tl::Compare::Wipe == options.compare)
        {
            const auto picture = tl::c::getViewPicture(view);
            if (picture.zoom > 0.F)
            {
                out = models::getWipeWidget(
                    options,
                    picture.box,
                    picture.pos,
                    picture.zoom,
                    ui::wipeRotateDistance * picture.displayScale);
            }
        }
        return out;
    }

    void drawWipe(const std::shared_ptr<ftk::IRender>& render)
    {
        if (const auto widget = wipeWidget())
        {
            ui::drawWipe(render, *widget, tl::c::getViewPicture(view).displayScale, wipeDrag);
        }
    }

    // Whether A is drawn: not when B is shown in its place. The
    // drawings are A's.
    bool isAShown() const
    {
        return tl::isShown(filesModel->getCompareOptions().compare, 0);
    }

    // The files the color is resolved for: A, then B.
    void activeFilesUpdate()
    {
        std::vector<std::pair<std::string, ftk::ImageTags> > files;
        if (auto timeline = tl::c::getTimeline(player))
        {
            files.push_back({ timeline->getPath().get(), timeline->getIOInfo().tags });
        }
        if (auto p = tl::c::getPlayer(player))
        {
            for (const auto& timeline : p->getCompare())
            {
                files.push_back({ timeline->getPath().get(), timeline->getIOInfo().tags });
            }
        }
        colorModel->setActiveFiles(files);
    }

    void framesUpdate()
    {
        frames.clear();
        auto p = tl::c::getPlayer(player);
        if (!p || fileId.empty())
            return;
        const OTIO_NS::RationalTime start = p->getTimeRange().start_time();
        for (const auto& annotation : annotationsModel->getAnnotations())
        {
            if (annotation.sourceId == fileId &&
                annotation.time.has_value() &&
                !annotation.strokes.empty())
            {
                frames.push_back((*annotation.time - start).rescaled_to(1.0).value());
            }
        }
        std::sort(frames.begin(), frames.end());
        frames.erase(std::unique(frames.begin(), frames.end()), frames.end());
    }

    // The review's file at a path: by its path, or by its name, as the
    // desktop's relocation finds a file that moved. Empty for none.
    std::string reviewFileId(const std::filesystem::path& media) const
    {
        const auto base = std::filesystem::u8path(reviewPath).parent_path();
        for (const auto& file : review.files)
        {
            const std::filesystem::path relative = std::filesystem::u8path(file.path);
            const std::filesystem::path absolute = std::filesystem::u8path(file.pathAbsolute);
            if ((!file.path.empty() && (base / relative).lexically_normal() == media.lexically_normal()) ||
                (!file.pathAbsolute.empty() && absolute == media) ||
                (!file.path.empty() && relative.filename() == media.filename()))
            {
                return file.id;
            }
        }
        return std::string();
    }

    // Add a file to the review, its path relative to the review's and
    // absolute; its id.
    std::string reviewFileAdd(const std::string& path)
    {
        const auto base = std::filesystem::u8path(reviewPath).parent_path();
        models::ReviewFile file;
        file.id = models::generateId();
        file.path = models::reviewGenericPath(models::reviewRelativePath(path, base));
        file.pathAbsolute = models::reviewGenericPath(path);
        review.files.push_back(file);
        return file.id;
    }

    // The comparison into the review, as the desktop writes it: "A", the
    // file compared with as "B", the options and the time. A "B" taken
    // out goes from the files too, unless something else is about it.
    void reviewCompareUpdate()
    {
        auto& compare = review.compare;
        for (const auto& id : compare.bIds)
        {
            const bool used =
                id == fileId ||
                std::any_of(
                    review.annotations.begin(),
                    review.annotations.end(),
                    [&id](const models::ReviewAnnotation& a) { return a.sourceId == id; });
            if (!used)
            {
                review.files.erase(
                    std::remove_if(
                        review.files.begin(),
                        review.files.end(),
                        [&id](const models::ReviewFile& f) { return f.id == id; }),
                    review.files.end());
            }
        }
        compare.aId = fileId;
        compare.bIds.clear();
        if (compareFile && !compareFilePath.empty())
        {
            const std::filesystem::path path = std::filesystem::u8path(compareFilePath);
            std::string id = reviewFileId(path);
            if (id.empty() || id == fileId)
            {
                id = reviewFileAdd(compareFilePath);
            }
            compare.bIds.push_back(id);
        }
        compare.options = filesModel->getCompareOptions();
        compare.time = filesModel->getCompareTime();
    }

    // The views the options are drawn with.
    std::vector<tl_view*> views() const
    {
        std::vector<tl_view*> out;
        if (view)
            out.push_back(view);
        if (output)
            out.push_back(output);
        return out;
    }

    void needsDraw()
    {
        for (tl_view* v : views())
        {
            tl::c::setNeedsDraw(v);
        }
    }

    // The options as a view and the player should have them now.
    void applyToView(tl_view* view)
    {
        if (!view)
            return;
        tl::c::setOCIOOptions(view, colorModel->observeResolvedOCIOOptions()->get());
        tl::c::setOCIOInputResolver(
            view,
            [this](const std::string& path, const ftk::ImageTags& tags)
            {
                return colorModel->resolveInput(path, tags);
            });
        tl::c::setLUTOptions(view, colorModel->getLUTOptions());
        tl::c::setImageOptions(view, viewportModel->getImageOptions());
        tl::c::setDisplayOptions(view, viewportModel->getDisplayOptions());
        tl::c::setBackgroundOptions(view, viewportModel->getBackgroundOptions());
        tl::c::setForegroundOptions(view, viewportModel->getForegroundOptions());
        tl::c::setCompareOptions(view, filesModel->getCompareOptions());
    }

    void applyToPlayer()
    {
        if (auto p = tl::c::getPlayer(player))
        {
            p->setVolume(audioModel->getVolume());
            p->setMute(audioModel->isMuted());
            p->setAudioOffset(audioModel->getSyncOffset());
            p->setCacheOptions(settingsModel->getCache());
            p->setCompareTime(filesModel->getCompareTime());
        }
    }
};

djv_models* djv_models_create(tl_engine* engine, const char* settingsPath)
{
    auto context = tl::c::getContext(engine);
    if (!context || !settingsPath)
        return nullptr;
    auto out = new djv_models;
    try
    {
        out->context = context;
        out->settings = ftk::Settings::create(context, settingsPath);
        out->colorModel = models::ColorModel::create(context, out->settings);
        out->ocioModel = models::OCIOModel::create(context);
        out->audioModel = models::AudioModel::create(context, out->settings);
        out->viewportModel = models::ViewportModel::create(context, out->settings);
        out->settingsModel = models::SettingsModel::create(context, out->settings);
        // The desktop registers its shortcuts with its actions; here there
        // are none, so the table of defaults is registered whole.
        out->settingsModel->addShortcuts(models::getDefaultShortcuts());
        out->timeUnitsModel = models::TimeUnitsModel::create(context, out->settings);
        out->drawModel = models::DrawModel::create(out->settings);
        out->annotationsModel = models::AnnotationsModel::create();
        out->markersModel = models::MarkersModel::create();
        out->filesModel = models::FilesModel::create(out->settings);
    }
    catch (const std::exception&)
    {
        delete out;
        return nullptr;
    }

    // The OpenColorIO model offers the configuration's choices and keeps
    // the color model's options, each following the other, as the
    // desktop's color widgets have them.
    out->ocioModel->setOptions(out->colorModel->getOCIOOptions());
    out->ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
        out->ocioModel->observeOptions(),
        [out](const tl::OCIOOptions& value)
        {
            out->colorModel->setOCIOOptions(value);
        });
    out->colorOCIOOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
        out->colorModel->observeOCIOOptions(),
        [out](const tl::OCIOOptions& value)
        {
            out->ocioModel->setOptions(value);
            ++out->generation;
        });
    out->ocioDataObserver = ftk::Observer<models::OCIOModelData>::create(
        out->ocioModel->observeData(),
        [out](const models::OCIOModelData& value)
        {
            out->ocioData = value;
            ++out->generation;
        });
    out->resolvedOCIOOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
        out->colorModel->observeResolvedOCIOOptions(),
        [out](const tl::OCIOOptions& value)
        {
            for (tl_view* view : out->views())
            {
                tl::c::setOCIOOptions(view, value);
            }
            ++out->generation;
        });
    out->resolvedInputObserver = ftk::Observer<std::string>::create(
        out->colorModel->observeResolvedInput(),
        [out](const std::string& value)
        {
            out->resolvedInput = value;
            ++out->generation;
        });
    out->lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
        out->colorModel->observeLUTOptions(),
        [out](const tl::LUTOptions& value)
        {
            for (tl_view* view : out->views())
            {
                tl::c::setLUTOptions(view, value);
            }
            ++out->generation;
        });
    out->volumeObserver = ftk::Observer<float>::create(
        out->audioModel->observeVolume(),
        [out](float)
        {
            out->applyToPlayer();
            ++out->generation;
        });
    out->muteObserver = ftk::Observer<bool>::create(
        out->audioModel->observeMute(),
        [out](bool)
        {
            out->applyToPlayer();
            ++out->generation;
        });
    out->syncOffsetObserver = ftk::Observer<double>::create(
        out->audioModel->observeSyncOffset(),
        [out](double)
        {
            out->applyToPlayer();
            ++out->generation;
        });
    out->imageOptionsObserver = ftk::Observer<ftk::ImageOptions>::create(
        out->viewportModel->observeImageOptions(),
        [out](const ftk::ImageOptions& value)
        {
            for (tl_view* view : out->views())
            {
                tl::c::setImageOptions(view, value);
            }
            ++out->generation;
        });
    out->displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
        out->viewportModel->observeDisplayOptions(),
        [out](const tl::DisplayOptions& value)
        {
            for (tl_view* view : out->views())
            {
                tl::c::setDisplayOptions(view, value);
            }
            ++out->generation;
        });
    out->backgroundOptionsObserver = ftk::Observer<tl::BackgroundOptions>::create(
        out->viewportModel->observeBackgroundOptions(),
        [out](const tl::BackgroundOptions& value)
        {
            for (tl_view* view : out->views())
            {
                tl::c::setBackgroundOptions(view, value);
            }
            ++out->generation;
        });
    out->foregroundOptionsObserver = ftk::Observer<tl::ForegroundOptions>::create(
        out->viewportModel->observeForegroundOptions(),
        [out](const tl::ForegroundOptions& value)
        {
            for (tl_view* view : out->views())
            {
                tl::c::setForegroundOptions(view, value);
            }
            ++out->generation;
        });
    out->annotationsObserver = ftk::ListObserver<models::ReviewAnnotation>::create(
        out->annotationsModel->observeAnnotations(),
        [out](const std::vector<models::ReviewAnnotation>&)
        {
            // The selection names a stroke by its place; an undo or an
            // erase can take that place away.
            if (out->selection.has_value() && !out->moving.has_value() &&
                out->selection->index >= out->annotationsModel->getStrokes(
                    out->fileId, out->selection->time).size())
            {
                out->selection.reset();
            }
            out->framesUpdate();
            out->needsDraw();
            ++out->generation;
        });
    out->drawToolObserver = ftk::Observer<models::DrawTool>::create(
        out->drawModel->observeTool(),
        [out](models::DrawTool value)
        {
            if (value != models::DrawTool::Select)
            {
                out->selectionClear();
            }
            ++out->generation;
        });
    // The color and the sizes are for the next stroke, and for the
    // selected one while there is one.
    out->drawColorObserver = ftk::Observer<ftk::Color4F>::create(
        out->drawModel->observeColor(),
        [out](const ftk::Color4F& value)
        {
            out->selectionUpdate([&value](models::ReviewStroke& s) { s.color = value; });
            ++out->generation;
        });
    out->drawSizeObserver = ftk::Observer<float>::create(
        out->drawModel->observeSize(),
        [out](float value)
        {
            out->selectionUpdate([value](models::ReviewStroke& s) { s.width = value; });
            ++out->generation;
        });
    out->drawTextSizeObserver = ftk::Observer<float>::create(
        out->drawModel->observeTextSize(),
        [out](float value)
        {
            out->selectionUpdate(
                [value](models::ReviewStroke& s)
                {
                    if (models::ReviewStrokeKind::Text == s.kind)
                    {
                        s.textSize = value;
                    }
                });
            ++out->generation;
        });
    out->markersObserver = ftk::ListObserver<models::ReviewMarker>::create(
        out->markersModel->observeMarkers(),
        [out](const std::vector<models::ReviewMarker>&) { ++out->generation; });
    // The notes and drawings are signed with the author from the
    // settings, as the desktop signs them.
    out->reviewSettingsObserver = ftk::Observer<models::ReviewSettings>::create(
        out->settingsModel->observeReview(),
        [out](const models::ReviewSettings& value)
        {
            models::setReviewAuthor(value.author);
            ++out->generation;
        });
    out->compareOptionsObserver = ftk::Observer<tl::CompareOptions>::create(
        out->filesModel->observeCompareOptions(),
        [out](const tl::CompareOptions& value)
        {
            for (tl_view* view : out->views())
            {
                tl::c::setCompareOptions(view, value);
            }
            if (!out->isAShown())
            {
                djv_draw_cancel(out);
            }
            if (!out->compareQuiet)
            {
                out->compareChanged = true;
            }
            ++out->generation;
        });
    out->compareTimeObserver = ftk::Observer<tl::CompareTime>::create(
        out->filesModel->observeCompareTime(),
        [out](tl::CompareTime value)
        {
            if (auto p = tl::c::getPlayer(out->player))
            {
                p->setCompareTime(value);
            }
            if (!out->compareQuiet)
            {
                out->compareChanged = true;
            }
            ++out->generation;
        });
    out->cacheObserver = ftk::Observer<tl::PlayerCacheOptions>::create(
        out->settingsModel->observeCache(),
        [out](const tl::PlayerCacheOptions& value)
        {
            if (auto p = tl::c::getPlayer(out->player))
            {
                p->setCacheOptions(value);
            }
            ++out->generation;
        });
    return out;
}

void djv_models_destroy(djv_models* models)
{
    if (!models)
        return;
    // The observers first: they hold the models.
    for (tl_view* view : models->views())
    {
        tl::c::setOverlay(view, nullptr);
    }
    models->annotationsObserver.reset();
    models->markersObserver.reset();
    models->reviewSettingsObserver.reset();
    models->drawToolObserver.reset();
    models->drawColorObserver.reset();
    models->drawSizeObserver.reset();
    models->drawTextSizeObserver.reset();
    models->compareOptionsObserver.reset();
    models->compareTimeObserver.reset();
    models->cacheObserver.reset();
    models->foregroundOptionsObserver.reset();
    models->backgroundOptionsObserver.reset();
    models->displayOptionsObserver.reset();
    models->imageOptionsObserver.reset();
    models->syncOffsetObserver.reset();
    models->muteObserver.reset();
    models->volumeObserver.reset();
    models->lutOptionsObserver.reset();
    models->resolvedInputObserver.reset();
    models->resolvedOCIOOptionsObserver.reset();
    models->ocioDataObserver.reset();
    models->colorOCIOOptionsObserver.reset();
    models->ocioOptionsObserver.reset();
    delete models;
}

void djv_models_save(djv_models* models)
{
    if (!models)
        return;
    models->colorModel->save();
    models->audioModel->save();
    models->viewportModel->save();
    models->settingsModel->save();
    models->timeUnitsModel->save();
    models->filesModel->save();
    models->drawModel->save();
    models->settings->save();
}

void djv_models_bind_view(djv_models* models, tl_view* view)
{
    if (!models)
        return;
    models->view = view;
    models->applyToView(view);
    if (view)
    {
        tl::c::setOverlay(
            view,
            [models, view](const std::shared_ptr<ftk::IRender>& render)
            {
                models->drawAnnotations(render, view);
                models->drawWipe(render);
            });
    }
}

void djv_models_bind_output(djv_models* models, tl_view* view)
{
    if (!models)
        return;
    if (models->output && models->output != view)
    {
        tl::c::setOverlay(models->output, nullptr);
    }
    models->output = view;
    models->applyToView(view);
    if (view)
    {
        tl::c::setOverlay(
            view,
            [models, view](const std::shared_ptr<ftk::IRender>& render)
            {
                models->drawAnnotations(render, view);
            });
    }
}

void djv_models_set_player(djv_models* models, tl_player* player)
{
    if (!models)
        return;
    models->player = player;
    // A new player is compared with nothing.
    models->compareFile = false;
    models->compareFilePath.clear();
    tl_player_set_compare(player, nullptr);
    auto compare = models->filesModel->getCompareOptions();
    compare.compare = tl::Compare::None;
    models->compareQuiet = true;
    models->filesModel->setCompareOptions(compare);
    models->compareQuiet = false;
    models->activeFilesUpdate();
    models->applyToPlayer();
    models->applyReviewMediaReference();
    djv_draw_cancel(models);
    models->framesUpdate();
    ++models->generation;
}

uint64_t djv_models_get_generation(djv_models* models)
{
    return models ? models->generation : 0;
}

// OpenColorIO.

int djv_ocio_is_available(void)
{
    return models::getOCIOVersion().empty() ? 0 : 1;
}

int djv_ocio_is_enabled(djv_models* models)
{
    return models ? (models->ocioData.enabled ? 1 : 0) : 0;
}

void djv_ocio_set_enabled(djv_models* models, int value)
{
    if (models)
        models->ocioModel->setEnabled(value != 0);
}

djv_ocio_config djv_ocio_get_config(djv_models* models)
{
    if (!models)
        return DJV_OCIO_CONFIG_BUILT_IN;
    switch (models->ocioData.config)
    {
    case tl::OCIOConfig::EnvVar: return DJV_OCIO_CONFIG_ENV;
    case tl::OCIOConfig::File: return DJV_OCIO_CONFIG_FILE;
    default: break;
    }
    return DJV_OCIO_CONFIG_BUILT_IN;
}

void djv_ocio_set_config(djv_models* models, djv_ocio_config value)
{
    if (!models)
        return;
    tl::OCIOConfig config = tl::OCIOConfig::BuiltIn;
    switch (value)
    {
    case DJV_OCIO_CONFIG_ENV: config = tl::OCIOConfig::EnvVar; break;
    case DJV_OCIO_CONFIG_FILE: config = tl::OCIOConfig::File; break;
    default: break;
    }
    models->ocioModel->setConfig(config);
}

const char* djv_ocio_get_file_name(djv_models* models)
{
    return models ? models->ocioData.fileName.c_str() : "";
}

void djv_ocio_set_file_name(djv_models* models, const char* value)
{
    if (models && value)
        models->ocioModel->setFileName(value);
}

const char* djv_ocio_get_name(djv_models* models)
{
    return models ? models->ocioData.name.c_str() : "";
}

namespace
{
    const char* item(const std::vector<std::string>& list, size_t i)
    {
        return i < list.size() ? list[i].c_str() : "";
    }
}

size_t djv_ocio_get_input_count(djv_models* models)
{
    return models ? models->ocioData.inputs.size() : 0;
}

const char* djv_ocio_get_input(djv_models* models, size_t i)
{
    return models ? item(models->ocioData.inputs, i) : "";
}

size_t djv_ocio_get_input_index(djv_models* models)
{
    return models ? models->ocioData.inputIndex : 0;
}

void djv_ocio_set_input_index(djv_models* models, size_t value)
{
    if (models)
        models->ocioModel->setInputIndex(value);
}

size_t djv_ocio_get_display_count(djv_models* models)
{
    return models ? models->ocioData.displays.size() : 0;
}

const char* djv_ocio_get_display(djv_models* models, size_t i)
{
    return models ? item(models->ocioData.displays, i) : "";
}

size_t djv_ocio_get_display_index(djv_models* models)
{
    return models ? models->ocioData.displayIndex : 0;
}

void djv_ocio_set_display_index(djv_models* models, size_t value)
{
    if (models)
        models->ocioModel->setDisplayIndex(value);
}

size_t djv_ocio_get_view_count(djv_models* models)
{
    return models ? models->ocioData.views.size() : 0;
}

const char* djv_ocio_get_view(djv_models* models, size_t i)
{
    return models ? item(models->ocioData.views, i) : "";
}

size_t djv_ocio_get_view_index(djv_models* models)
{
    return models ? models->ocioData.viewIndex : 0;
}

void djv_ocio_set_view_index(djv_models* models, size_t value)
{
    if (models)
        models->ocioModel->setViewIndex(value);
}

size_t djv_ocio_get_look_count(djv_models* models)
{
    return models ? models->ocioData.looks.size() : 0;
}

const char* djv_ocio_get_look(djv_models* models, size_t i)
{
    return models ? item(models->ocioData.looks, i) : "";
}

size_t djv_ocio_get_look_index(djv_models* models)
{
    return models ? models->ocioData.lookIndex : 0;
}

void djv_ocio_set_look_index(djv_models* models, size_t value)
{
    if (models)
        models->ocioModel->setLookIndex(value);
}

const char* djv_color_get_resolved_input(djv_models* models)
{
    return models ? models->resolvedInput.c_str() : "";
}

// LUT.

int djv_lut_is_enabled(djv_models* models)
{
    return models ? (models->colorModel->getLUTOptions().enabled ? 1 : 0) : 0;
}

void djv_lut_set_enabled(djv_models* models, int value)
{
    if (!models)
        return;
    auto options = models->colorModel->getLUTOptions();
    options.enabled = value != 0;
    models->colorModel->setLUTOptions(options);
}

const char* djv_lut_get_file_name(djv_models* models)
{
    return models ? models->colorModel->getLUTOptions().fileName.c_str() : "";
}

void djv_lut_set_file_name(djv_models* models, const char* value)
{
    if (!models || !value)
        return;
    auto options = models->colorModel->getLUTOptions();
    options.fileName = value;
    models->colorModel->setLUTOptions(options);
}

djv_lut_order djv_lut_get_order(djv_models* models)
{
    return models && tl::LUTOrder::PreConfig == models->colorModel->getLUTOptions().order ?
        DJV_LUT_ORDER_PRE_CONFIG :
        DJV_LUT_ORDER_POST_CONFIG;
}

void djv_lut_set_order(djv_models* models, djv_lut_order value)
{
    if (!models)
        return;
    auto options = models->colorModel->getLUTOptions();
    options.order = DJV_LUT_ORDER_PRE_CONFIG == value ?
        tl::LUTOrder::PreConfig :
        tl::LUTOrder::PostConfig;
    models->colorModel->setLUTOptions(options);
}

djv_lut_direction djv_lut_get_direction(djv_models* models)
{
    return models && tl::LUTDirection::Inverse == models->colorModel->getLUTOptions().direction ?
        DJV_LUT_DIRECTION_INVERSE :
        DJV_LUT_DIRECTION_FORWARD;
}

size_t djv_media_reference_get_count(djv_models* models)
{
    if (!models)
        return 0;
    models->mediaReferenceKeys.clear();
    if (auto player = tl::c::getPlayer(models->player))
    {
        const auto keys = player->getMediaReferenceKeys();
        const bool versions = std::any_of(
            keys.begin(),
            keys.end(),
            [](const std::string& key)
            {
                return !key.empty() && key != OTIO_NS::Clip::default_media_key;
            });
        if (versions)
        {
            models->mediaReferenceKeys.push_back(std::string());
            for (const auto& key : keys)
            {
                if (!key.empty())
                {
                    models->mediaReferenceKeys.push_back(key);
                }
            }
        }
    }
    return models->mediaReferenceKeys.size();
}

const char* djv_media_reference_get_key(djv_models* models, size_t index)
{
    return models && index < models->mediaReferenceKeys.size() ?
        models->mediaReferenceKeys[index].c_str() :
        "";
}

const char* djv_media_reference_get_label(djv_models* models, const char* key)
{
    if (!models)
        return "";
    models->mediaReferenceLabel = models::getMediaReferenceLabel(key ? key : "");
    return models->mediaReferenceLabel.c_str();
}

const char* djv_media_reference_get_current(djv_models* models)
{
    if (!models)
        return "";
    auto player = tl::c::getPlayer(models->player);
    models->mediaReferenceKey = player ? player->getMediaReferenceKey() : std::string();
    return models->mediaReferenceKey.c_str();
}

void djv_media_reference_set_current(djv_models* models, const char* key)
{
    if (!models || !key)
        return;
    auto player = tl::c::getPlayer(models->player);
    if (!player || player->getMediaReferenceKey() == key)
        return;
    player->setMediaReferenceKey(key);
    // Kept with the review, for the file shown, as the desktop keeps it.
    for (auto& file : models->review.files)
    {
        if (file.id == models->fileId)
        {
            file.mediaReferenceKey = key;
            models->mediaReferenceChanged = true;
            break;
        }
    }
    ++models->generation;
}

const char* djv_lut_get_extensions(void)
{
    static const std::string out = []
    {
        std::vector<std::string> exts;
        for (std::string ext : tl::getLUTFormatExts())
        {
            if (!ext.empty() && '.' == ext.front())
            {
                ext.erase(ext.begin());
            }
            for (auto& c : ext)
            {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            if (!ext.empty() && std::find(exts.begin(), exts.end(), ext) == exts.end())
            {
                exts.push_back(ext);
            }
        }
        std::string s;
        for (const auto& ext : exts)
        {
            s += (s.empty() ? "" : " ") + ext;
        }
        return s;
    }();
    return out.c_str();
}

void djv_lut_set_direction(djv_models* models, djv_lut_direction value)
{
    if (!models)
        return;
    auto options = models->colorModel->getLUTOptions();
    options.direction = DJV_LUT_DIRECTION_INVERSE == value ?
        tl::LUTDirection::Inverse :
        tl::LUTDirection::Forward;
    models->colorModel->setLUTOptions(options);
}

// Audio.

float djv_audio_get_volume(djv_models* models)
{
    return models ? models->audioModel->getVolume() : 0.F;
}

void djv_audio_set_volume(djv_models* models, float value)
{
    if (models)
        models->audioModel->setVolume(value);
}

void djv_audio_volume_up(djv_models* models)
{
    if (models)
    {
        models->audioModel->volumeUp();
    }
}

void djv_audio_volume_down(djv_models* models)
{
    if (models)
    {
        models->audioModel->volumeDown();
    }
}

int djv_audio_is_muted(djv_models* models)
{
    return models ? (models->audioModel->isMuted() ? 1 : 0) : 0;
}

void djv_audio_set_muted(djv_models* models, int value)
{
    if (models)
        models->audioModel->setMute(value != 0);
}

double djv_audio_get_sync_offset(djv_models* models)
{
    return models ? models->audioModel->getSyncOffset() : 0.0;
}

void djv_audio_set_sync_offset(djv_models* models, double value)
{
    if (models)
        models->audioModel->setSyncOffset(value);
}

// Viewport.

namespace
{
    void toC(const ftk::V3F& v, float* out)
    {
        out[0] = v.x;
        out[1] = v.y;
        out[2] = v.z;
    }

    ftk::V3F fromC3(const float* v)
    {
        return ftk::V3F(v[0], v[1], v[2]);
    }

    void toC(const ftk::Color4F& c, float* out)
    {
        out[0] = c.r;
        out[1] = c.g;
        out[2] = c.b;
        out[3] = c.a;
    }

    ftk::Color4F fromC4(const float* c)
    {
        return ftk::Color4F(c[0], c[1], c[2], c[3]);
    }
}

djv_display_options djv_viewport_get_display(djv_models* models)
{
    djv_display_options out = {};
    if (!models)
        return out;
    const tl::DisplayOptions& o = models->viewportModel->getDisplayOptions();
    out.channels = static_cast<int>(o.channels);
    out.mirrorX = o.mirror.x ? 1 : 0;
    out.mirrorY = o.mirror.y ? 1 : 0;
    out.negative = o.negative ? 1 : 0;
    out.colorEnabled = o.color.enabled ? 1 : 0;
    toC(o.color.add, out.add);
    toC(o.color.brightness, out.brightness);
    toC(o.color.contrast, out.contrast);
    toC(o.color.saturation, out.saturation);
    out.hue = o.color.hue;
    out.levelsEnabled = o.levels.enabled ? 1 : 0;
    out.levelsInLow = o.levels.inLow;
    out.levelsInHigh = o.levels.inHigh;
    out.levelsGamma = o.levels.gamma;
    out.levelsOutLow = o.levels.outLow;
    out.levelsOutHigh = o.levels.outHigh;
    out.exposureEnabled = o.exposure.enabled ? 1 : 0;
    out.exposure = o.exposure.exposure;
    out.softClipEnabled = o.softClip.enabled ? 1 : 0;
    out.softClip = o.softClip.value;
    return out;
}

void djv_viewport_set_display(djv_models* models, djv_display_options value)
{
    if (!models)
        return;
    tl::DisplayOptions o = models->viewportModel->getDisplayOptions();
    o.channels = static_cast<ftk::ChannelDisplay>(value.channels);
    o.mirror.x = value.mirrorX != 0;
    o.mirror.y = value.mirrorY != 0;
    o.negative = value.negative != 0;
    o.color.enabled = value.colorEnabled != 0;
    o.color.add = fromC3(value.add);
    o.color.brightness = fromC3(value.brightness);
    o.color.contrast = fromC3(value.contrast);
    o.color.saturation = fromC3(value.saturation);
    o.color.hue = value.hue;
    o.levels.enabled = value.levelsEnabled != 0;
    o.levels.inLow = value.levelsInLow;
    o.levels.inHigh = value.levelsInHigh;
    o.levels.gamma = value.levelsGamma;
    o.levels.outLow = value.levelsOutLow;
    o.levels.outHigh = value.levelsOutHigh;
    o.exposure.enabled = value.exposureEnabled != 0;
    o.exposure.exposure = value.exposure;
    o.softClip.enabled = value.softClipEnabled != 0;
    o.softClip.value = value.softClip;
    models->viewportModel->setDisplayOptions(o);
}

int djv_viewport_has_color_adjustments(djv_models* models)
{
    return models && models->viewportModel->hasColorAdjustments() ? 1 : 0;
}

void djv_viewport_reset_color_adjustments(djv_models* models, int color, int levels)
{
    if (!models)
        return;
    models->viewportModel->resetColorAdjustments(color != 0, levels != 0);
}

djv_image_options djv_viewport_get_image(djv_models* models)
{
    djv_image_options out = {};
    if (!models)
        return out;
    const ftk::ImageOptions& o = models->viewportModel->getImageOptions();
    out.videoLevels = static_cast<int>(o.videoLevels);
    out.alphaBlend = static_cast<int>(o.alphaBlend);
    out.minify = static_cast<int>(o.imageFilters.minify);
    out.magnify = static_cast<int>(o.imageFilters.magnify);
    return out;
}

void djv_viewport_set_image(djv_models* models, djv_image_options value)
{
    if (!models)
        return;
    ftk::ImageOptions o = models->viewportModel->getImageOptions();
    o.videoLevels = static_cast<ftk::InputVideoLevels>(value.videoLevels);
    o.alphaBlend = static_cast<ftk::AlphaBlend>(value.alphaBlend);
    o.imageFilters.minify = static_cast<ftk::ImageFilter>(value.minify);
    o.imageFilters.magnify = static_cast<ftk::ImageFilter>(value.magnify);
    models->viewportModel->setImageOptions(o);
}

djv_background_options djv_viewport_get_background(djv_models* models)
{
    djv_background_options out = {};
    if (!models)
        return out;
    const tl::BackgroundOptions& o = models->viewportModel->getBackgroundOptions();
    out.type = static_cast<int>(o.type);
    toC(o.solidColor, out.solid);
    toC(o.checkersColor.first, out.checkers0);
    toC(o.checkersColor.second, out.checkers1);
    out.checkersSize = o.checkersSize.w;
    toC(o.gradientColor.first, out.gradient0);
    toC(o.gradientColor.second, out.gradient1);
    out.outlineEnabled = o.outline.enabled ? 1 : 0;
    out.outlineWidth = o.outline.width;
    toC(o.outline.color, out.outlineColor);
    return out;
}

void djv_viewport_set_background(djv_models* models, djv_background_options value)
{
    if (!models)
        return;
    tl::BackgroundOptions o = models->viewportModel->getBackgroundOptions();
    o.type = static_cast<tl::Background>(value.type);
    o.solidColor = fromC4(value.solid);
    o.checkersColor.first = fromC4(value.checkers0);
    o.checkersColor.second = fromC4(value.checkers1);
    o.checkersSize = ftk::Size2I(value.checkersSize, value.checkersSize);
    o.gradientColor.first = fromC4(value.gradient0);
    o.gradientColor.second = fromC4(value.gradient1);
    o.outline.enabled = value.outlineEnabled != 0;
    o.outline.width = value.outlineWidth;
    o.outline.color = fromC4(value.outlineColor);
    models->viewportModel->setBackgroundOptions(o);
}

djv_foreground_options djv_viewport_get_foreground(djv_models* models)
{
    djv_foreground_options out = {};
    if (!models)
        return out;
    const tl::ForegroundOptions& o = models->viewportModel->getForegroundOptions();
    out.gridEnabled = o.grid.enabled ? 1 : 0;
    out.gridCellMode = static_cast<int>(o.grid.cellMode);
    out.gridCellSize = o.grid.cellSize;
    out.gridCellCountX = o.grid.cellCount.x;
    out.gridCellCountY = o.grid.cellCount.y;
    out.gridLineWidth = o.grid.lineWidth;
    toC(o.grid.color, out.gridColor);
    out.gridLabels = static_cast<int>(o.grid.labels);
    out.centerMarkerEnabled = o.centerMarker.enabled ? 1 : 0;
    out.centerMarkerSize = o.centerMarker.size;
    out.centerMarkerWidth = o.centerMarker.width;
    toC(o.centerMarker.color, out.centerMarkerColor);
    out.missingEnabled = o.missingIndicator.enabled ? 1 : 0;
    out.missingWidth = o.missingIndicator.width;
    toC(o.missingIndicator.color, out.missingColor);
    out.clippingEnabled = o.clippingWarning.enabled ? 1 : 0;
    out.clippingMode = static_cast<int>(o.clippingWarning.mode);
    out.clippingLow = o.clippingWarning.low;
    out.clippingHigh = o.clippingWarning.high;
    out.frameGuideEnabled = o.frameGuide.enabled ? 1 : 0;
    out.frameGuideAspect = o.frameGuide.aspect;
    out.frameGuideMaskOpacity = o.frameGuide.maskOpacity;
    out.safeAreasEnabled = o.safeAreas.enabled ? 1 : 0;
    out.safeAreasAction = o.safeAreas.action;
    out.safeAreasTitle = o.safeAreas.title;
    return out;
}

void djv_viewport_set_foreground(djv_models* models, djv_foreground_options value)
{
    if (!models)
        return;
    tl::ForegroundOptions o = models->viewportModel->getForegroundOptions();
    o.grid.enabled = value.gridEnabled != 0;
    o.grid.cellMode = static_cast<tl::GridCellMode>(value.gridCellMode);
    o.grid.cellSize = value.gridCellSize;
    o.grid.cellCount = ftk::V2I(value.gridCellCountX, value.gridCellCountY);
    o.grid.lineWidth = value.gridLineWidth;
    o.grid.color = fromC4(value.gridColor);
    o.grid.labels = static_cast<tl::GridLabels>(value.gridLabels);
    o.centerMarker.enabled = value.centerMarkerEnabled != 0;
    o.centerMarker.size = value.centerMarkerSize;
    o.centerMarker.width = value.centerMarkerWidth;
    o.centerMarker.color = fromC4(value.centerMarkerColor);
    o.missingIndicator.enabled = value.missingEnabled != 0;
    o.missingIndicator.width = value.missingWidth;
    o.missingIndicator.color = fromC4(value.missingColor);
    o.clippingWarning.enabled = value.clippingEnabled != 0;
    o.clippingWarning.mode = static_cast<tl::ClippingWarningMode>(value.clippingMode);
    o.clippingWarning.low = value.clippingLow;
    o.clippingWarning.high = value.clippingHigh;
    o.frameGuide.enabled = value.frameGuideEnabled != 0;
    o.frameGuide.aspect = value.frameGuideAspect;
    o.frameGuide.maskOpacity = value.frameGuideMaskOpacity;
    o.safeAreas.enabled = value.safeAreasEnabled != 0;
    o.safeAreas.action = value.safeAreasAction;
    o.safeAreas.title = value.safeAreasTitle;
    models->viewportModel->setForegroundOptions(o);
}

namespace
{
    void aspectRatios(djv_models* models)
    {
        models->aspectRatios.clear();
        for (const auto& i : models->viewportModel->getAspectRatioOptions().options)
        {
            models->aspectRatios.push_back(tl::getLabel(i));
        }
    }
}

size_t djv_viewport_get_aspect_ratio_count(djv_models* models)
{
    if (!models)
        return 0;
    aspectRatios(models);
    return models->aspectRatios.size();
}

const char* djv_viewport_get_aspect_ratio(djv_models* models, size_t i)
{
    if (!models)
        return "";
    if (models->aspectRatios.empty())
    {
        aspectRatios(models);
    }
    return item(models->aspectRatios, i);
}

size_t djv_viewport_get_aspect_ratio_index(djv_models* models)
{
    return models ? static_cast<size_t>(std::max(models->viewportModel->getAspectRatioOptions().index, 0)) : 0;
}

void djv_viewport_set_aspect_ratio_index(djv_models* models, size_t value)
{
    if (!models)
        return;
    auto options = models->viewportModel->getAspectRatioOptions();
    options.index = static_cast<int>(value);
    models->viewportModel->setAspectRatioOptions(options);
}

// Information.

size_t djv_info_get_count(djv_models* models, const char* search)
{
    if (!models)
        return 0;
    models->info.clear();
    if (auto player = tl::c::getPlayer(models->player))
    {
        models->info = models::filterInfo(
            models::getInfo(player->getPath(), player->getIOInfo(), player->getVideoLayer()),
            search ? search : "");
    }
    return models->info.size();
}

const char* djv_info_get_section(djv_models* models, size_t i)
{
    return models && i < models->info.size() ? models->info[i].section.c_str() : "";
}

const char* djv_info_get_name(djv_models* models, size_t i)
{
    return models && i < models->info.size() ? models->info[i].name.c_str() : "";
}

const char* djv_info_get_value(djv_models* models, size_t i)
{
    return models && i < models->info.size() ? models->info[i].value.c_str() : "";
}

// Cache.

float djv_cache_get_video_gb(djv_models* models)
{
    return models ? models->settingsModel->getCache().videoGB : 0.F;
}

void djv_cache_set_video_gb(djv_models* models, float value)
{
    if (!models)
        return;
    auto options = models->settingsModel->getCache();
    options.videoGB = std::min(std::max(value, 0.F), models::SettingsModel::getMaxCacheGB());
    models->settingsModel->setCache(options);
}

float djv_cache_get_default_video_gb(void)
{
    return models::SettingsModel::getDefaultCache().videoGB;
}

float djv_cache_get_max_gb(void)
{
    return models::SettingsModel::getMaxCacheGB();
}

// HUD.

size_t djv_hud_get_item_count(void)
{
    return models::getHUDItemOrder().size();
}

djv_hud_item djv_hud_get_item(size_t i)
{
    const auto order = models::getHUDItemOrder();
    return i < order.size() ? static_cast<djv_hud_item>(order[i]) : DJV_HUD_COUNT;
}

const char* djv_hud_get_item_name(djv_hud_item item)
{
    static std::vector<std::string> names;
    if (names.empty())
    {
        for (const auto i : models::getHUDItemEnums())
        {
            names.push_back(models::getLabel(i));
        }
    }
    const size_t i = static_cast<size_t>(item);
    return i < names.size() ? names[i].c_str() : "";
}

int djv_hud_is_enabled(djv_models* models)
{
    return models && models->viewportModel->getHUDOptions().enabled ? 1 : 0;
}

void djv_hud_set_enabled(djv_models* models, int value)
{
    if (!models)
        return;
    auto options = models->viewportModel->getHUDOptions();
    options.enabled = value != 0;
    models->viewportModel->setHUDOptions(options);
}

djv_hud_pos djv_hud_get_pos(djv_models* models, djv_hud_item item)
{
    if (!models)
        return DJV_HUD_POS_NONE;
    const auto& items = models->viewportModel->getHUDOptions().items;
    const auto i = items.find(static_cast<models::HUDItem>(item));
    return i != items.end() ? static_cast<djv_hud_pos>(i->second) : DJV_HUD_POS_NONE;
}

void djv_hud_set_pos(djv_models* models, djv_hud_item item, djv_hud_pos pos)
{
    if (!models ||
        static_cast<int>(item) < 0 || static_cast<int>(item) >= DJV_HUD_COUNT ||
        static_cast<int>(pos) < 0 || static_cast<int>(pos) > DJV_HUD_POS_BOTTOM_RIGHT)
        return;
    auto options = models->viewportModel->getHUDOptions();
    options.items[static_cast<models::HUDItem>(item)] = static_cast<models::HUDPos>(pos);
    models->viewportModel->setHUDOptions(options);
}

const char* djv_hud_get_text(
    djv_models* models,
    djv_hud_item item,
    double fps,
    size_t droppedFrames,
    double viewZoom)
{
    if (!models)
        return "";
    models::HUDData data;
    if (auto player = tl::c::getPlayer(models->player))
    {
        data.path = player->getPath();
        data.ioInfo = player->getIOInfo();
        data.videoLayer = player->getVideoLayer();
        data.currentTime = player->getCurrentTime();
        data.cacheInfo = player->observeCacheInfo()->get();
    }
    data.timeLabel = models->timeUnitsModel->getLabel(data.currentTime);
    data.fps = fps;
    data.droppedFrames = droppedFrames;
    data.aspectRatio = models->viewportModel->getDisplayOptions().aspectRatio;
    data.lutOptions = models->colorModel->getLUTOptions();
    data.viewZoom = viewZoom;
    models->hudText = models::getHUDText(static_cast<models::HUDItem>(item), data);
    return models->hudText.c_str();
}

// Time units.

djv_time_units djv_time_get_units(djv_models* models)
{
    if (!models)
        return DJV_TIME_UNITS_TIMECODE;
    switch (models->timeUnitsModel->getTimeUnits())
    {
    case tl::TimeUnits::Frames: return DJV_TIME_UNITS_FRAMES;
    case tl::TimeUnits::Seconds: return DJV_TIME_UNITS_SECONDS;
    default: return DJV_TIME_UNITS_TIMECODE;
    }
}

void djv_time_set_units(djv_models* models, djv_time_units value)
{
    if (!models)
        return;
    tl::TimeUnits units = tl::TimeUnits::Timecode;
    switch (value)
    {
    case DJV_TIME_UNITS_FRAMES: units = tl::TimeUnits::Frames; break;
    case DJV_TIME_UNITS_SECONDS: units = tl::TimeUnits::Seconds; break;
    default: break;
    }
    models->timeUnitsModel->setTimeUnits(units);
    ++models->generation;
}

const char* djv_time_get_label(djv_models* models, double seconds)
{
    if (!models)
        return "";
    models->timeLabel.clear();
    if (auto player = tl::c::getPlayer(models->player))
    {
        const OTIO_NS::RationalTime& start = player->getTimeRange().start_time();
        models->timeLabel = models->timeUnitsModel->getLabel(
            start + OTIO_NS::RationalTime(seconds, 1.0).rescaled_to(start.rate()).round());
    }
    return models->timeLabel.c_str();
}

// Comparing.

static_assert(DJV_COMPARE_COUNT == static_cast<int>(tl::Compare::Count));

const char* djv_compare_get_label(djv_compare value)
{
    static const std::vector<std::string> labels = tl::getCompareLabels();
    const size_t i = static_cast<size_t>(value);
    return i < labels.size() ? labels[i].c_str() : "";
}

int djv_compare_set_file(djv_models* models, const char* path)
{
    if (!models || !models->player)
        return 0;
    const bool file = path && path[0];
    if (!tl_player_set_compare(models->player, path))
        return 0;
    models->compareFile = file;
    models->compareFilePath = file ? path : "";
    models->compareChanged = true;
    if (!file)
    {
        // Nothing to compare with, as the files model does with one file.
        auto options = models->filesModel->getCompareOptions();
        options.compare = tl::Compare::None;
        models->filesModel->setCompareOptions(options);
    }
    models->activeFilesUpdate();
    models->applyToPlayer();
    models->needsDraw();
    ++models->generation;
    return 1;
}

int djv_compare_has_file(djv_models* models)
{
    return models && models->compareFile ? 1 : 0;
}

djv_compare djv_compare_get_mode(djv_models* models)
{
    return models ?
        static_cast<djv_compare>(models->filesModel->getCompareOptions().compare) :
        DJV_COMPARE_NONE;
}

void djv_compare_set_mode(djv_models* models, djv_compare value)
{
    if (!models || value < DJV_COMPARE_NONE || value >= DJV_COMPARE_COUNT)
        return;
    auto options = models->filesModel->getCompareOptions();
    options.compare = models->compareFile ?
        static_cast<tl::Compare>(value) :
        tl::Compare::None;
    models->filesModel->setCompareOptions(options);
}

void djv_compare_get_wipe(djv_models* models, float* x, float* y, float* rotation)
{
    if (!models)
        return;
    const auto& options = models->filesModel->getCompareOptions();
    if (x) *x = options.wipeCenter.x;
    if (y) *y = options.wipeCenter.y;
    if (rotation) *rotation = options.wipeRotation;
}

void djv_compare_set_wipe_rotation(djv_models* models, float value)
{
    if (!models)
        return;
    auto options = models->filesModel->getCompareOptions();
    options.wipeRotation = value;
    models->filesModel->setCompareOptions(options);
}

int djv_compare_wipe_begin(djv_models* models, float x, float y)
{
    if (!models)
        return 0;
    const auto widget = models->wipeWidget();
    if (!widget.has_value())
        return 0;
    const ftk::V2F pos(x, y);
    const auto picture = tl::c::getViewPicture(models->view);
    models->wipeDrag = models::getWipeHandle(
        *widget, pos, ui::wipeTouchReach * picture.displayScale);
    if (models::WipeHandle::None == models->wipeDrag)
        return 0;
    models->wipeGrab = models::WipeHandle::Center == models->wipeDrag ?
        ftk::V2F(pos.x - widget->center.x, pos.y - widget->center.y) :
        ftk::V2F();
    djv_compare_wipe_move(models, x, y);
    models->needsDraw();
    return 1;
}

int djv_compare_wipe_hit(djv_models* models, float x, float y)
{
    if (!models)
        return 0;
    const auto widget = models->wipeWidget();
    if (!widget.has_value())
        return 0;
    const auto picture = tl::c::getViewPicture(models->view);
    return models::getWipeHandle(
        *widget, ftk::V2F(x, y), ui::wipeTouchReach * picture.displayScale) !=
        models::WipeHandle::None ? 1 : 0;
}

void djv_compare_wipe_move(djv_models* models, float x, float y)
{
    if (!models || models::WipeHandle::None == models->wipeDrag)
        return;
    const auto widget = models->wipeWidget();
    if (!widget.has_value())
        return;
    const auto picture = tl::c::getViewPicture(models->view);
    auto options = models->filesModel->getCompareOptions();
    if (models::WipeHandle::Rotate == models->wipeDrag)
    {
        options.wipeRotation = models::getWipeRotation(widget->center, ftk::V2F(x, y));
    }
    else
    {
        options.wipeCenter = models::getWipeCenter(
            ftk::V2F(x - models->wipeGrab.x, y - models->wipeGrab.y),
            picture.box,
            picture.pos,
            picture.zoom);
    }
    models->filesModel->setCompareOptions(options);
}

void djv_compare_wipe_end(djv_models* models)
{
    if (!models)
        return;
    models->wipeDrag = models::WipeHandle::None;
    models->needsDraw();
}

float djv_compare_get_overlay(djv_models* models)
{
    return models ? models->filesModel->getCompareOptions().overlay : 0.F;
}

void djv_compare_set_overlay(djv_models* models, float value)
{
    if (!models)
        return;
    auto options = models->filesModel->getCompareOptions();
    options.overlay = std::clamp(value, 0.F, 1.F);
    models->filesModel->setCompareOptions(options);
}

float djv_compare_get_difference_gain(djv_models* models)
{
    return models ? models->filesModel->getCompareOptions().differenceGain : 1.F;
}

void djv_compare_set_difference_gain(djv_models* models, float value)
{
    if (!models)
        return;
    auto options = models->filesModel->getCompareOptions();
    options.differenceGain = std::max(value, 0.F);
    models->filesModel->setCompareOptions(options);
}

int djv_compare_is_absolute_time(djv_models* models)
{
    return models && models->filesModel->getCompareTime() == tl::CompareTime::Absolute ? 1 : 0;
}

void djv_compare_set_absolute_time(djv_models* models, int value)
{
    if (!models)
        return;
    models->filesModel->setCompareTime(
        value ? tl::CompareTime::Absolute : tl::CompareTime::Relative);
}

const char* djv_compare_get_time_label(int absolute)
{
    static const std::vector<std::string> labels = models::getCompareTimeLabels();
    const size_t i = absolute ? 1 : 0;
    return i < labels.size() ? labels[i].c_str() : "";
}

// Drawing.

djv_draw_tool djv_draw_get_tool(djv_models* models)
{
    return models ? static_cast<djv_draw_tool>(models->drawModel->getTool()) : DJV_DRAW_PEN;
}

void djv_draw_set_tool(djv_models* models, djv_draw_tool value)
{
    if (models && static_cast<int>(value) >= 0 && static_cast<int>(value) <= DJV_DRAW_SELECT)
    {
        models->drawModel->setTool(static_cast<models::DrawTool>(value));
    }
}

void djv_draw_get_color(djv_models* models, float color[4])
{
    if (!models || !color)
        return;
    const ftk::Color4F& c = models->drawModel->getColor();
    color[0] = c.r;
    color[1] = c.g;
    color[2] = c.b;
    color[3] = c.a;
}

void djv_draw_set_color(djv_models* models, const float color[4])
{
    if (models && color)
    {
        models->drawModel->setColor(ftk::Color4F(color[0], color[1], color[2], color[3]));
    }
}

float djv_draw_get_size(djv_models* models)
{
    return models ? models->drawModel->getSize() : 0.F;
}

void djv_draw_set_size(djv_models* models, float value)
{
    if (models)
    {
        models->drawModel->setSize(value);
    }
}

int djv_draw_is_constrain(djv_models* models)
{
    return models && models->drawModel->isConstrain() ? 1 : 0;
}

void djv_draw_set_constrain(djv_models* models, int value)
{
    if (models)
    {
        models->drawModel->setConstrain(value != 0);
        ++models->generation;
    }
}

int djv_review_open(djv_models* models, const char* reviewPath, const char* mediaPath)
{
    if (!models || !reviewPath || !mediaPath)
        return 0;
    djv_review_close(models);
    models->reviewPath = reviewPath;
    const std::filesystem::path review = std::filesystem::u8path(reviewPath);
    const std::filesystem::path media = std::filesystem::u8path(mediaPath);
    try
    {
        if (std::filesystem::exists(review))
        {
            models->review = models::reviewOpen(reviewPath);
        }
    }
    catch (const std::exception& e)
    {
        models->reviewError = e.what();
        models->reviewPath.clear();
        return 0;
    }
    // The file in the review the media is.
    models->fileId = models->reviewFileId(media);
    if (models->fileId.empty())
    {
        models->fileId = models->reviewFileAdd(mediaPath);
        if (models->review.compare.aId.empty())
        {
            models->review.compare.aId = models->fileId;
        }
    }
    models->annotationsModel->setAnnotations(models->review.annotations);
    models->markersModel->setMarkers(models->review.markers);
    models->applyReviewMediaReference();
    models->mediaReferenceChanged = false;

    // The file the review compares with, where it is found from the
    // review; and the comparison is the review's until it changes.
    models->reviewCompareFile.clear();
    const auto& compare = models->review.compare;
    if (!compare.bIds.empty())
    {
        for (const auto& file : models->review.files)
        {
            if (file.id == compare.bIds.front() && file.id != models->fileId)
            {
                bool exists = false;
                const auto path = models::resolveReviewPath(
                    file.path,
                    file.pathAbsolute,
                    review.parent_path(),
                    std::filesystem::path(),
                    ftk::PathOptions(),
                    exists);
                if (exists)
                {
                    models->reviewCompareFile = ftk::fromFileSystem(path);
                }
                break;
            }
        }
    }
    models->compareChanged = false;
    return 1;
}

int djv_review_save(djv_models* models)
{
    if (!models || models->reviewPath.empty())
        return 0;
    const auto& annotations = models->annotationsModel->getAnnotations();
    const auto& markers = models->markersModel->getMarkers();
    const std::filesystem::path path = std::filesystem::u8path(models->reviewPath);
    const bool comparing = models->compareChanged && models->compareFile;
    if (annotations.empty() &&
        markers.empty() &&
        !comparing &&
        !models->mediaReferenceChanged &&
        !std::filesystem::exists(path))
        return 1;
    try
    {
        models->review.annotations = annotations;
        models->review.markers = markers;
        // The comparison only once it changed here: a review's the iPad
        // could not show, its "B" not found, stays as it was.
        if (models->compareChanged)
        {
            models->reviewCompareUpdate();
        }
        if (models->review.app.empty())
        {
            models->review.app = "DJV Studio for iPad";
        }
        if (models->review.created.empty())
        {
            models->review.created = models::timestamp();
        }
        models::reviewSave(models->reviewPath, models->review);
    }
    catch (const std::exception& e)
    {
        models->reviewError = e.what();
        return 0;
    }
    return 1;
}

void djv_review_close(djv_models* models)
{
    if (!models)
        return;
    djv_draw_cancel(models);
    models->review = models::Review();
    models->reviewPath.clear();
    models->fileId.clear();
    models->reviewCompareFile.clear();
    models->reviewError.clear();
    models->mediaReferenceChanged = false;
    models->annotationsModel->clear();
    models->markersModel->clear();
}

const char* djv_review_get_error(djv_models* models)
{
    return models ? models->reviewError.c_str() : "";
}

int djv_draw_begin(djv_models* models, float x, float y, int pen, float pressure)
{
    if (!models || models->fileId.empty() || !models->isAShown())
        return 0;
    djv_draw_cancel(models);
    const models::DrawTool tool = models->drawModel->getTool();
    if (models::DrawTool::Select == tool)
    {
        // A handle can be at the edge of the picture, and a press off it
        // still takes the selection away.
        return models->selectPress(x, y, pen != 0) ? 1 : 0;
    }
    const auto time = models->currentTime();
    const models::ImageMapping m = models->mapping();
    const ftk::V2F render = models->toRender(x, y);
    if (!time.has_value() || !m.contains(render))
        return 0;
    switch (tool)
    {
    case models::DrawTool::Eraser:
        models->erasing = true;
        models->erasingPen = pen != 0;
        models->eraseSteps = 0;
        models->erase(render);
        return 1;
    case models::DrawTool::Text:
        // Begun where it was tapped, and open for typing until
        // djv_text_end().
        models->stroke = models::ReviewStroke();
        models->stroke.kind = models::ReviewStrokeKind::Text;
        models->stroke.color = models->drawModel->getColor();
        models->stroke.width = models->drawModel->getSize();
        models->stroke.textSize = models->drawModel->getTextSize();
        models->stroke.points.push_back(m.toImage(render));
        models->strokeTime = *time;
        models->typing = true;
        models->needsDraw();
        ++models->generation;
        return 1;
    default: break;
    }
    models->stroke = models::ReviewStroke();
    models->stroke.kind = models::getStrokeKind(tool);
    models->stroke.color = models->drawModel->getColor();
    models->stroke.width = models->drawModel->getSize();
    models::strokeBegin(models->stroke, m.toImage(render), pen != 0, pressure);
    models->strokeTime = *time;
    models->drawing = true;
    models->needsDraw();
    return 1;
}

void djv_draw_move(djv_models* models, float x, float y, float pressure)
{
    if (!models)
        return;
    const ftk::V2F render = models->toRender(x, y);
    if (models->erasing)
    {
        models->erase(render);
        return;
    }
    if (models->moving.has_value())
    {
        models->selectMove(x, y);
        return;
    }
    if (!models->drawing)
        return;
    const models::ImageMapping m = models->mapping();
    if (!m.isValid())
        return;
    if (models::isTwoPointShape(models->stroke.kind))
    {
        // A shape goes on at the edge when the finger leaves the picture.
        models::strokeContinue(
            models->stroke,
            m.clamp(m.toImage(render)),
            pressure,
            models->drawModel->isConstrain());
    }
    else if (m.contains(render))
    {
        models::strokeContinue(models->stroke, m.toImage(render), pressure, false);
    }
    models->needsDraw();
}

void djv_draw_end(djv_models* models)
{
    if (!models)
        return;
    // Text stays open for typing.
    if (models->typing)
        return;
    if (models->moving.has_value())
    {
        // One undo step for the move; a press that did not move changes
        // nothing.
        if (models->selected().has_value())
        {
            models->annotationsModel->setStroke(
                models->fileId,
                models->selection->time,
                models->selection->index,
                *models->moving);
        }
        models->moving.reset();
        models->handle = -1;
        models->needsDraw();
        return;
    }
    if (models->drawing && models::strokeComplete(models->stroke))
    {
        models->annotationsModel->addStroke(models->fileId, models->strokeTime, models->stroke);
    }
    // What the eraser took stays taken.
    models->eraseSteps = 0;
    djv_draw_cancel(models);
}

void djv_draw_cancel(djv_models* models)
{
    if (!models)
        return;
    const bool was = models->drawing || models->eraseSteps > 0;
    models->drawing = false;
    if (models->erasing)
    {
        // What the eraser took is put back, as the desktop does for the
        // first finger of a pinch.
        for (; models->eraseSteps > 0; --models->eraseSteps)
        {
            models->annotationsModel->undo();
        }
    }
    models->erasing = false;
    // Text being typed is dropped, and a move put back.
    if (models->typing)
    {
        models->textEnd(false);
    }
    if (models->moving.has_value())
    {
        models->moving.reset();
        models->handle = -1;
        models->needsDraw();
    }
    models->stroke = models::ReviewStroke();
    if (was)
    {
        models->needsDraw();
    }
}

float djv_draw_get_text_size(djv_models* models)
{
    return models ? models->drawModel->getTextSize() : 48.F;
}

void djv_draw_set_text_size(djv_models* models, float value)
{
    if (models)
        models->drawModel->setTextSize(value);
}

int djv_text_is_open(djv_models* models)
{
    return models && models->typing ? 1 : 0;
}

const char* djv_text_get(djv_models* models)
{
    return models && models->typing ? models->stroke.text.c_str() : "";
}

void djv_text_set(djv_models* models, const char* text)
{
    if (!models || !models->typing)
        return;
    models->stroke.text = text ? text : "";
    models->needsDraw();
}

void djv_text_get_area(djv_models* models, float* x, float* y, float* height)
{
    float ox = 0.F, oy = 0.F, oh = 0.F;
    if (models && models->typing && !models->stroke.points.empty())
    {
        const models::ImageMapping m = models->mapping();
        const auto picture = tl::c::getViewPicture(models->view);
        if (m.isValid())
        {
            const ftk::V2F r = m.toRender(models->stroke.points.front());
            ox = picture.pos.x + r.x * picture.zoom;
            oy = picture.pos.y + r.y * picture.zoom;
            oh = models->stroke.textSize * m.scale() * picture.zoom;
        }
    }
    if (x) *x = ox;
    if (y) *y = oy;
    if (height) *height = oh;
}

void djv_text_end(djv_models* models, int commit)
{
    if (models)
        models->textEnd(commit != 0);
}

int djv_select_has(djv_models* models)
{
    return models && models->selected().has_value() ? 1 : 0;
}

int djv_select_get_index(djv_models* models)
{
    return models && models->selected().has_value() ? static_cast<int>(models->selection->index) : -1;
}

int djv_select_is_text(djv_models* models)
{
    if (!models)
        return 0;
    const auto stroke = models->selected();
    return stroke.has_value() && models::ReviewStrokeKind::Text == stroke->kind ? 1 : 0;
}

void djv_select_remove(djv_models* models)
{
    if (!models || models->typing || !models->selected().has_value())
        return;
    const auto selection = *models->selection;
    models->selection.reset();
    models->moving.reset();
    models->annotationsModel->removeStroke(models->fileId, selection.time, selection.index);
}

int djv_select_edit_text(djv_models* models)
{
    if (!models || models->typing)
        return 0;
    const auto stroke = models->selected();
    if (!stroke.has_value() || stroke->kind != models::ReviewStrokeKind::Text)
        return 0;
    models->moving.reset();
    models->stroke = *stroke;
    models->strokeTime = models->selection->time;
    models->typing = true;
    models->typingSelection = true;
    models->needsDraw();
    ++models->generation;
    return 1;
}

void djv_select_clear(djv_models* models)
{
    if (models)
        models->selectionClear();
}

int djv_draw_has_undo(djv_models* models)
{
    return models && models->annotationsModel->observeHasUndo()->get() ? 1 : 0;
}

int djv_draw_has_redo(djv_models* models)
{
    return models && models->annotationsModel->observeHasRedo()->get() ? 1 : 0;
}

void djv_draw_undo(djv_models* models)
{
    if (models)
    {
        models->annotationsModel->undo();
    }
}

void djv_draw_redo(djv_models* models)
{
    if (models)
    {
        models->annotationsModel->redo();
    }
}

void djv_draw_clear_frame(djv_models* models)
{
    if (!models || models->fileId.empty())
        return;
    if (const auto time = models->currentTime())
    {
        models->annotationsModel->clearFrame({ models->fileId }, *time);
    }
}

size_t djv_draw_get_frame_count(djv_models* models)
{
    return models ? models->frames.size() : 0;
}

double djv_draw_get_frame(djv_models* models, size_t i)
{
    return models && i < models->frames.size() ? models->frames[i] : 0.0;
}

int djv_draw_has_frame(djv_models* models)
{
    if (!models || models->fileId.empty())
        return 0;
    const auto time = models->currentTime();
    return time.has_value() && !models->annotationsModel->getStrokes(models->fileId, *time).empty() ? 1 : 0;
}

double djv_shuttle_get_time(djv_models* models, double start, float dx)
{
    if (!models)
        return start;
    auto player = tl::c::getPlayer(models->player);
    if (!player)
        return start;
    const auto& range = player->getTimeRange();
    const double rate = range.duration().rate();
    const auto t = models::getFrameShuttleTime(
        range.start_time() + OTIO_NS::RationalTime(start * rate, rate).round(),
        range,
        dx,
        models->settingsModel->getMouse().frameShuttleScale);
    return (t - range.start_time()).rescaled_to(1.0).value();
}

double djv_review_get_jump(djv_models* models, const double* markers, size_t count, int next)
{
    if (!models)
        return -1.0;
    auto p = tl::c::getPlayer(models->player);
    if (!p)
        return -1.0;
    const auto& range = p->getTimeRange();
    const double rate = range.duration().rate();
    const auto frame = [rate](double seconds)
    {
        return static_cast<int>(std::round(seconds * rate));
    };
    std::vector<int> frames;
    for (double seconds : models->frames)
    {
        frames.push_back(frame(seconds));
    }
    for (size_t i = 0; markers && i < count; ++i)
    {
        frames.push_back(frame(markers[i]));
    }
    for (const auto& marker : models->markersModel->getMarkers())
    {
        if (marker.range.has_value())
        {
            frames.push_back(frame(
                (marker.range->start_time() - range.start_time()).rescaled_to(1.0).value()));
        }
    }
    std::sort(frames.begin(), frames.end());
    frames.erase(std::unique(frames.begin(), frames.end()), frames.end());
    const auto target = models::getReviewJump(
        frames,
        frame((p->getCurrentTime() - range.start_time()).rescaled_to(1.0).value()),
        next != 0);
    return target.has_value() ? *target / rate : -1.0;
}

namespace
{
    const models::ReviewMarker* note(djv_models* models, size_t i)
    {
        if (!models)
            return nullptr;
        const auto& markers = models->markersModel->getMarkers();
        return i < markers.size() ? &markers[i] : nullptr;
    }
}

size_t djv_note_get_count(djv_models* models)
{
    return models ? models->markersModel->getMarkers().size() : 0;
}

const char* djv_note_get_id(djv_models* models, size_t i)
{
    const auto* n = note(models, i);
    return n ? n->id.c_str() : "";
}

const char* djv_note_get_name(djv_models* models, size_t i)
{
    const auto* n = note(models, i);
    return n ? n->name.c_str() : "";
}

const char* djv_note_get_text(djv_models* models, size_t i)
{
    const auto* n = note(models, i);
    return n ? n->text.c_str() : "";
}

const char* djv_note_get_author(djv_models* models, size_t i)
{
    const auto* n = note(models, i);
    return n ? n->author.c_str() : "";
}

const char* djv_note_get_created(djv_models* models, size_t i)
{
    const auto* n = note(models, i);
    return n ? n->created.c_str() : "";
}

void djv_note_get_range(djv_models* models, size_t i, double* start, double* duration)
{
    double s = -1.0;
    double d = 0.0;
    const auto* n = note(models, i);
    auto p = models ? tl::c::getPlayer(models->player) : nullptr;
    if (n && n->range.has_value() && p)
    {
        const auto& range = p->getTimeRange();
        s = (n->range->start_time() - range.start_time()).rescaled_to(1.0).value();
        d = n->range->duration().rescaled_to(1.0).value();
    }
    if (start) *start = s;
    if (duration) *duration = d;
}

void djv_note_get_color(djv_models* models, size_t i, float* rgba)
{
    const auto* n = note(models, i);
    const ftk::Color4F c = n ? n->color : models::reviewMarkerColor();
    if (rgba)
    {
        rgba[0] = c.r;
        rgba[1] = c.g;
        rgba[2] = c.b;
        rgba[3] = c.a;
    }
}

const char* djv_note_add(djv_models* models, const char* text)
{
    if (!models || models->reviewPath.empty())
        return nullptr;
    auto p = tl::c::getPlayer(models->player);
    if (!p)
        return nullptr;
    const OTIO_NS::RationalTime time = p->getCurrentTime();
    models->noteId = models->markersModel->add(
        OTIO_NS::TimeRange(time, OTIO_NS::RationalTime(1.0, time.rate())),
        std::string(),
        text ? text : "");
    return models->noteId.c_str();
}

const char* djv_note_add_range(
    djv_models* models,
    double start,
    double duration,
    const char* text)
{
    if (!models || models->reviewPath.empty())
        return nullptr;
    auto p = tl::c::getPlayer(models->player);
    if (!p)
        return nullptr;
    // Whole frames of the timeline, from its start.
    const OTIO_NS::TimeRange& timeRange = p->getTimeRange();
    const double rate = timeRange.duration().rate();
    const OTIO_NS::RationalTime first =
        timeRange.start_time() + OTIO_NS::RationalTime(start * rate, rate).round();
    const OTIO_NS::RationalTime count =
        OTIO_NS::RationalTime(std::max(1.0, std::round(duration * rate)), rate);
    models->noteId = models->markersModel->add(
        OTIO_NS::TimeRange(first, count),
        std::string(),
        text ? text : "");
    return models->noteId.c_str();
}

void djv_note_set_color(djv_models* models, const char* id, const float rgba[4])
{
    if (models && id && rgba)
    {
        models->markersModel->updateColor(id, ftk::Color4F(rgba[0], rgba[1], rgba[2], rgba[3]));
    }
}

void djv_note_set_text(djv_models* models, const char* id, const char* text)
{
    if (models && id)
    {
        models->markersModel->update(id, text ? text : "");
    }
}

void djv_note_remove(djv_models* models, const char* id)
{
    if (models && id)
    {
        models->markersModel->remove(id);
    }
}

const char* djv_review_get_author(djv_models* models)
{
    if (!models)
        return "";
    models->author = models->settingsModel->getReview().author;
    return models->author.c_str();
}

void djv_review_set_author(djv_models* models, const char* value)
{
    if (!models)
        return;
    auto review = models->settingsModel->getReview();
    review.author = value ? value : "";
    models->settingsModel->setReview(review);
}

const char* djv_review_get_compare_file(djv_models* models)
{
    return models ? models->reviewCompareFile.c_str() : "";
}

void djv_review_restore_compare(djv_models* models)
{
    if (!models)
        return;
    auto options = models->review.compare.options;
    if (!models->compareFile)
    {
        options.compare = tl::Compare::None;
    }
    models->compareQuiet = true;
    models->filesModel->setCompareOptions(options);
    models->filesModel->setCompareTime(models->review.compare.time);
    models->compareQuiet = false;
    models->compareChanged = false;
}

void djv_draw_clear_all(djv_models* models)
{
    if (!models || models->fileId.empty())
        return;
    djv_draw_cancel(models);
    models->annotationsModel->clearSources({ models->fileId });
}

void djv_note_clear_all(djv_models* models)
{
    if (models)
    {
        models->markersModel->clear();
    }
}

size_t djv_review_read_file_names(djv_models* models, const char* path)
{
    if (!models)
        return 0;
    models->reviewFileNames.clear();
    if (!path)
        return 0;
    try
    {
        const models::Review review = models::reviewOpen(path);
        auto name = [](const models::ReviewFile& file)
        {
            const std::string& p = !file.path.empty() ? file.path : file.pathAbsolute;
            return ftk::fromFileSystem(std::filesystem::u8path(p).filename());
        };
        // "A" first: the file the review is about.
        for (const auto& file : review.files)
        {
            if (file.id == review.compare.aId)
            {
                models->reviewFileNames.push_back(name(file));
            }
        }
        for (const auto& file : review.files)
        {
            if (file.id != review.compare.aId)
            {
                models->reviewFileNames.push_back(name(file));
            }
        }
    }
    catch (const std::exception& e)
    {
        models->reviewError = e.what();
    }
    return models->reviewFileNames.size();
}

const char* djv_review_get_file_name(djv_models* models, size_t i)
{
    return models && i < models->reviewFileNames.size() ?
        models->reviewFileNames[i].c_str() : "";
}

size_t djv_shortcut_get_count(djv_models* models)
{
    if (!models)
        return 0;
    models->shortcuts = models->settingsModel->getShortcuts().shortcuts;
    models->shortcutKeys.clear();
    for (const auto& shortcut : models->shortcuts)
    {
        const auto name = [](const ftk::KeyShortcut& value)
        {
            return value.key != ftk::Key::Unknown ? ftk::to_string(value.key) : std::string();
        };
        models->shortcutKeys.push_back({ name(shortcut.primary), name(shortcut.secondary) });
    }
    return models->shortcuts.size();
}

const char* djv_shortcut_get_name(djv_models* models, size_t index)
{
    return models && index < models->shortcuts.size() ?
        models->shortcuts[index].name.c_str() :
        "";
}

const char* djv_shortcut_get_key(djv_models* models, size_t index, int which, int* modifiers)
{
    if (modifiers)
    {
        *modifiers = 0;
    }
    if (!models || index >= models->shortcuts.size())
        return "";
    const auto& shortcut = models->shortcuts[index];
    const auto& key = which ? shortcut.secondary : shortcut.primary;
    if (modifiers)
    {
        *modifiers = key.modifiers;
    }
    return which ?
        models->shortcutKeys[index].second.c_str() :
        models->shortcutKeys[index].first.c_str();
}

const char* djv_still_get_name(djv_models* models, const char* ext)
{
    if (!models)
        return "";
    models->stillName.clear();
    if (auto player = tl::c::getPlayer(models->player))
    {
        models->stillName = models::getStillFileName(
            player->getPath(),
            models->timeUnitsModel->getLabel(player->getCurrentTime()),
            ext ? ext : ".png");
    }
    return models->stillName.c_str();
}

int djv_still_write(djv_models* models, const char* path)
{
    if (!models || !path)
        return 0;
    models->stillError.clear();
    try
    {
        // The drawings the view shows: A's.
        models::writeStill(
            models->context,
            tl::c::getPlayer(models->player),
            models->viewportModel,
            models->colorModel,
            models->filesModel,
            models->annotationsModel,
            { models->fileId },
            true,
            ftk::Path(path));
    }
    catch (const std::exception& e)
    {
        models->stillError = e.what();
        return 0;
    }
    return 1;
}

const char* djv_still_get_error(djv_models* models)
{
    return models ? models->stillError.c_str() : "";
}
