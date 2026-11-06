// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/ExportWidget.h>

#include <djv/Models/ExportRender.h>

#include <djv/Models/AnnotationsModel.h>

#include <djv/UI/ExportWidgets.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/UI/Viewport.h>
#include <tlRender/Timeline/CompareOptions.h>
#include <tlRender/Timeline/IRender.h>
#include <tlRender/Timeline/Util.h>
#include <tlRender/IO/System.h>
#if defined(TLRENDER_FFMPEG_PLUGIN)
#include <tlRender/IO/FFmpeg.h>
#endif // TLRENDER_FFMPEG_PLUGIN

#include <tlRender/Core/Audio.h>

#include <ftk/UI/CheckBox.h>
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
#include <tlRender/UI/OffscreenRender.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Timer.h>
#include <ftk/Core/Path.h>

#include <algorithm>
#include <cctype>
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

        }

        struct ExportWidget::Private
        {
            std::shared_ptr<tl::Player> player;
            std::shared_ptr<models::SettingsModel> settings;
            std::weak_ptr<models::FilesModel> filesModel;
            std::weak_ptr<models::ColorModel> colorModel;
            std::weak_ptr<models::ViewportModel> viewportModel;
            std::weak_ptr<models::AnnotationsModel> annotationsModel;

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
                //! Where the frames are rendered, as the viewport shows them;
                //! see models::ExportRender.
                std::shared_ptr<models::ExportRender> render;
            };
            std::unique_ptr<ExportData> exportData;

            std::shared_ptr<ftk::FileEdit> dirEdit;
            std::shared_ptr<ftk::ComboBox> renderSizeComboBox;
            std::shared_ptr<ftk::Label> sizeLabel;
            std::shared_ptr<ftk::IntEdit> widthEdit;
            std::shared_ptr<ftk::IntEdit> heightEdit;
            std::shared_ptr<ftk::HorizontalLayout> sizeEditLayout;
            // The edits report every change, including the ones made here
            // to show the custom size, which must not read as typed.
            bool sizeUpdating = false;
            std::shared_ptr<ftk::CheckBox> burnAnnotationsCheckBox;
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
            //! Told how an export requested by exportMovie() ended.
            std::function<void(bool)> doneCallback;
            // Set when a frame, or the writer finishing, reported an error:
            // the last frame can be written and the file still fail to
            // close, which otherwise read as an export that worked.
            bool exportFailed = false;
        };

        void ExportWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<models::AnnotationsModel>& annotationsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            ftk::IContainer::_init(context, "djv::ui::ExportWidget", parent);
            FTK_P();

            p.settings = settingsModel;
            p.annotationsModel = annotationsModel;
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
                "The presets and \"Width\" scale to a width, with the height "
                "following the aspect ratio of what is being exported, "
                "rounded to an even number for a movie, which its codecs "
                "need.\n"
                "\"Custom\" is a width and height typed in, used as they are.");
            ftk::setScreenshotTag(p.renderSizeComboBox, "Export.RenderSize");
            p.sizeLabel = ftk::Label::create(context);
            p.sizeLabel->setTooltip("The size the export comes out at.");
            ftk::setScreenshotTag(p.sizeLabel, "Export.Size");
            p.widthEdit = ftk::IntEdit::create(context);
            p.widthEdit->setRange(customSizeMin, customSizeMax);
            p.widthEdit->setTooltip(
                "The width the export comes out at, used as it is. For a "
                "custom size a movie's codecs need even numbers; the export "
                "says if the codec cannot take the size.");
            ftk::setScreenshotTag(p.widthEdit, "Export.Width");
            p.heightEdit = ftk::IntEdit::create(context);
            p.heightEdit->setRange(customSizeMin, customSizeMax);
            p.heightEdit->setTooltip(
                "The height the export comes out at, used as it is. A movie's "
                "codecs need even numbers; the export says if the codec "
                "cannot take the size.");
            ftk::setScreenshotTag(p.heightEdit, "Export.Height");
            p.burnAnnotationsCheckBox = ftk::CheckBox::create(context);
            p.burnAnnotationsCheckBox->setTooltip(
                "Write the drawings into the picture, on the frames they "
                "are on.");
            ftk::setScreenshotTag(p.burnAnnotationsCheckBox, "Export.BurnAnnotations");

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
            p.formLayout->addRow("Render size:", p.renderSizeComboBox);
            // One row showing whichever applies: the size as a label, a
            // width edit with the height it comes to, or both edits.
            p.sizeEditLayout = ftk::HorizontalLayout::create(context);
            p.sizeEditLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.widthEdit->setParent(p.sizeEditLayout);
            p.heightEdit->setParent(p.sizeEditLayout);
            p.sizeLabel->setParent(p.sizeEditLayout);
            p.sizeLabel->setVAlign(ftk::VAlign::Center);
            p.formLayout->addRow("Size:", p.sizeEditLayout);
            p.formLayout->addRow("Burn in drawings:", p.burnAnnotationsCheckBox);
            // Under both of the ways of giving a width, since it is what
            // either of them comes to, and it is worth saying for the
            // default and the presets as much as for a typed width.
            p.tabWidget = ftk::TabWidget::create(context, p.layout);
            // Tag the tab bar rather than the whole tab widget so that
            // screenshot annotations point at the tabs.
            ftk::setScreenshotTag(p.tabWidget->getTabBar(), "Export.Tabs");
            p.tabWidget->addTab("Image", p.imageWidget);
            p.tabWidget->addTab("Sequence", p.seqWidget);
            p.tabWidget->addTab("Movie", p.movieWidget);

            // The form above the tabs and the one in each tab share a label
            // width, so the controls stay put from one tab to the next.
            ftk::setFormGroup(p.layout, ftk::FormGroup::create());

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

            p.burnAnnotationsCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.burnAnnotations = value;
                    p.settings->setExport(options);
                });

            p.widthEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    if (p.sizeUpdating)
                        return;
                    auto options = p.settings->getExport();
                    options.customWidth = value;
                    p.settings->setExport(options);
                });

            p.heightEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    if (p.sizeUpdating)
                        return;
                    auto options = p.settings->getExport();
                    options.customHeight = value;
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
            const std::shared_ptr<models::AnnotationsModel>& annotationsModel,
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
                annotationsModel,
                parent);
            return out;
        }

        void ExportWidget::setPlayer(const std::shared_ptr<tl::Player>& value)
        {
            FTK_P();
            p.player = value;
            if (p.player)
            {
                // Named for what is being exported rather than carried over
                // from the last export: a name that stays behind writes each
                // file of a session over the one before it.
                if (auto context = getContext())
                {
                    const std::vector<std::string> seqExts = tl::getExts(
                        context, static_cast<int>(tl::FileType::Seq));
                    models::ExportSettings options = p.settings->getExport();
                    models::getExportNames(
                        p.player->getPath(),
                        seqExts,
                        options.imageFileName,
                        options.seqFileName);
                    options.movieFileName = options.imageFileName;
                    p.settings->setExport(options);
                }

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
            return models::getExportInfos(p.player);
        }

        ftk::Size2I ExportWidget::_getDefaultSize() const
        {
            FTK_P();
            return models::getExportDefaultSize(
                p.filesModel.lock()->getCompareOptions(),
                p.viewportModel.lock()->getDisplayOptions().aspectRatio,
                _getInfos());
        }

        namespace
        {
            // Subsampled chroma covers the pixels in pairs, so a movie's
            // size has to be even: the nearest even number, from the exact
            // value so that a half rounds to the nearer of the two, and
            // never nothing.
            int roundEven(double value)
            {
                return std::max(2, static_cast<int>(std::lround(value / 2.0)) * 2);
            }
        }

        ftk::Size2I ExportWidget::_getWidthSize(int width, bool even) const
        {
            ftk::Size2I out;
            const ftk::Size2I size = _getDefaultSize();
            if (size.isValid())
            {
                out.w = std::clamp(width, customSizeMin, customSizeMax);
                const double h = out.w * size.h / static_cast<double>(size.w);
                out.h = std::clamp(
                    even ? roundEven(h) : static_cast<int>(std::lround(h)),
                    customSizeMin,
                    customSizeMax);
            }
            return out;
        }

        ftk::Size2I ExportWidget::_getExportSize(
            const models::ExportSettings& settings) const
        {
            ftk::Size2I out;
            // A height worked out from a width is rounded for a movie's
            // codecs. The source's own size and a custom size are used as
            // they are, and the writer says if the codec cannot take them.
            const bool even = models::ExportFileType::Movie == settings.fileType;
            switch (settings.renderSize)
            {
            case models::ExportRenderSize::Source:
                out = _getDefaultSize();
                break;
            case models::ExportRenderSize::Width:
                out = _getWidthSize(settings.customWidth, even);
                break;
            case models::ExportRenderSize::Custom:
                out.w = std::clamp(settings.customWidth, customSizeMin, customSizeMax);
                out.h = std::clamp(settings.customHeight, customSizeMin, customSizeMax);
                break;
            default:
                // A preset is a width like any other: taking its height as
                // well would squash anything that is not the shape it was
                // named for, since the export scales to fill rather than
                // letterboxing.
                out = _getWidthSize(models::getWidth(settings.renderSize), even);
                break;
            }
            return out;
        }

        void ExportWidget::_sizeUpdate()
        {
            FTK_P();
            const auto settings = p.settings->getExport();
            const bool width = models::ExportRenderSize::Width == settings.renderSize;
            const bool custom = models::ExportRenderSize::Custom == settings.renderSize;
            p.widthEdit->setVisible(width || custom);
            p.heightEdit->setVisible(custom);
            p.sizeLabel->setVisible(!custom);
            // Blank until there is something to take a size from, which is
            // also when there is nothing to export. With a width typed in,
            // the label is the height it comes to.
            const ftk::Size2I size = _getExportSize(settings);
            std::string text;
            if (size.isValid())
            {
                text = width ?
                    ftk::Format("x {0}").arg(size.h).str() :
                    ftk::Format("{0} x {1}").arg(size.w).arg(size.h).str();
            }
            p.sizeLabel->setText(text);
            p.sizeUpdating = true;
            p.widthEdit->setValue(settings.customWidth);
            p.heightEdit->setValue(settings.customHeight);
            p.sizeUpdating = false;
            p.burnAnnotationsCheckBox->setChecked(settings.burnAnnotations);
        }

        void ExportWidget::_widgetUpdate(const models::ExportSettings& settings)
        {
            FTK_P();
            p.dirEdit->setPath(ftk::Path(settings.dir));
            p.renderSizeComboBox->setCurrentIndex(static_cast<int>(settings.renderSize));
            _sizeUpdate();
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

        void ExportWidget::exportMovie(
            bool overwrite,
            const std::function<void(bool)>& callback)
        {
            FTK_P();
            const auto done = [this, callback](bool value)
            {
                // Copied out first: this runs from inside the function it
                // is about to clear, and clearing it destroys what it
                // captured.
                const auto callbackCopy = callback;
                _p->doneCallback = nullptr;
                if (callbackCopy)
                {
                    callbackCopy(value);
                }
            };
            if (!p.player || p.exportData)
            {
                _error(!p.player ? "No file to export" : "An export is already running");
                done(false);
                return;
            }
            const auto options = p.settings->getExport();
            const auto fileType = models::ExportFileType::Movie;
            if (!overwrite &&
                getExportExists(options, fileType, _getExportRange(fileType)))
            {
                _error(ftk::Format("Output file already exists: \"{0}\"").
                    arg(ftk::Path(options.dir, options.movieFileName + options.movieExt).get()));
                done(false);
                return;
            }
            p.exportFailed = false;
            p.doneCallback = done;
            _exportStart(fileType);
            // A start that failed has reported already and left nothing
            // running to finish.
            if (!p.exportData && p.doneCallback)
            {
                p.doneCallback(false);
            }
        }

        void ExportWidget::_error(const std::string& value)
        {
            FTK_P();
            p.exportFailed = true;
            // Logged as well as shown: a scripted export has nobody watching
            // the dialog, and the log is where it would look.
            if (auto context = getContext())
            {
                context->log("djv::ui::ExportWidget", value, ftk::LogType::Error);
            }
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
                        ftk::toFileSystem(options.dir)))
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
                    if (!tl::ui::OffscreenRender::canRead(p.exportData->info.type))
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
                    // the work; an unknown name, or one this build cannot
                    // write, falls back to the first preset rather than
                    // exporting with nothing set.
                    tl::IOOptions ioOptions;
#if defined(TLRENDER_FFMPEG_PLUGIN)
                    const auto presets =
                        ioSystem->getPlugin<tl::ffmpeg::WritePlugin>()->getWritePresets();
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
#endif // TLRENDER_FFMPEG_PLUGIN
                    p.exportData->writer = plugin->write(p.exportData->path, outputInfo, ioOptions);

                    // Create the renderer, which renders what the viewport
                    // shows: the files being compared with the A file laid out
                    // alongside it, the color as it is managed and adjusted,
                    // and the drawings if they are burned in.
                    std::vector<std::string> sourceIds;
                    for (const auto& item : p.filesModel.lock()->getActive())
                    {
                        sourceIds.push_back(item->id);
                    }
                    p.exportData->render = models::ExportRender::create(
                        context,
                        models::getExportRenderOptions(
                            p.player,
                            p.viewportModel.lock(),
                            p.colorModel.lock(),
                            p.filesModel.lock(),
                            p.annotationsModel.lock(),
                            sourceIds,
                            p.exportData->info.size,
                            p.exportData->info.type,
                            options.burnAnnotations));

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
                            // Finished only when every frame was written;
                            // closing early is cancelling, and a failed frame
                            // closes it too.
                            const bool finished =
                                p.exportData &&
                                !p.exportFailed &&
                                p.exportData->frame > p.exportData->range.end_time_inclusive().value();
                            p.exportData.reset();
                            p.progressDialog.reset();
                            if (p.doneCallback)
                            {
                                p.doneCallback(finished);
                            }
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
                    // Nothing is left running: an export that failed to
                    // start would otherwise read as one in progress.
                    p.exportData.reset();
                    _error(e.what());
                    context->getSystem<ftk::DialogSystem>()->message(
                        "ERROR",
                        ftk::Format("Error: {0}").arg(e.what()),
                        getWindow());
                    if (p.doneCallback)
                    {
                        p.doneCallback(false);
                    }
                }
            }
        }

        bool ExportWidget::_exportFrame()
        {
            FTK_P();
            bool out = false;
            try
            {
                // Render the frame as the viewport shows it.
                const OTIO_NS::RationalTime t(p.exportData->frame, p.exportData->range.duration().rate());
                auto image = p.exportData->render->render(p.player, t);

                // Write the output image.

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
                _error(e.what());
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
