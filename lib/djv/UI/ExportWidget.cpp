// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/ExportWidget.h>

#include <djv/Models/ExportQueue.h>

#include <djv/Models/AnnotationsModel.h>

#include <djv/UI/ExportQueueWidget.h>
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

#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/FileEdit.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/IntEdit.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/StackLayout.h>
#include <ftk/UI/TabBar.h>
#include <tlRender/UI/OffscreenRender.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Path.h>

#include <algorithm>
#include <cctype>
#include <cmath>
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
            std::weak_ptr<models::ExportQueue> queue;

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
            std::shared_ptr<ftk::TabBar> typeTabBar;
            std::shared_ptr<ImageExportWidget> imageWidget;
            std::shared_ptr<SeqExportWidget> seqWidget;
            std::shared_ptr<MovieExportWidget> movieWidget;
            std::shared_ptr<ftk::StackLayout> typeLayout;
            std::shared_ptr<ExportQueueWidget> queueWidget;
            std::shared_ptr<ftk::FormLayout> formLayout;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<models::ExportSettings> > settingsObserver;
            // What the export size is worked out from, so that the size shown
            // for it does not go stale while the tool is open.
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<ftk::Observer<std::optional<models::StoredFrame> > > frameStoreObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<tl::Timeline> > > compareObserver;

            //! Told how an export requested by exportMovie() ended; handed
            //! to the queue with it.
            std::function<void(bool)> doneCallback;
        };

        void ExportWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<models::AnnotationsModel>& annotationsModel,
            const std::shared_ptr<models::ExportQueue>& queue,
            const std::shared_ptr<IWidget>& parent)
        {
            ftk::IContainer::_init(context, "djv::ui::ExportWidget", parent);
            FTK_P();

            p.queue = queue;
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

            // What kind of export, as tabs across the top: everything
            // under them is that kind's, the shared settings included.
            p.typeTabBar = ftk::TabBar::create(context);
            int tab = 0;
            for (const auto& i : std::vector<std::pair<std::string, std::string> >{
                { "Image", "The current frame, as an image." },
                { "Sequence", "The frames between the in and out points, as an image sequence." },
                { "Movie", "The frames between the in and out points, as a movie." } })
            {
                p.typeTabBar->addTab(i.first);
                p.typeTabBar->setTabTooltip(tab++, i.second);
            }
            ftk::setScreenshotTag(p.typeTabBar, "Export.Type");

            p.imageWidget = ImageExportWidget::create(
                context, settingsModel);
            p.seqWidget = SeqExportWidget::create(
                context, settingsModel, timeUnitsModel);
            p.movieWidget = MovieExportWidget::create(
                context, settingsModel, timeUnitsModel);

            p.layout = ftk::VerticalLayout::create(context);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            // Tabs over a divider, as a tab widget has them: the tool's
            // background goes on under them, so without it they do not
            // read as heading what is below.
            auto tabsLayout = ftk::VerticalLayout::create(context, p.layout);
            tabsLayout->setSpacingRole(ftk::SizeRole::None);
            p.typeTabBar->setParent(tabsLayout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, tabsLayout);
            auto vLayout = ftk::VerticalLayout::create(context, p.layout);
            vLayout->setMarginRole(ftk::SizeRole::Margin);
            vLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
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
            // Only the chosen kind's rows, as tall as they are: the kinds
            // differ, and sized to the largest the shorter ones left a gap
            // above the queue. In the same margin as the rows above, so
            // that it all reads as one form.
            p.typeLayout = ftk::StackLayout::create(context, vLayout);
            p.typeLayout->setFitAll(false);
            p.imageWidget->setParent(p.typeLayout);
            p.seqWidget->setParent(p.typeLayout);
            p.movieWidget->setParent(p.typeLayout);

            // Under the settings, the exports they made, which run on while
            // the work goes on.
            p.queueWidget = ExportQueueWidget::create(context, queue, vLayout);

            // The form above and the chosen kind's share a label width, so
            // the controls line up and stay put from one kind to the next.
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

            // A stored frame's size takes the compared files' place.
            p.frameStoreObserver = ftk::Observer<std::optional<models::StoredFrame> >::create(
                p.filesModel.lock()->observeFrameStore(),
                [this](const std::optional<models::StoredFrame>&)
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

            p.typeTabBar->setCallback(
                [this](int index)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.fileType = static_cast<models::ExportFileType>(index);
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
            const std::shared_ptr<models::ExportQueue>& queue,
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
                queue,
                parent);
            return out;
        }

        void ExportWidget::setOpenCallback(const std::function<void(const ftk::Path&)>& value)
        {
            _p->queueWidget->setOpenCallback(value);
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
            auto filesModel = p.filesModel.lock();
            return models::getExportInfos(
                p.player,
                filesModel ? filesModel->getCompareFrameStore() : std::nullopt);
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
            p.typeTabBar->setCurrent(static_cast<int>(settings.fileType));
            p.typeLayout->setCurrentIndex(static_cast<int>(settings.fileType));
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
            if (!p.player)
            {
                _error("No file to export");
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
            // Handed to the queue with the export, or told at once that it
            // could not be queued.
            p.doneCallback = done;
            _exportStart(fileType);
        }

        void ExportWidget::_error(const std::string& value)
        {
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
            auto context = getContext();
            auto queue = p.queue.lock();
            if (!p.player || !context || !queue)
                return;
            try
            {
                const tl::IOInfo ioInfo = p.player->getIOInfo();
                if (ioInfo.video.empty())
                {
                    throw std::runtime_error("No video to render");
                }
                models::ExportJobOptions job;
                job.fileType = fileType;
                job.sourceName = p.player->getPath().getFileName();

                // Get the time range.
                const auto options = p.settings->getExport();
                job.range = _getExportRange(fileType);

                // Get the render size.
                ftk::ImageInfo info;
                info.size = _getExportSize(options);

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
                    static_cast<int64_t>(job.range.start_time().value()));
                job.path = ftk::Path(options.dir, fileName);
                job.name = fileName;
                // Two exports to one file would write over each other as
                // they went.
                if (queue->isWriting(job.path))
                {
                    throw std::runtime_error(
                        ftk::Format("Already being exported: \"{0}\"").
                        arg(job.path.get()).str());
                }

                // The writer's image type, nearest the source's.
                auto ioSystem = context->getSystem<tl::WriteSystem>();
                auto plugin = ioSystem->getPlugin(job.path);
                if (!plugin)
                {
                    throw std::runtime_error(
                        ftk::Format("Cannot open: \"{0}\"").arg(job.path.get()));
                }
                info.type = ioInfo.video.front().type;
                info = plugin->getInfo(info);
                if (ftk::ImageType::None == info.type)
                {
                    info.type = ftk::ImageType::RGBA_U8;
                }
                if (!tl::ui::OffscreenRender::canRead(info.type))
                {
                    throw std::runtime_error(
                        ftk::Format("Cannot open: \"{0}\"").arg(job.path.get()));
                }
                const double speed = p.player->getSpeed();
                job.outputInfo.video.push_back(info);
                job.outputInfo.videoTime = OTIO_NS::TimeRange(
                    OTIO_NS::RationalTime(0.0, speed),
                    job.range.duration().rescaled_to(speed));
                if (models::ExportFileType::Movie == fileType)
                {
                    // A movie's frames start at zero, so where it came
                    // from in the timeline is only recoverable from the
                    // start timecode. Rates that have no timecode of
                    // their own are left without one rather than given a
                    // wrong one.
                    try
                    {
                        job.outputInfo.tags["timecode"] =
                            job.range.start_time().to_timecode();
                    }
                    catch (const std::exception&)
                    {}
                }
#if defined(TLRENDER_FFMPEG_PLUGIN)
                if (models::ExportFileType::Movie == fileType &&
                    ioInfo.audio.isValid() &&
                    std::dynamic_pointer_cast<tl::ffmpeg::WritePlugin>(plugin))
                {
                    job.hasAudio = true;
                    job.outputInfo.audio = ioInfo.audio;
                    job.outputInfo.audioTime = OTIO_NS::TimeRange(
                        OTIO_NS::RationalTime(0.0, ioInfo.audio.sampleRate),
                        job.range.duration().rescaled_to(ioInfo.audio.sampleRate));
                    job.audioStartSeconds = job.range.start_time().rescaled_to(1.0).value();
                    job.audioDurationSeconds = job.range.duration().rescaled_to(1.0).value();
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
                    job.outputInfo.tags.insert(
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
                            job.outputInfo.tags[tag] = i->second;
                        }
                    }
                }

                // The preset carries the options and which writer does
                // the work; an unknown name, or one this build cannot
                // write, falls back to the first preset rather than
                // exporting with nothing set.
#if defined(TLRENDER_FFMPEG_PLUGIN)
                const auto presets =
                    ioSystem->getPlugin<tl::ffmpeg::WritePlugin>()->getWritePresets(
                        p.settings->getIOOptions());
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
                    job.ioOptions = p.settings->getIOOptions();
                }
                for (const auto& i : preset->options)
                {
                    job.ioOptions[i.first] = i.second;
                }
                if (!preset->command &&
                    !options.movieAudioCodec.empty() &&
                    options.movieAudioCodec != "Auto")
                {
                    job.ioOptions["FFmpeg/AudioCodec"] = options.movieAudioCodec;
                }
#endif // TLRENDER_FFMPEG_PLUGIN

                // What is rendered: what the viewport shows, the files
                // being compared with the A file laid out alongside it, the
                // color as it is managed and adjusted, and the drawings if
                // they are burned in. All of it taken now, so that what is
                // changed while the export waits or runs is not in it.
                std::vector<std::string> sourceIds;
                for (const auto& item : p.filesModel.lock()->getActive())
                {
                    sourceIds.push_back(item->id);
                }
                job.sources = models::getExportSources(p.player);
                job.render = models::getExportRenderOptions(
                    p.player,
                    p.viewportModel.lock(),
                    p.colorModel.lock(),
                    p.filesModel.lock(),
                    p.annotationsModel.lock(),
                    sourceIds,
                    info.size,
                    info.type,
                    options.burnAnnotations);

                // Told how it ended, for a script that asked for it. A
                // failure is not put in a dialog: it can come long after
                // the click, in the middle of something else. The export's
                // row says so, and the log, which the status bar shows.
                const auto callback = p.doneCallback;
                p.doneCallback = nullptr;
                queue->add(job, callback);
            }
            catch (const std::exception& e)
            {
                _error(e.what());
                context->getSystem<ftk::DialogSystem>()->message(
                    "ERROR",
                    ftk::Format("Error: {0}").arg(e.what()),
                    getWindow());
                if (p.doneCallback)
                {
                    const auto callback = p.doneCallback;
                    p.doneCallback = nullptr;
                    callback(false);
                }
            }
        }
    }
}
