// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ExportTool.h>

#include <djv/App/App.h>
#include <djv/App/ExportWidgets.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/GL/Render.h>
#include <tlRender/Timeline/IRender.h>
#include <tlRender/Timeline/Util.h>
#include <tlRender/IO/System.h>
#if defined(TLRENDER_FFMPEG_PLUGIN)
#include <tlRender/IO/FFmpeg.h>
#endif // TLRENDER_FFMPEG_PLUGIN

#include <tlRender/Core/Audio.h>

#include <ftk/UI/ComboBox.h>
#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/FileEdit.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/IntEdit.h>
#include <ftk/UI/ProgressDialog.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/TabBar.h>
#include <ftk/UI/TabWidget.h>
#include <ftk/GL/GL.h>
#include <ftk/GL/OffscreenBuffer.h>
#include <ftk/GL/Util.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Timer.h>

#include <cstring>

namespace djv
{
    namespace app
    {
        struct ExportTool::Private
        {
            std::shared_ptr<tl::Player> player;
            std::shared_ptr<models::SettingsModel> settings;

            struct ExportData
            {
                OTIO_NS::TimeRange range;
                int64_t frame = 0;
                ftk::Path path;
                ftk::ImageInfo info;
                std::shared_ptr<tl::IWrite> writer;
                bool hasAudio = false;
                double audioStartSeconds = 0.0;
                double audioDurationSeconds = 0.0;
                double audioSeconds = 0.0;
                int64_t audioSamples = 0;
                tl::OCIOOptions ocioOptions;
                tl::LUTOptions lutOptions;
                ftk::ImageOptions imageOptions;
                tl::DisplayOptions displayOptions;
                ftk::gl::TextureType colorBuffer = ftk::gl::TextureType::RGBA_U8;
                std::shared_ptr<ftk::gl::OffscreenBuffer> buffer;
                std::shared_ptr<tl::IRender> render;
                GLenum glFormat = 0;
                GLenum glType = 0;
            };
            std::unique_ptr<ExportData> exportData;

            std::shared_ptr<ftk::FileEdit> dirEdit;
            std::shared_ptr<ftk::ComboBox> renderSizeComboBox;
            std::shared_ptr<ftk::IntEdit> renderWidthEdit;
            std::shared_ptr<ftk::IntEdit> renderHeightEdit;
            std::shared_ptr<ImageExportWidget> imageWidget;
            std::shared_ptr<SeqExportWidget> seqWidget;
            std::shared_ptr<MovieExportWidget> movieWidget;
            std::shared_ptr<ftk::TabWidget> tabWidget;
            std::shared_ptr<ftk::HorizontalLayout> customSizeLayout;
            std::shared_ptr<ftk::FormLayout> formLayout;
            std::shared_ptr<ftk::VerticalLayout> layout;
            std::shared_ptr<ftk::ProgressDialog> progressDialog;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<models::ExportSettings> > settingsObserver;

            std::shared_ptr<ftk::Timer> progressTimer;
        };

        void ExportTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                mainWindow,
                "Export",
                "Export",
                "djv::app::ExportTool",
                parent);
            FTK_P();

            p.settings = app->getSettingsModel();

            p.dirEdit = ftk::FileEdit::create(context, ftk::FileBrowserMode::Dir);
            ftk::setScreenshotTag(p.dirEdit, "Export.Dir");

            p.renderSizeComboBox = ftk::ComboBox::create(context, models::getExportRenderSizeLabels());
            ftk::setScreenshotTag(p.renderSizeComboBox, "Export.RenderSize");
            p.renderWidthEdit = ftk::IntEdit::create(context);
            p.renderWidthEdit->setRange(1, 16384);
            p.renderHeightEdit = ftk::IntEdit::create(context);
            p.renderHeightEdit->setRange(1, 16384);

            p.imageWidget = ImageExportWidget::create(context, app);
            p.seqWidget = SeqExportWidget::create(context, app);
            p.movieWidget = MovieExportWidget::create(context, app);

            p.layout = ftk::VerticalLayout::create(context);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto vLayout = ftk::VerticalLayout::create(context, p.layout);
            vLayout->setMarginRole(ftk::SizeRole::Margin);
            p.formLayout = ftk::FormLayout::create(context, vLayout);
            p.formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.formLayout->addRow("Directory:", p.dirEdit);
            p.formLayout->addRow("Render size:", p.renderSizeComboBox);
            p.customSizeLayout = ftk::HorizontalLayout::create(context);
            p.customSizeLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.renderWidthEdit->setParent(p.customSizeLayout);
            p.renderHeightEdit->setParent(p.customSizeLayout);
            p.formLayout->addRow("Custom size:", p.customSizeLayout);
            p.tabWidget = ftk::TabWidget::create(context, p.layout);
            // Tag the tab bar rather than the whole tab widget so that
            // screenshot annotations point at the tabs.
            ftk::setScreenshotTag(p.tabWidget->getTabBar(), "Export.Tabs");
            p.tabWidget->addTab("Image", p.imageWidget);
            p.tabWidget->addTab("Sequence", p.seqWidget);
            p.tabWidget->addTab("Movie", p.movieWidget);

            auto scrollWidget = ftk::ScrollWidget::create(context);
            scrollWidget->setBorder(false);
            scrollWidget->setWidget(p.layout);
            _setWidget(scrollWidget);

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    FTK_P();
                    p.player = value;
                });

            p.settingsObserver = ftk::Observer<models::ExportSettings>::create(
                p.settings->observeExport(),
                [this](const models::ExportSettings& value)
                {
                    _widgetUpdate(value);
                });

            p.dirEdit->setCallback(
                [this](const ftk::Path& value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.dir = value.get();
                    p.settings->setExport(options);
                });

            p.renderSizeComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.renderSize = static_cast<models::ExportRenderSize>(value);
                    p.settings->setExport(options);
                });

            p.renderWidthEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.customSize.w = value;
                    p.settings->setExport(options);
                });

            p.renderHeightEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.customSize.h = value;
                    p.settings->setExport(options);
                });

            p.tabWidget->setCallback(
                [this](int value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.fileType = static_cast<models::ExportFileType>(value);
                    p.settings->setExport(options);
                });

            p.imageWidget->setExportCallback(
                [this]
                {
                    _export(models::ExportFileType::Image);
                });

            p.seqWidget->setExportCallback(
                [this]
                {
                    _export(models::ExportFileType::Seq);
                });

            p.movieWidget->setExportCallback(
                [this]
                {
                    _export(models::ExportFileType::Movie);
                });

            p.progressTimer = ftk::Timer::create(context);
            p.progressTimer->setRepeating(true);
        }

        ExportTool::ExportTool() :
            _p(new Private)
        {}

        ExportTool::~ExportTool()
        {}

        std::shared_ptr<ExportTool> ExportTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ExportTool>(new ExportTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }

        void ExportTool::_widgetUpdate(const models::ExportSettings& settings)
        {
            FTK_P();
            p.dirEdit->setPath(ftk::Path(settings.dir));
            p.renderSizeComboBox->setCurrentIndex(static_cast<int>(settings.renderSize));
            p.renderWidthEdit->setValue(settings.customSize.w);
            p.renderHeightEdit->setValue(settings.customSize.h);
            p.formLayout->setRowVisible(
                p.customSizeLayout,
                models::ExportRenderSize::Custom == settings.renderSize);
            p.tabWidget->setCurrent(static_cast<int>(settings.fileType));
        }

        void ExportTool::_export(models::ExportFileType fileType)
        {
            FTK_P();
            if (p.player)
            {
                auto context = getContext();
                try
                {
                    const tl::IOInfo ioInfo = p.player->getIOInfo();
                    if (ioInfo.video.empty())
                    {
                        throw std::runtime_error("No video to render");
                    }
                    p.exportData.reset(new Private::ExportData);

                    // Get the time range.
                    const auto options = p.settings->getExport();
                    switch (fileType)
                    {
                    case models::ExportFileType::Image:
                        p.exportData->range = OTIO_NS::TimeRange(
                            p.player->getCurrentTime(),
                            OTIO_NS::RationalTime(1.0, p.player->getTimeRange().duration().rate()));
                        break;
                    default:
                        p.exportData->range = p.player->getInOutRange();
                        break;
                    }
                    p.exportData->frame = p.exportData->range.start_time().value();

                    // Get the render size.
                    auto app = _app.lock();
                    const auto& displayOptions = app->getViewportModel()->getDisplayOptions();
                    switch (options.renderSize)
                    {
                    case models::ExportRenderSize::Default:
                        p.exportData->info.size = getRenderSize(
                            ioInfo.video.front(),
                            displayOptions.aspectRatio);
                        break;
                    case models::ExportRenderSize::Custom:
                        p.exportData->info.size = options.customSize;
                        break;
                    default:
                        p.exportData->info.size = getSize(options.renderSize);
                        break;
                    }

                    // Get the export path.
                    const std::string fileName = getExportFileName(
                        options,
                        fileType,
                        p.exportData->range.start_time().value());
                    p.exportData->path = ftk::Path(options.dir, fileName);

                    // Get the writer.
                    auto ioSystem = context->getSystem<tl::WriteSystem>();
                    auto plugin = ioSystem->getPlugin(p.exportData->path);
                    if (!plugin)
                    {
                        throw std::runtime_error(
                            ftk::Format("Cannot open: \"{0}\"").arg(p.exportData->path.get()));
                    }
                    p.exportData->info.type = ioInfo.video.front().type;
                    p.exportData->info = plugin->getInfo(p.exportData->info);
                    if (ftk::ImageType::None == p.exportData->info.type)
                    {
                        p.exportData->info.type = ftk::ImageType::RGBA_U8;
                    }
                    p.exportData->glFormat = ftk::gl::getReadPixelsFormat(p.exportData->info.type);
                    p.exportData->glType = ftk::gl::getReadPixelsType(p.exportData->info.type);
                    if (GL_NONE == p.exportData->glFormat || GL_NONE == p.exportData->glType)
                    {
                        throw std::runtime_error(
                            ftk::Format("Cannot open: \"{0}\"").arg(p.exportData->path.get()));
                    }
                    const double speed = p.player->getSpeed();
                    tl::IOInfo outputInfo;
                    outputInfo.video.push_back(p.exportData->info);
                    outputInfo.videoTime = OTIO_NS::TimeRange(
                        OTIO_NS::RationalTime(0.0, speed),
                        p.exportData->range.duration().rescaled_to(speed));
#if defined(TLRENDER_FFMPEG_PLUGIN)
                    if (models::ExportFileType::Movie == fileType &&
                        ioInfo.audio.isValid() &&
                        std::dynamic_pointer_cast<tl::ffmpeg::WritePlugin>(plugin))
                    {
                        p.exportData->hasAudio = true;
                        outputInfo.audio = ioInfo.audio;
                        outputInfo.audioTime = OTIO_NS::TimeRange(
                            OTIO_NS::RationalTime(0.0, ioInfo.audio.sampleRate),
                            p.exportData->range.duration().rescaled_to(ioInfo.audio.sampleRate));
                        p.exportData->audioStartSeconds =
                            p.exportData->range.start_time().rescaled_to(1.0).value();
                        p.exportData->audioDurationSeconds =
                            p.exportData->range.duration().rescaled_to(1.0).value();
                    }
#endif // TLRENDER_FFMPEG_PLUGIN
                    tl::IOOptions ioOptions;
                    ioOptions["FFmpeg/Codec"] = options.movieCodec;
                    if (!options.movieAudioCodec.empty() &&
                        options.movieAudioCodec != "Auto")
                    {
                        ioOptions["FFmpeg/AudioCodec"] = options.movieAudioCodec;
                    }
                    p.exportData->writer = plugin->write(p.exportData->path, outputInfo, ioOptions);

                    // Create the renderer.
                    p.exportData->ocioOptions = app->getColorModel()->getOCIOOptions();
                    p.exportData->lutOptions = app->getColorModel()->getLUTOptions();
                    p.exportData->imageOptions = app->getViewportModel()->getImageOptions();
                    p.exportData->displayOptions = app->getViewportModel()->getDisplayOptions();
                    p.exportData->colorBuffer = app->getViewportModel()->getColorBuffer();
                    p.exportData->render = tl::gl::Render::create(
                        context->getLogSystem(),
                        context->getSystem<ftk::FontSystem>());
                    ftk::gl::OffscreenBufferOptions offscreenBufferOptions;
                    p.exportData->buffer = ftk::gl::OffscreenBuffer::create(
                        p.exportData->info.size,
                        p.exportData->colorBuffer,
                        offscreenBufferOptions);

                    // Create the progress dialog.
                    p.progressDialog = ftk::ProgressDialog::create(
                        context,
                        "Export",
                        "Rendering:");
                    p.progressDialog->setRange(0.0, p.exportData->range.duration().value() - 1.0);
                    p.progressDialog->setMessage(ftk::Format("Frame: {0} / {1}").
                        arg(p.exportData->frame).
                        arg(p.exportData->range.end_time_inclusive().value()));
                    p.progressDialog->setCloseCallback(
                        [this]
                        {
                            FTK_P();
                            p.progressTimer->stop();
                            p.exportData.reset();
                            p.progressDialog.reset();
                        });
                    p.progressDialog->open(getWindow());
                    p.progressTimer->start(
                        std::chrono::microseconds(500),
                        [this]
                        {
                            FTK_P();
                            if (_exportFrame())
                            {
                                const int64_t start = p.exportData->range.start_time().value();
                                p.progressDialog->setValue(p.exportData->frame - start);
                                const int64_t end = p.exportData->range.end_time_inclusive().value();
                                if (p.exportData->frame <= end)
                                {
                                    p.progressDialog->setMessage(ftk::Format("Frame: {0} / {1}").
                                        arg(p.exportData->frame - start).
                                        arg(static_cast<int64_t>(p.exportData->range.duration().value())));
                                }
                                else
                                {
                                    p.progressDialog->close();
                                }
                            }
                            else if (p.progressDialog)
                            {
                                p.progressDialog->close();
                            }
                        });
                }
                catch (const std::exception& e)
                {
                    if (p.progressDialog)
                    {
                        p.progressDialog->close();
                    }
                    context->getSystem<ftk::DialogSystem>()->message(
                        "ERROR",
                        ftk::Format("Error: {0}").arg(e.what()),
                        getWindow());
                }
            }
        }

        bool ExportTool::_exportFrame()
        {
            FTK_P();
            bool out = false;
            try
            {
                // Get the video.
                const OTIO_NS::RationalTime t(p.exportData->frame, p.exportData->range.duration().rate());
                auto ioOptions = p.player->getTimeline()->getOptions().ioOptions;
                ioOptions["Layer"] = ftk::Format("{0}").arg(p.player->getVideoLayer());
                auto video = p.player->getTimeline()->getVideo(t, ioOptions).future.get();

                // Render the video.
                ftk::gl::OffscreenBufferBinding binding(p.exportData->buffer);
                p.exportData->render->begin(p.exportData->info.size);
                p.exportData->render->setOCIOOptions(p.exportData->ocioOptions);
                p.exportData->render->setLUTOptions(p.exportData->lutOptions);
                p.exportData->render->drawVideo(
                    { video },
                    { ftk::Box2I(0, 0, p.exportData->info.size.w, p.exportData->info.size.h) },
                    { p.exportData->imageOptions },
                    { p.exportData->displayOptions },
                    tl::CompareOptions(),
                    p.exportData->colorBuffer);
                p.exportData->render->end();

                // Write the output image.
                auto image = ftk::Image::create(p.exportData->info);
                glPixelStorei(GL_PACK_ALIGNMENT, p.exportData->info.layout.alignment);
#if defined(FTK_API_GL_4_1)
                glPixelStorei(GL_PACK_SWAP_BYTES, p.exportData->info.layout.endian != ftk::getEndian());
#endif // FTK_API_GL_4_1
                glReadPixels(
                    0,
                    0,
                    p.exportData->info.size.w,
                    p.exportData->info.size.h,
                    p.exportData->glFormat,
                    p.exportData->glType,
                    image->getData());

                const int64_t start = p.exportData->range.start_time().value();
                const double speed = p.player->getSpeed();
                const OTIO_NS::RationalTime t2(p.exportData->frame - start, speed);
                p.exportData->writer->writeVideo(t2, image);

                ++p.exportData->frame;

                // Write the audio.
                _exportAudio();

                // Finish writing after the last frame.
                if (p.exportData->frame > p.exportData->range.end_time_inclusive().value())
                {
                    p.exportData->writer->finish();
                }

                out = true;
            }
            catch (const std::exception& e)
            {
                if (p.progressDialog)
                {
                    p.progressDialog->close();
                }
                if (auto context = getContext())
                {
                    context->getSystem<ftk::DialogSystem>()->message(
                        "ERROR",
                        ftk::Format("Error: {0}").arg(e.what()),
                        getWindow());
                }
            }
            return out;
        }

        void ExportTool::_exportAudio()
        {
            FTK_P();
            if (!p.exportData->hasAudio)
                return;

            const int64_t start = p.exportData->range.start_time().value();
            const double speed = p.player->getSpeed();
            const double videoSeconds = OTIO_NS::RationalTime(
                p.exportData->frame - start,
                speed).rescaled_to(1.0).value();
            while (p.exportData->audioSeconds <
                std::min(videoSeconds, p.exportData->audioDurationSeconds))
            {
                // Get one second of audio from the timeline and mix the
                // layers together.
                auto frame = p.player->getTimeline()->getAudio(
                    p.exportData->audioStartSeconds + p.exportData->audioSeconds).future.get();
                std::vector<std::shared_ptr<tl::Audio> > layers;
                for (const auto& layer : frame.layers)
                {
                    if (layer.audio)
                    {
                        layers.push_back(layer.audio);
                    }
                }
                auto audio = tl::mixAudio(layers, 1.F);
                if (audio && audio->isValid())
                {
                    // Trim the final chunk to the in/out range.
                    const double remaining =
                        p.exportData->audioDurationSeconds - p.exportData->audioSeconds;
                    if (remaining < 1.0)
                    {
                        const size_t sampleCount = std::min(
                            audio->getSampleCount(),
                            static_cast<size_t>(
                                remaining * audio->getInfo().sampleRate + .5));
                        auto tmp = tl::Audio::create(audio->getInfo(), sampleCount);
                        std::memcpy(
                            tmp->getData(),
                            audio->getData(),
                            tmp->getByteCount());
                        audio = tmp;
                    }
                    const OTIO_NS::TimeRange timeRange(
                        OTIO_NS::RationalTime(
                            p.exportData->audioSamples,
                            audio->getInfo().sampleRate),
                        OTIO_NS::RationalTime(
                            audio->getSampleCount(),
                            audio->getInfo().sampleRate));
                    p.exportData->writer->writeAudio(timeRange, audio);
                    p.exportData->audioSamples += audio->getSampleCount();
                }
                p.exportData->audioSeconds += 1.0;
            }
        }
    }
}
