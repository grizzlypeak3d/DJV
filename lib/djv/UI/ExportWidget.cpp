// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/ExportWidget.h>

#include <djv/UI/ExportWidgets.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/GL/Render.h>
#include <tlRender/Timeline/CompareOptions.h>
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
#include <ftk/UI/Label.h>
#include <ftk/UI/ProgressDialog.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/TabBar.h>
#include <ftk/UI/TabWidget.h>
#include <ftk/GL/GL.h>
#include <ftk/GL/OffscreenBuffer.h>
#include <ftk/GL/Util.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Timer.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace djv
{
    namespace ui
    {
        namespace
        {
            const int customSizeMin = 1;
            const int customSizeMax = 16384;

            // Scale a comparison layout to the export size. The boxes come
            // out of the comparison at the size it lays out to naturally; a
            // custom or preset export size stretches that, the same as the
            // single image case does. Scaling the edges rather than the
            // origin and the size keeps neighbouring boxes touching instead
            // of leaving a seam between them.
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
        }

        struct ExportWidget::Private
        {
            std::shared_ptr<tl::Player> player;
            std::shared_ptr<models::SettingsModel> settings;
            std::weak_ptr<models::FilesModel> filesModel;
            std::weak_ptr<models::ColorModel> colorModel;
            std::weak_ptr<models::ViewportModel> viewportModel;

            struct ExportData
            {
                models::ExportFileType fileType = models::ExportFileType::Image;
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
                // One entry per video source: the A file followed by the
                // files it is being compared with. The layout is computed
                // once so that it cannot shift part way through a movie.
                std::vector<ftk::ImageOptions> imageOptions;
                std::vector<tl::DisplayOptions> displayOptions;
                tl::CompareOptions compareOptions;
                std::vector<ftk::Box2I> boxes;
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
            std::shared_ptr<ftk::Label> outputSizeLabel;
            std::shared_ptr<ImageExportWidget> imageWidget;
            std::shared_ptr<SeqExportWidget> seqWidget;
            std::shared_ptr<MovieExportWidget> movieWidget;
            std::shared_ptr<ftk::TabWidget> tabWidget;
            std::shared_ptr<ftk::FormLayout> formLayout;
            std::shared_ptr<ftk::VerticalLayout> layout;
            std::shared_ptr<ftk::ProgressDialog> progressDialog;

            std::shared_ptr<ftk::Observer<models::ExportSettings> > settingsObserver;
            // What the export size is worked out from, so that the size shown
            // for it does not go stale while the tool is open.
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<tl::Timeline> > > compareObserver;

            std::shared_ptr<ftk::Timer> progressTimer;
        };

        void ExportWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            ftk::IContainer::_init(context, "djv::ui::ExportWidget", parent);
            FTK_P();

            p.settings = settingsModel;
            p.filesModel = filesModel;
            p.colorModel = colorModel;
            p.viewportModel = viewportModel;

            p.dirEdit = ftk::FileEdit::create(context, ftk::FileBrowserMode::Dir);
            ftk::setScreenshotTag(p.dirEdit, "Export.Dir");

            p.renderSizeComboBox = ftk::ComboBox::create(context, models::getExportRenderSizeLabels());
            // "Default" is the one choice that is not a width, so it is the
            // one that needs saying.
            p.renderSizeComboBox->setTooltip(
                "\"Default\" is the size of what is being exported, with no "
                "scaling.\n"
                "The other choices scale to the width given.");
            ftk::setScreenshotTag(p.renderSizeComboBox, "Export.RenderSize");
            p.renderWidthEdit = ftk::IntEdit::create(context);
            p.renderWidthEdit->setRange(customSizeMin, customSizeMax);
            p.renderWidthEdit->setTooltip(
                "The height follows the aspect ratio of what is being "
                "exported.");
            ftk::setScreenshotTag(p.renderWidthEdit, "Export.CustomWidth");
            p.outputSizeLabel = ftk::Label::create(context);
            p.outputSizeLabel->setTooltip(
                "The size the export comes out at, which the width and the "
                "aspect ratio of what is being exported give between them.");
            ftk::setScreenshotTag(p.outputSizeLabel, "Export.OutputSize");

            p.imageWidget = ImageExportWidget::create(
                context, settingsModel);
            p.seqWidget = SeqExportWidget::create(
                context, settingsModel, timeUnitsModel);
            p.movieWidget = MovieExportWidget::create(
                context, settingsModel, timeUnitsModel);

            p.layout = ftk::VerticalLayout::create(context);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto vLayout = ftk::VerticalLayout::create(context, p.layout);
            vLayout->setMarginRole(ftk::SizeRole::Margin);
            p.formLayout = ftk::FormLayout::create(context, vLayout);
            p.formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.formLayout->addRow("Directory:", p.dirEdit);
            p.formLayout->addRow("Render width:", p.renderSizeComboBox);
            p.formLayout->addRow("Custom width:", p.renderWidthEdit);
            // Under both of the ways of giving a width, since it is what
            // either of them comes to, and it is worth saying for the
            // default and the presets as much as for a typed width.
            p.formLayout->addRow("Output size:", p.outputSizeLabel);
            p.tabWidget = ftk::TabWidget::create(context, p.layout);
            // Tag the tab bar rather than the whole tab widget so that
            // screenshot annotations point at the tabs.
            ftk::setScreenshotTag(p.tabWidget->getTabBar(), "Export.Tabs");
            p.tabWidget->addTab("Image", p.imageWidget);
            p.tabWidget->addTab("Sequence", p.seqWidget);
            p.tabWidget->addTab("Movie", p.movieWidget);

            p.layout->setHStretch(ftk::Stretch::Expanding);
            p.layout->setVStretch(ftk::Stretch::Expanding);
            _setWidget(p.layout);

            p.settingsObserver = ftk::Observer<models::ExportSettings>::create(
                p.settings->observeExport(),
                [this](const models::ExportSettings& value)
                {
                    _widgetUpdate(value);
                });

            p.compareOptionsObserver = ftk::Observer<tl::CompareOptions>::create(
                p.filesModel.lock()->observeCompareOptions(),
                [this](const tl::CompareOptions&)
                {
                    _sizeUpdate();
                });

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                p.viewportModel.lock()->observeDisplayOptions(),
                [this](const tl::DisplayOptions&)
                {
                    _sizeUpdate();
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
                    options.customWidth = value;
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

        ExportWidget::ExportWidget() :
            _p(new Private)
        {}

        ExportWidget::~ExportWidget()
        {}

        std::shared_ptr<ExportWidget> ExportWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ExportWidget>(new ExportWidget);
            out->_init(
                context,
                filesModel,
                colorModel,
                viewportModel,
                settingsModel,
                timeUnitsModel,
                parent);
            return out;
        }

        void ExportWidget::setPlayer(const std::shared_ptr<tl::Player>& value)
        {
            FTK_P();
            p.player = value;
            if (p.player)
            {
                // The comparison the player is running, which is not the
                // same news as the compare options. Turning a comparison on
                // with no "B" file chosen picks one, so the options change
                // first and the timeline that goes with them second;
                // without this the size would be worked out from the "A"
                // file alone and left there.
                p.compareObserver =
                    ftk::ListObserver<std::shared_ptr<tl::Timeline> >::create(
                        p.player->observeCompare(),
                        [this](const std::vector<std::shared_ptr<tl::Timeline> >&)
                        {
                            _sizeUpdate();
                        });
            }
            else
            {
                p.compareObserver.reset();
                _sizeUpdate();
            }
            p.imageWidget->setPlayer(value);
            p.seqWidget->setPlayer(value);
            p.movieWidget->setPlayer(value);
        }

        std::vector<ftk::ImageInfo> ExportWidget::_getInfos() const
        {
            FTK_P();
            std::vector<ftk::ImageInfo> out;
            if (p.player)
            {
                const tl::IOInfo& ioInfo = p.player->getIOInfo();
                if (!ioInfo.video.empty())
                {
                    out.push_back(ioInfo.video.front());
                    for (const auto& compare : p.player->getCompare())
                    {
                        // A source without video still takes a place in the
                        // layout, so that the sources stay lined up with the
                        // frames requested for them.
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

        ftk::Size2I ExportWidget::_getDefaultSize() const
        {
            FTK_P();
            ftk::Size2I out;
            {
                const std::vector<ftk::ImageInfo> infos = _getInfos();
                const tl::AspectRatioOptions& aspectRatio =
                    p.viewportModel.lock()->getDisplayOptions().aspectRatio;
                out = tl::getRenderSize(
                    p.filesModel.lock()->getCompareOptions(),
                    aspectRatio,
                    infos);
                // The B comparison sizes itself from the B file, which there
                // need not be one of yet. Fall back to the A file so that the
                // export still has a size to work with; it renders the same
                // empty picture the viewport shows in the meantime.
                if (!out.isValid() && !infos.empty())
                {
                    out = tl::getRenderSize(infos.front(), aspectRatio);
                }
            }
            return out;
        }

        ftk::Size2I ExportWidget::_getWidthSize(int width) const
        {
            ftk::Size2I out;
            const ftk::Size2I size = _getDefaultSize();
            if (size.isValid())
            {
                out.w = std::clamp(width, customSizeMin, customSizeMax);
                out.h = std::clamp(
                    static_cast<int>(std::lround(
                        out.w * size.h / static_cast<double>(size.w))),
                    customSizeMin,
                    customSizeMax);
            }
            return out;
        }

        ftk::Size2I ExportWidget::_getExportSize(
            const models::ExportSettings& settings) const
        {
            ftk::Size2I out;
            switch (settings.renderSize)
            {
            case models::ExportRenderSize::Default:
                out = _getDefaultSize();
                break;
            case models::ExportRenderSize::Custom:
                out = _getWidthSize(settings.customWidth);
                break;
            default:
                // A preset is a width like any other: taking its height as
                // well would squash anything that is not the shape it was
                // named for, since the export scales to fill rather than
                // letterboxing.
                out = _getWidthSize(models::getWidth(settings.renderSize));
                break;
            }
            return out;
        }

        void ExportWidget::_sizeUpdate()
        {
            FTK_P();
            // Blank until there is something to take an aspect ratio from,
            // which is also when there is nothing to export.
            const ftk::Size2I size = _getExportSize(p.settings->getExport());
            p.outputSizeLabel->setText(size.isValid() ?
                ftk::Format("{0} x {1}").arg(size.w).arg(size.h).str() :
                std::string());
        }

        void ExportWidget::_widgetUpdate(const models::ExportSettings& settings)
        {
            FTK_P();
            p.dirEdit->setPath(ftk::Path(settings.dir));
            p.renderSizeComboBox->setCurrentIndex(static_cast<int>(settings.renderSize));
            p.renderWidthEdit->setValue(settings.customWidth);
            _sizeUpdate();
            p.formLayout->setRowVisible(
                p.renderWidthEdit,
                models::ExportRenderSize::Custom == settings.renderSize);
            p.tabWidget->setCurrent(static_cast<int>(settings.fileType));
        }

        OTIO_NS::TimeRange ExportWidget::_getExportRange(
            models::ExportFileType fileType) const
        {
            FTK_P();
            OTIO_NS::TimeRange out;
            if (p.player)
            {
                switch (fileType)
                {
                case models::ExportFileType::Image:
                    out = OTIO_NS::TimeRange(
                        p.player->getCurrentTime(),
                        OTIO_NS::RationalTime(1.0, p.player->getTimeRange().duration().rate()));
                    break;
                default:
                    out = p.player->getInOutRange();
                    break;
                }
            }
            return out;
        }

        void ExportWidget::_export(models::ExportFileType fileType)
        {
            FTK_P();
            if (!p.player)
                return;
            // Ask before writing over anything. The file names are worked out
            // from a base name and the frame numbers, so the same export runs
            // twice write the same files: overwriting is easy to do without
            // meaning to, and a rendered sequence is expensive to lose.
            const auto options = p.settings->getExport();
            const OTIO_NS::TimeRange range = _getExportRange(fileType);
            if (getExportExists(options, fileType, range))
            {
                // The names are in the tool, a click away from the button
                // that opened this, so the state of things is all the dialog
                // has to carry. A sequence is more than one file.
                //
                // The button says what it does rather than answering a
                // question the text asks, so that agreeing to it does not
                // depend on having read the text: "(exists)" beside the file
                // name was missed for the same reason a "Yes" would be.
                const std::string text =
                    models::ExportFileType::Seq == fileType ?
                    "Output files already exist; overwrite?" :
                    "Output file already exists; overwrite?";
                if (auto context = getContext())
                {
                    context->getSystem<ftk::DialogSystem>()->confirm(
                        "Export",
                        text,
                        getWindow(),
                        [this, fileType](bool value)
                        {
                            if (value)
                            {
                                _exportStart(fileType);
                            }
                        },
                        "Overwrite");
                }
                return;
            }
            _exportStart(fileType);
        }

        void ExportWidget::_exportStart(models::ExportFileType fileType)
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
                    p.exportData->fileType = fileType;

                    // Get the time range.
                    const auto options = p.settings->getExport();
                    p.exportData->range = _getExportRange(fileType);
                    p.exportData->frame = p.exportData->range.start_time().value();

                    // Get the video sources. The export renders what the
                    // viewport shows, so the files being compared with the A
                    // file are laid out alongside it rather than dropped.
                    const auto& displayOptions = p.viewportModel.lock()->getDisplayOptions();
                    p.exportData->compareOptions =
                        p.filesModel.lock()->getCompareOptions();
                    const std::vector<ftk::ImageInfo> infos = _getInfos();
                    const ftk::Size2I compareSize = _getDefaultSize();

                    // Get the render size.
                    p.exportData->info.size = _getExportSize(options);

                    // Check the output directory before anything is rendered,
                    // rather than letting each writer report it in its own way.
                    // An empty one is an error and not an implicit write to
                    // wherever the application happens to be running: the field
                    // is filled in with a real directory, so clearing it is
                    // something the user did.
                    if (options.dir.empty())
                    {
                        throw std::runtime_error("No export directory");
                    }
                    if (!std::filesystem::is_directory(
                        std::filesystem::u8path(options.dir)))
                    {
                        throw std::runtime_error(
                            ftk::Format("Directory not found: \"{0}\"").
                            arg(options.dir).str());
                    }

                    // Get the export path.
                    const std::string fileName = getExportFileName(
                        options,
                        fileType,
                        static_cast<int64_t>(
                            p.exportData->range.start_time().value()));
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
                    p.exportData->boxes = scaleBoxes(
                        tl::getBoxes(
                            p.exportData->compareOptions,
                            displayOptions.aspectRatio,
                            infos),
                        compareSize,
                        p.exportData->info.size);
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
                    if (models::ExportFileType::Movie == fileType)
                    {
                        // A movie's frames start at zero, so where it came
                        // from in the timeline is only recoverable from the
                        // start timecode. Rates that have no timecode of
                        // their own are left without one rather than given a
                        // wrong one.
                        try
                        {
                            outputInfo.tags["timecode"] =
                                p.exportData->range.start_time().to_timecode();
                        }
                        catch (const std::exception&)
                        {}
                    }
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

                    // What the output pixels are. The export bakes the
                    // display transform, so the color description written
                    // is the display's; an unrecognized display writes
                    // nothing rather than guessing. Without color
                    // management the source pixels pass through, and the
                    // source's description with them.
                    const tl::OCIOOptions ocioOptions =
                        p.colorModel.lock()->getOCIOOptions();
                    if (ocioOptions.enabled &&
                        !ocioOptions.display.empty() &&
                        !ocioOptions.view.empty())
                    {
                        const ftk::ImageTags colorTags =
                            tl::getDisplayColorTags(
                                ocioOptions,
                                models::ExportFileType::Movie != fileType);
                        outputInfo.tags.insert(
                            colorTags.begin(),
                            colorTags.end());
                    }
                    else
                    {
                        for (const auto& tag :
                            { "Color Primaries", "Color Transfer", "Chromaticities" })
                        {
                            const auto i = ioInfo.tags.find(tag);
                            if (i != ioInfo.tags.end())
                            {
                                outputInfo.tags[tag] = i->second;
                            }
                        }
                    }

                    // The preset carries the options and which writer does
                    // the work; an unknown name falls back to the first
                    // preset rather than exporting with nothing set.
                    tl::IOOptions ioOptions;
                    const auto& presets = tl::ffmpeg::getWritePresets();
                    const tl::ffmpeg::WritePreset* preset = &presets.front();
                    for (const auto& i : presets)
                    {
                        if (i.name == options.moviePreset)
                        {
                            preset = &i;
                            break;
                        }
                    }
                    if (preset->command)
                    {
                        // The settings' paths, so a custom ffmpeg is
                        // honoured.
                        ioOptions = p.settings->getIOOptions();
                    }
                    for (const auto& i : preset->options)
                    {
                        ioOptions[i.first] = i.second;
                    }
                    if (!preset->command &&
                        !options.movieAudioCodec.empty() &&
                        options.movieAudioCodec != "Auto")
                    {
                        ioOptions["FFmpeg/AudioCodec"] = options.movieAudioCodec;
                    }
                    p.exportData->writer = plugin->write(p.exportData->path, outputInfo, ioOptions);

                    // Create the renderer.
                    p.exportData->ocioOptions = ocioOptions;
                    p.exportData->lutOptions = p.colorModel.lock()->getLUTOptions();
                    p.exportData->imageOptions = std::vector<ftk::ImageOptions>(
                        infos.size(),
                        p.viewportModel.lock()->getImageOptions());
                    p.exportData->displayOptions = std::vector<tl::DisplayOptions>(
                        infos.size(),
                        displayOptions);
                    // Each file's resolved input color space, the same as
                    // the viewport, so the export bakes what the viewport
                    // shows.
                    const auto resolvedInputs =
                        p.colorModel.lock()->observeResolvedInputs()->get();
                    for (size_t i = 0;
                        i < p.exportData->displayOptions.size() &&
                            i < resolvedInputs.size();
                        ++i)
                    {
                        p.exportData->displayOptions[i].ocioInput = resolvedInputs[i];
                    }
                    p.exportData->colorBuffer = p.viewportModel.lock()->getColorBuffer();
                    p.exportData->render = tl::gl::Render::create(
                        context->getLogSystem(),
                        context->getSystem<ftk::FontSystem>());
                    {
                        // The same per layer resolution as the viewport, so
                        // the export bakes what the viewport shows.
                        const auto colorModel = p.colorModel.lock();
                        p.exportData->render->setOCIOInputResolver(
                            [colorModel](const std::string& path, const ftk::ImageTags& tags)
                            {
                                return colorModel->getOCIOOptions().input.empty() ?
                                    colorModel->resolveInput(path, tags) :
                                    std::string();
                            });
                    }
                    ftk::gl::OffscreenBufferOptions offscreenBufferOptions;
                    // The wipe comparison masks with the stencil buffer, so
                    // the buffer needs one. Paired with depth, as the
                    // viewport does, for the combined format rather than a
                    // stencil-only attachment.
#if defined(FTK_API_GL_4_1)
                    offscreenBufferOptions.depth = ftk::gl::OffscreenDepth::_24;
                    offscreenBufferOptions.stencil = ftk::gl::OffscreenStencil::_8;
#elif defined(FTK_API_GLES_3)
                    offscreenBufferOptions.stencil = ftk::gl::OffscreenStencil::_8;
#endif // FTK_API_GL_4_1
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

        bool ExportWidget::_exportFrame()
        {
            FTK_P();
            bool out = false;
            try
            {
                // Get the video for the A file and each of the files it is
                // being compared with. The requests are all made before any
                // of them is waited on so that the sources are read in
                // parallel.
                const OTIO_NS::RationalTime t(p.exportData->frame, p.exportData->range.duration().rate());
                auto ioOptions = p.player->getTimeline()->getOptions().ioOptions;
                ioOptions["Layer"] = ftk::Format("{0}").arg(p.player->getVideoLayer());
                std::vector<tl::VideoRequest> requests;
                requests.push_back(p.player->getTimeline()->getVideo(t, ioOptions));
                const auto& compare = p.player->getCompare();
                const auto& compareVideoLayers = p.player->getCompareVideoLayers();
                for (size_t i = 0; i < compare.size(); ++i)
                {
                    // The same time mapping the player uses, so that the
                    // frame exported for each source is the frame that was
                    // on screen.
                    const OTIO_NS::RationalTime compareTime = tl::getCompareTime(
                        t,
                        p.player->getTimeRange(),
                        compare[i]->getTimeRange(),
                        p.player->getCompareTime());
                    ioOptions["Layer"] = ftk::Format("{0}").arg(
                        i < compareVideoLayers.size() ?
                        compareVideoLayers[i] :
                        p.player->getVideoLayer());
                    requests.push_back(compare[i]->getVideo(compareTime, ioOptions));
                }
                std::vector<tl::VideoFrame> videoFrame;
                for (auto& request : requests)
                {
                    videoFrame.push_back(request.future.get());
                }

                // Render the video.
                ftk::gl::OffscreenBufferBinding binding(p.exportData->buffer);
                p.exportData->render->begin(p.exportData->info.size);
                p.exportData->render->setOCIOOptions(p.exportData->ocioOptions);
                p.exportData->render->setLUTOptions(p.exportData->lutOptions);
                p.exportData->render->drawVideo(
                    videoFrame,
                    p.exportData->boxes,
                    p.exportData->imageOptions,
                    p.exportData->displayOptions,
                    p.exportData->compareOptions,
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

                // The sequence writers name each file from the time it is
                // written at, so those keep the frame numbers of the timeline
                // -- which is what the file name shown in the tool promises.
                // A movie has no frame numbers in its name and the time
                // becomes the presentation timestamp, so it starts at zero.
                const int64_t start = p.exportData->range.start_time().value();
                const double speed = p.player->getSpeed();
                const OTIO_NS::RationalTime t2(
                    models::ExportFileType::Movie == p.exportData->fileType ?
                        p.exportData->frame - start :
                        p.exportData->frame,
                    speed);
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

        void ExportWidget::_exportAudio()
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
