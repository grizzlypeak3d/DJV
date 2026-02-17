// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ExportTool.h>

#include <djv/App/App.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/GL/Render.h>
#include <tlRender/Timeline/IRender.h>
#include <tlRender/Timeline/Util.h>
#include <tlRender/IO/System.h>
#if defined(TLRENDER_FFMPEG)
#include <tlRender/IO/FFmpeg.h>
#endif // TLRENDER_FFMPEG

#include <ftk/UI/ComboBox.h>
#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/FileEdit.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/IntEdit.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/LineEdit.h>
#include <ftk/UI/ProgressDialog.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/GL/GL.h>
#include <ftk/GL/OffscreenBuffer.h>
#include <ftk/GL/Util.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Timer.h>

namespace djv
{
    namespace app
    {
        struct ExportTool::Private
        {
            std::weak_ptr<App> app;
            std::shared_ptr<tl::Player> player;
            std::shared_ptr<models::SettingsModel> settings;
            std::vector<std::string> imageExts;
            std::vector<std::string> movieExts;
            std::vector<std::string> movieCodecs;

            struct ExportData
            {
                OTIO_NS::TimeRange range;
                int64_t frame = 0;
                ftk::Path path;
                ftk::ImageInfo info;
                std::shared_ptr<tl::IWrite> writer;
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
            std::shared_ptr<ftk::ComboBox> fileTypeComboBox;
            std::shared_ptr<ftk::LineEdit> imageBaseEdit;
            std::shared_ptr<ftk::IntEdit> imageZeroPadEdit;
            std::shared_ptr<ftk::ComboBox> imageExtComboBox;
            std::shared_ptr<ftk::LineEdit> movieBaseEdit;
            std::shared_ptr<ftk::ComboBox> movieExtComboBox;
            std::shared_ptr<ftk::ComboBox> movieCodecComboBox;
            std::shared_ptr<ftk::PushButton> exportButton;
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
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                models::Tool::Export,
                "djv::app::ExportTool",
                parent);
            FTK_P();

            p.app = app;
            p.settings = app->getSettingsModel();
            auto ioSystem = context->getSystem<tl::WriteSystem>();
            auto exts = ioSystem->getExts(static_cast<int>(tl::FileType::Seq));
            p.imageExts.insert(p.imageExts.end(), exts.begin(), exts.end());
            exts = ioSystem->getExts(static_cast<int>(tl::FileType::Media));
            p.movieExts.insert(p.movieExts.end(), exts.begin(), exts.end());
#if defined(TLRENDER_FFMPEG)
            auto ffmpegPlugin = ioSystem->getPlugin<tl::ffmpeg::WritePlugin>();
            p.movieCodecs = ffmpegPlugin->getCodecs();
#endif // TLRENDER_FFMPEG

            p.dirEdit = ftk::FileEdit::create(context, ftk::FileBrowserMode::Dir);

            p.renderSizeComboBox = ftk::ComboBox::create(context, models::getExportRenderSizeLabels());
            p.renderWidthEdit = ftk::IntEdit::create(context);
            p.renderWidthEdit->setRange(1, 16384);
            p.renderHeightEdit = ftk::IntEdit::create(context);
            p.renderHeightEdit->setRange(1, 16384);

            p.fileTypeComboBox = ftk::ComboBox::create(context, models::getExportFileTypeLabels());

            p.imageBaseEdit = ftk::LineEdit::create(context);
            p.imageZeroPadEdit = ftk::IntEdit::create(context);
            p.imageZeroPadEdit->setRange(0, 16);
            p.imageExtComboBox = ftk::ComboBox::create(context, p.imageExts);

            p.movieBaseEdit = ftk::LineEdit::create(context);
            p.movieExtComboBox = ftk::ComboBox::create(context, p.movieExts);
            p.movieCodecComboBox = ftk::ComboBox::create(context, p.movieCodecs);

            p.exportButton = ftk::PushButton::create(context, "Export");

            p.layout = ftk::VerticalLayout::create(context);
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.formLayout = ftk::FormLayout::create(context, p.layout);
            p.formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.formLayout->addRow("Directory:", p.dirEdit);
            p.formLayout->addRow("Render size:", p.renderSizeComboBox);
            p.customSizeLayout = ftk::HorizontalLayout::create(context);
            p.customSizeLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.renderWidthEdit->setParent(p.customSizeLayout);
            p.renderHeightEdit->setParent(p.customSizeLayout);
            p.formLayout->addRow("Custom size:", p.customSizeLayout);
            p.formLayout->addRow("File type:", p.fileTypeComboBox);
            p.formLayout->addRow("Base name:", p.imageBaseEdit);
            p.formLayout->addRow("Zero padding:", p.imageZeroPadEdit);
            p.formLayout->addRow("Extension:", p.imageExtComboBox);
            p.formLayout->addRow("Base name:", p.movieBaseEdit);
            p.formLayout->addRow("Extension:", p.movieExtComboBox);
            p.formLayout->addRow("Codec:", p.movieCodecComboBox);
            p.exportButton->setParent(p.layout);
            auto label = ftk::Label::create(
                context,
                "Audio export is not currently supported.",
                p.layout);

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
                    p.exportButton->setEnabled(value.get());
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

            p.fileTypeComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.fileType = static_cast<models::ExportFileType>(value);
                    p.settings->setExport(options);
                });

            p.imageBaseEdit->setCallback(
                [this](const std::string& value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.imageBase = value;
                    p.settings->setExport(options);
                });

            p.imageZeroPadEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.imageZeroPad = value;
                    p.settings->setExport(options);
                });

            p.imageExtComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    if (value >= 0 && value < p.imageExts.size())
                    {
                        auto options = p.settings->getExport();
                        options.imageExt = p.imageExts[value];
                        p.settings->setExport(options);
                    }
                });

            p.movieBaseEdit->setCallback(
                [this](const std::string& value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.movieBase = value;
                    p.settings->setExport(options);
                });

            p.movieExtComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    if (value >= 0 && value < p.movieExts.size())
                    {
                        auto options = p.settings->getExport();
                        options.movieExt = p.movieExts[value];
                        p.settings->setExport(options);
                    }
                });

            p.movieCodecComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    if (value >= 0 && value < p.movieCodecs.size())
                    {
                        auto options = p.settings->getExport();
                        options.movieCodec = p.movieCodecs[value];
                        p.settings->setExport(options);
                    }
                });

            p.exportButton->setClickedCallback(
                [this]
                {
                    _export();
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
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ExportTool>(new ExportTool);
            out->_init(context, app, parent);
            return out;
        }

        void ExportTool::_widgetUpdate(const models::ExportSettings& settings)
        {
            FTK_P();
            p.dirEdit->setPath(ftk::Path(settings.dir));
            p.renderSizeComboBox->setCurrentIndex(static_cast<int>(settings.renderSize));
            p.renderWidthEdit->setValue(settings.customSize.w);
            p.renderHeightEdit->setValue(settings.customSize.h);
            p.fileTypeComboBox->setCurrentIndex(static_cast<int>(settings.fileType));
            
            p.imageBaseEdit->setText(settings.imageBase);
            auto i = std::find(p.imageExts.begin(), p.imageExts.end(), settings.imageExt);
            p.imageExtComboBox->setCurrentIndex(i != p.imageExts.end() ? (i - p.imageExts.begin()) : -1);

            p.movieBaseEdit->setText(settings.movieBase);
            i = std::find(p.movieExts.begin(), p.movieExts.end(), settings.movieExt);
            p.movieExtComboBox->setCurrentIndex(i != p.movieExts.end() ? (i - p.movieExts.begin()) : -1);
            i = std::find(p.movieCodecs.begin(), p.movieCodecs.end(), settings.movieCodec);
            p.movieCodecComboBox->setCurrentIndex(i != p.movieCodecs.end() ? (i - p.movieCodecs.begin()) : -1);

            p.formLayout->setRowVisible(p.customSizeLayout, models::ExportRenderSize::Custom == settings.renderSize);
            p.formLayout->setRowVisible(
                p.imageBaseEdit,
                models::ExportFileType::Image == settings.fileType ||
                models::ExportFileType::Seq == settings.fileType);
            p.formLayout->setRowVisible(
                p.imageZeroPadEdit,
                models::ExportFileType::Image == settings.fileType ||
                models::ExportFileType::Seq == settings.fileType);
            p.formLayout->setRowVisible(
                p.imageExtComboBox,
                models::ExportFileType::Image == settings.fileType ||
                models::ExportFileType::Seq == settings.fileType);
            p.formLayout->setRowVisible(p.movieBaseEdit, models::ExportFileType::Movie == settings.fileType);
            p.formLayout->setRowVisible(p.movieExtComboBox, models::ExportFileType::Movie == settings.fileType);
            p.formLayout->setRowVisible(p.movieCodecComboBox, models::ExportFileType::Movie == settings.fileType);
        }

        void ExportTool::_export()
        {
            FTK_P();
            auto context = getContext();
            auto app = p.app.lock();
            if (app && context && p.player)
            {
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
                    switch (options.fileType)
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
                    switch (options.renderSize)
                    {
                    case models::ExportRenderSize::Default:
                        p.exportData->info.size = ioInfo.video.front().size;
                        break;
                    case models::ExportRenderSize::Custom:
                        p.exportData->info.size = options.customSize;
                        break;
                    default:
                        p.exportData->info.size = getSize(options.renderSize);
                        break;
                    }

                    // Get the export path.
                    std::string fileName;
                    switch (options.fileType)
                    {
                    case models::ExportFileType::Image:
                    case models::ExportFileType::Seq:
                    {
                        std::stringstream ss;
                        ss << options.imageBase;
                        ss << std::setfill('0') << std::setw(options.imageZeroPad) << p.exportData->range.start_time().value();
                        ss << options.imageExt;
                        fileName = ss.str();
                        break;
                    }
                    case models::ExportFileType::Movie:
                    {
                        std::stringstream ss;
                        ss << options.movieBase << options.movieExt;
                        fileName = ss.str();
                        break;
                    }
                    default: break;
                    }
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
                    tl::IOOptions ioOptions;
                    ioOptions["FFmpeg/Codec"] = options.movieCodec;
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
                        ftk::gl::offscreenColorDefault);

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
                            else
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
    }
}
