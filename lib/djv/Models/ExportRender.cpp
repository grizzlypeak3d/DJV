// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/ExportRender.h>

#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/StrokeDraw.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/UI/OffscreenRender.h>
#include <tlRender/UI/Viewport.h>
#include <tlRender/Timeline/IRender.h>
#include <tlRender/Timeline/Player.h>
#include <tlRender/IO/System.h>

#include <ftk/Core/Context.h>
#include <ftk/Core/FontSystem.h>
#include <ftk/Core/Format.h>

#include <cmath>
#include <stdexcept>

namespace djv
{
    namespace models
    {
        std::vector<ftk::ImageInfo> getExportInfos(const std::shared_ptr<tl::Player>& player)
        {
            std::vector<ftk::ImageInfo> out;
            if (player)
            {
                const tl::IOInfo& ioInfo = player->getIOInfo();
                if (!ioInfo.video.empty())
                {
                    out.push_back(ioInfo.video.front());
                    for (const auto& compare : player->getCompare())
                    {
                        const tl::IOInfo& compareIOInfo = compare->getIOInfo();
                        out.push_back(
                            !compareIOInfo.video.empty() ?
                            compareIOInfo.video.front() :
                            ftk::ImageInfo());
                    }
                }
            }
            return out;
        }

        ftk::Size2I getExportDefaultSize(
            const tl::CompareOptions& compareOptions,
            const tl::AspectRatioOptions& aspectRatio,
            const std::vector<ftk::ImageInfo>& infos)
        {
            ftk::Size2I out = tl::getRenderSize(compareOptions, aspectRatio, infos);
            if (!out.isValid() && !infos.empty())
            {
                out = tl::getRenderSize(infos.front(), aspectRatio);
            }
            return out;
        }

        std::vector<ftk::Box2I> scaleBoxes(
            const std::vector<ftk::Box2I>& boxes,
            const ftk::Size2I& from,
            const ftk::Size2I& to)
        {
            if (!from.isValid() || from == to)
            {
                return boxes;
            }
            const double sx = to.w / static_cast<double>(from.w);
            const double sy = to.h / static_cast<double>(from.h);
            std::vector<ftk::Box2I> out;
            for (const auto& box : boxes)
            {
                const int x0 = std::lround(box.min.x * sx);
                const int y0 = std::lround(box.min.y * sy);
                const int x1 = std::lround((box.max.x + 1) * sx);
                const int y1 = std::lround((box.max.y + 1) * sy);
                out.push_back(ftk::Box2I(x0, y0, x1 - x0, y1 - y0));
            }
            return out;
        }

        ExportRenderOptions getExportRenderOptions(
            const std::shared_ptr<tl::Player>& player,
            const std::shared_ptr<ViewportModel>& viewportModel,
            const std::shared_ptr<ColorModel>& colorModel,
            const std::shared_ptr<FilesModel>& filesModel,
            const std::shared_ptr<AnnotationsModel>& annotationsModel,
            const std::vector<std::string>& sourceIds,
            const ftk::Size2I& size,
            ftk::ImageType type,
            bool burnAnnotations)
        {
            ExportRenderOptions out;
            out.info.size = size;
            out.info.type = type;
            out.colorBuffer = tl::ui::getViewportColorBufferDefault();

            // The sources and their layout, at the size asked for.
            const auto& displayOptions = viewportModel->getDisplayOptions();
            out.compareOptions = filesModel->getCompareOptions();
            const std::vector<ftk::ImageInfo> infos = getExportInfos(player);
            out.boxes = scaleBoxes(
                tl::getBoxes(out.compareOptions, displayOptions.aspectRatio, infos),
                getExportDefaultSize(out.compareOptions, displayOptions.aspectRatio, infos),
                size);

            // The color as the viewport shows it: each file's resolved input
            // color space, and the same per layer resolution.
            out.ocioOptions = colorModel->getOCIOOptions();
            out.lutOptions = colorModel->getLUTOptions();
            out.imageOptions = std::vector<ftk::ImageOptions>(
                infos.size(),
                viewportModel->getImageOptions());
            out.displayOptions = std::vector<tl::DisplayOptions>(
                infos.size(),
                displayOptions);
            const auto resolvedInputs = colorModel->observeResolvedInputs()->get();
            for (size_t i = 0; i < out.displayOptions.size() && i < resolvedInputs.size(); ++i)
            {
                out.displayOptions[i].ocioInput = resolvedInputs[i];
            }
            std::weak_ptr<ColorModel> colorModelWeak = colorModel;
            out.ocioInputResolver =
                [colorModelWeak](const std::string& path, const ftk::ImageTags& tags)
                {
                    std::string out;
                    if (auto colorModel = colorModelWeak.lock())
                    {
                        if (colorModel->getOCIOOptions().input.empty())
                        {
                            out = colorModel->resolveInput(path, tags);
                        }
                    }
                    return out;
                };

            // The drawings.
            out.burnAnnotations = burnAnnotations;
            if (burnAnnotations)
            {
                if (annotationsModel)
                {
                    out.annotations = annotationsModel->getAnnotations();
                }
                out.sourceIds = sourceIds;
                out.infos = infos;
            }
            return out;
        }

        std::string getStillFileName(
            const ftk::Path& source,
            const std::string& time,
            const std::string& ext)
        {
            // The name without a sequence's frame number and what separates
            // it, or a file's name without its extension.
            std::string name = source.getBase();
            while (!name.empty() && ('.' == name.back() || '_' == name.back() || '-' == name.back()))
            {
                name.pop_back();
            }
            if (name.empty())
            {
                name = "still";
            }
            std::string label = time;
            for (auto& c : label)
            {
                switch (c)
                {
                case ':':
                case ';':
                case '/':
                case '\\':
                case '*':
                case '?':
                case '"':
                case '<':
                case '>':
                case '|':
                case ' ':
                    c = '-';
                    break;
                default: break;
                }
            }
            return label.empty() ? name + ext : name + "_" + label + ext;
        }

        void writeStill(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<tl::Player>& player,
            const std::shared_ptr<ViewportModel>& viewportModel,
            const std::shared_ptr<ColorModel>& colorModel,
            const std::shared_ptr<FilesModel>& filesModel,
            const std::shared_ptr<AnnotationsModel>& annotationsModel,
            const std::vector<std::string>& sourceIds,
            bool burnAnnotations,
            const ftk::Path& path)
        {
            if (!player)
            {
                throw std::runtime_error("No file to write a still of");
            }
            const tl::IOInfo& ioInfo = player->getIOInfo();
            if (ioInfo.video.empty())
            {
                throw std::runtime_error("No video to render");
            }
            auto plugin = context->getSystem<tl::WriteSystem>()->getPlugin(path);
            if (!plugin)
            {
                throw std::runtime_error(
                    ftk::Format("Cannot write: \"{0}\"").arg(path.get()));
            }

            // The size the sources are shown at together, and the type the
            // writer takes nearest the source's.
            ftk::ImageInfo info;
            info.size = getExportDefaultSize(
                filesModel->getCompareOptions(),
                viewportModel->getDisplayOptions().aspectRatio,
                getExportInfos(player));
            info.type = ioInfo.video.front().type;
            info = plugin->getInfo(info);
            if (ftk::ImageType::None == info.type)
            {
                info.type = ftk::ImageType::RGBA_U8;
            }

            // What the pixels are: the display's description when color
            // managed, which is what is baked in, otherwise the source's.
            const OTIO_NS::RationalTime time = player->getCurrentTime();
            tl::IOInfo outputInfo;
            outputInfo.video.push_back(info);
            outputInfo.videoTime = OTIO_NS::TimeRange(time, OTIO_NS::RationalTime(1.0, time.rate()));
            const tl::OCIOOptions& ocioOptions = colorModel->getOCIOOptions();
            if (ocioOptions.enabled && !ocioOptions.display.empty() && !ocioOptions.view.empty())
            {
                const ftk::ImageTags colorTags = tl::getDisplayColorTags(ocioOptions, true);
                outputInfo.tags.insert(colorTags.begin(), colorTags.end());
            }
            else
            {
                for (const auto& tag : { "Color Primaries", "Color Transfer", "Chromaticities" })
                {
                    const auto i = ioInfo.tags.find(tag);
                    if (i != ioInfo.tags.end())
                    {
                        outputInfo.tags[tag] = i->second;
                    }
                }
            }

            auto render = ExportRender::create(
                context,
                getExportRenderOptions(
                    player,
                    viewportModel,
                    colorModel,
                    filesModel,
                    annotationsModel,
                    sourceIds,
                    info.size,
                    info.type,
                    burnAnnotations));
            // The file is written with the name given. A name that ends in
            // a number, as one ending in the time does, would otherwise be
            // taken for a frame of a sequence, and the number replaced with
            // the frame's.
            ftk::PathOptions pathOptions;
            pathOptions.seqMaxDigits = 0;
            const ftk::Path writePath(path.get(), pathOptions);
            auto writer = plugin->write(writePath, outputInfo, tl::IOOptions());
            writer->writeVideo(time, render->render(player, time));
            writer->finish();
        }

        struct ExportRender::Private
        {
            std::weak_ptr<ftk::Context> context;
            ExportRenderOptions options;
            std::shared_ptr<tl::ui::OffscreenRender> offscreen;
            std::shared_ptr<tl::IRender> render;
        };

        void ExportRender::_init(
            const std::shared_ptr<ftk::Context>& context,
            const ExportRenderOptions& options)
        {
            FTK_P();
            p.context = context;
            p.options = options;
            if (ftk::ImageType::None == p.options.colorBuffer)
            {
                p.options.colorBuffer = tl::ui::getViewportColorBufferDefault();
            }
            if (!tl::ui::OffscreenRender::canRead(p.options.info.type))
            {
                throw std::runtime_error(
                    ftk::Format("Cannot render images of the type: {0}").
                    arg(p.options.info.type));
            }
            p.offscreen = tl::ui::OffscreenRender::create(
                context,
                p.options.info.size,
                p.options.colorBuffer);
            p.render = p.offscreen->getRender();
            p.render->setOCIOInputResolver(p.options.ocioInputResolver);
        }

        ExportRender::ExportRender() :
            _p(new Private)
        {}

        ExportRender::~ExportRender()
        {}

        std::shared_ptr<ExportRender> ExportRender::create(
            const std::shared_ptr<ftk::Context>& context,
            const ExportRenderOptions& options)
        {
            auto out = std::shared_ptr<ExportRender>(new ExportRender);
            out->_init(context, options);
            return out;
        }

        std::shared_ptr<ftk::Image> ExportRender::render(
            const std::shared_ptr<tl::Player>& player,
            const OTIO_NS::RationalTime& t)
        {
            FTK_P();

            // The video for the A file and each of the files it is being
            // compared with. The requests are all made before any of them
            // is waited on so that the sources are read in parallel.
            auto ioOptions = player->getTimeline()->getOptions().ioOptions;
            ioOptions["Layer"] = ftk::Format("{0}").arg(player->getVideoLayer());
            std::vector<tl::VideoRequest> requests;
            requests.push_back(player->getTimeline()->getVideo(t, ioOptions));
            const auto& compare = player->getCompare();
            const auto& compareVideoLayers = player->getCompareVideoLayers();
            std::vector<OTIO_NS::RationalTime> times = { t };
            for (size_t i = 0; i < compare.size(); ++i)
            {
                // The same time mapping the player uses, so that the frame
                // rendered for each source is the frame that was on screen.
                const OTIO_NS::RationalTime compareTime = tl::getCompareTime(
                    t,
                    player->getTimeRange(),
                    compare[i]->getTimeRange(),
                    player->getCompareTime());
                times.push_back(compareTime);
                ioOptions["Layer"] = ftk::Format("{0}").arg(
                    i < compareVideoLayers.size() ?
                    compareVideoLayers[i] :
                    player->getVideoLayer());
                requests.push_back(compare[i]->getVideo(compareTime, ioOptions));
            }
            std::vector<tl::VideoFrame> videoFrame;
            for (auto& request : requests)
            {
                videoFrame.push_back(request.future.get());
            }

            // Render the video, and the drawings over it.
            p.offscreen->begin();
            p.render->setOCIOOptions(p.options.ocioOptions);
            p.render->setLUTOptions(p.options.lutOptions);
            p.render->drawVideo(
                videoFrame,
                p.options.boxes,
                p.options.imageOptions,
                p.options.displayOptions,
                p.options.compareOptions,
                p.options.colorBuffer);
            if (p.options.burnAnnotations)
            {
                if (auto context = p.context.lock())
                {
                    auto fontSystem = context->getSystem<ftk::FontSystem>();
                    for (const auto& annotation : p.options.annotations)
                    {
                        // The source's place in the layout, and the frame it
                        // is showing at this time.
                        int index = -1;
                        for (size_t i = 0; i < p.options.sourceIds.size(); ++i)
                        {
                            if (p.options.sourceIds[i] == annotation.sourceId)
                            {
                                index = static_cast<int>(i);
                                break;
                            }
                        }
                        if (index < 0 ||
                            index >= static_cast<int>(p.options.boxes.size()) ||
                            index >= static_cast<int>(p.options.infos.size()) ||
                            index >= static_cast<int>(times.size()))
                        {
                            continue;
                        }
                        if (!sameTime(annotation.time, times[index]))
                        {
                            continue;
                        }
                        const ftk::Box2I& box = p.options.boxes[index];
                        const ftk::Size2I& imageSize = p.options.infos[index].size;
                        if (!imageSize.isValid())
                        {
                            continue;
                        }
                        const float scale = box.w() / static_cast<float>(imageSize.w);
                        for (const auto& stroke : annotation.strokes)
                        {
                            drawStroke(
                                p.render,
                                fontSystem,
                                stroke,
                                [&box, scale](const ftk::V2F& point)
                                {
                                    return ftk::V2F(
                                        box.min.x + point.x * scale,
                                        box.min.y + point.y * scale);
                                },
                                scale);
                        }
                    }
                }
            }
            p.offscreen->end();
            return p.offscreen->read(p.options.info);
        }
    }
}
