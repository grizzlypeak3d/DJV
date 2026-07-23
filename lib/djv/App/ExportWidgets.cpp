// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ExportWidgets.h>

#include <djv/App/App.h>
#include <djv/Models/TimeUnitsModel.h>

#include <tlRender/Timeline/Player.h>

#include <tlRender/IO/System.h>
#if defined(TLRENDER_FFMPEG_PLUGIN)
#include <tlRender/IO/FFmpeg.h>
#endif // TLRENDER_FFMPEG_PLUGIN

#include <ftk/UI/ComboBox.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/IntEdit.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/LineEdit.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/Core/Format.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>

namespace djv
{
    namespace app
    {
        namespace
        {
            const std::vector<std::string> imageExts =
            {
                ".exr",
                ".png",
                ".tif",
                ".tiff",
                ".jpg",
                ".jpeg"
            };

            const std::vector<std::string> movieExts =
            {
                ".mov",
                ".mp4",
                ".m4v"
            };

            const std::vector<std::string> movieAudioCodecs =
            {
                "aac",
                "ac3",
                "alac",
                "flac",
                "opus",
                "pcm_s16le",
                "pcm_s24le",
                "pcm_f32le"
            };

            std::vector<std::string> getImageExts(const std::shared_ptr<ftk::Context>& context)
            {
                std::vector<std::string> out;
                auto ioSystem = context->getSystem<tl::WriteSystem>();
                for (const auto& ext : ioSystem->getExts(static_cast<int>(tl::FileType::Seq)))
                {
                    if (std::find(imageExts.begin(), imageExts.end(), ext) != imageExts.end())
                    {
                        out.push_back(ext);
                    }
                }
                return out;
            }
        }

        std::string getExportFileName(
            const models::ExportSettings& options,
            models::ExportFileType fileType,
            double frame)
        {
            std::string out;
            switch (fileType)
            {
            case models::ExportFileType::Image:
            {
                std::stringstream ss;
                ss << options.imageBase;
                ss << std::setfill('0') << std::setw(options.imageZeroPad) << frame;
                ss << options.imageExt;
                out = ss.str();
                break;
            }
            case models::ExportFileType::Seq:
            {
                std::stringstream ss;
                ss << options.seqBase;
                ss << std::setfill('0') << std::setw(options.seqZeroPad) << frame;
                ss << options.seqExt;
                out = ss.str();
                break;
            }
            case models::ExportFileType::Movie:
            {
                std::stringstream ss;
                ss << options.movieBase << options.movieExt;
                out = ss.str();
                break;
            }
            default: break;
            }
            return out;
        }

        namespace
        {
            // Check whether any frame of the sequence exists on disk.
            bool getSeqExists(
                const models::ExportSettings& options,
                const OTIO_NS::TimeRange& range)
            {
                bool out = false;
                const int64_t start = range.start_time().value();
                const int64_t end = range.end_time_inclusive().value();
                std::error_code ec;
                for (const auto& entry :
                    std::filesystem::directory_iterator(std::filesystem::path(options.dir), ec))
                {
                    const std::string fileName = entry.path().filename().string();
                    if (fileName.size() > options.seqBase.size() + options.seqExt.size() &&
                        0 == fileName.compare(0, options.seqBase.size(), options.seqBase) &&
                        0 == fileName.compare(
                            fileName.size() - options.seqExt.size(),
                            options.seqExt.size(),
                            options.seqExt))
                    {
                        const std::string digits = fileName.substr(
                            options.seqBase.size(),
                            fileName.size() - options.seqBase.size() - options.seqExt.size());
                        const bool isDigits = !digits.empty() && std::all_of(
                            digits.begin(),
                            digits.end(),
                            [](unsigned char c) { return std::isdigit(c); });
                        if (isDigits)
                        {
                            const int64_t frame = std::atoll(digits.c_str());
                            if (frame >= start &&
                                frame <= end &&
                                fileName == getExportFileName(
                                    options,
                                    models::ExportFileType::Seq,
                                    frame))
                            {
                                out = true;
                                break;
                            }
                        }
                    }
                }
                return out;
            }

            std::string getRangeText(
                const OTIO_NS::TimeRange& range,
                const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel)
            {
                return ftk::Format("{0} - {1} ({2} frames @ {3})").
                    arg(timeUnitsModel->getLabel(range.start_time())).
                    arg(timeUnitsModel->getLabel(range.end_time_inclusive())).
                    arg(static_cast<int64_t>(range.duration().value())).
                    arg(range.duration().rate(), 2);
            }
        }

        IExportWidget::~IExportWidget()
        {}

        struct ImageExportWidget::Private
        {
            std::shared_ptr<tl::Player> player;
            std::shared_ptr<models::SettingsModel> settings;
            std::vector<std::string> exts;

            std::shared_ptr<ftk::LineEdit> baseEdit;
            std::shared_ptr<ftk::IntEdit> zeroPadEdit;
            std::shared_ptr<ftk::ComboBox> extComboBox;
            std::shared_ptr<ftk::Label> fileLabel;
            std::shared_ptr<ftk::Label> existsLabel;
            std::shared_ptr<ftk::PushButton> exportButton;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<models::ExportSettings> > settingsObserver;
            std::shared_ptr<ftk::Observer<OTIO_NS::RationalTime> > currentTimeObserver;
        };

        void ImageExportWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IExportWidget::_init(context, "djv::app::ImageExportWidget", parent);
            FTK_P();

            p.settings = app->getSettingsModel();
            p.exts = getImageExts(context);

            p.baseEdit = ftk::LineEdit::create(context);
            p.baseEdit->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.baseEdit, "Export.ImageBaseName");
            p.zeroPadEdit = ftk::IntEdit::create(context);
            p.zeroPadEdit->setRange(0, 16);
            ftk::setScreenshotTag(p.zeroPadEdit, "Export.ImageZeroPad");
            p.extComboBox = ftk::ComboBox::create(context, p.exts);
            p.extComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.extComboBox, "Export.ImageExt");

            p.fileLabel = ftk::Label::create(context);
            p.existsLabel = ftk::Label::create(context, "(exists)");
            p.existsLabel->setTextRole(ftk::ColorRole::Yellow);

            p.exportButton = ftk::PushButton::create(context, "Export Image");
            ftk::setScreenshotTag(p.exportButton, "Export.ImageExport");

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto formLayout = ftk::FormLayout::create(context, p.layout);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Base name:", p.baseEdit);
            formLayout->addRow("Zero padding:", p.zeroPadEdit);
            formLayout->addRow("Extension:", p.extComboBox);
            auto fileLayout = ftk::HorizontalLayout::create(context);
            fileLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            ftk::setScreenshotTag(fileLayout, "Export.ImageFile");
            p.fileLabel->setParent(fileLayout);
            p.existsLabel->setParent(fileLayout);
            formLayout->addRow("File:", fileLayout);
            p.layout->addSpacer(ftk::SizeRole::Spacing);
            p.exportButton->setParent(p.layout);

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    FTK_P();
                    p.player = value;
                    p.exportButton->setEnabled(value.get());
                    if (value)
                    {
                        p.currentTimeObserver = ftk::Observer<OTIO_NS::RationalTime>::create(
                            value->observeCurrentTime(),
                            [this](const OTIO_NS::RationalTime&)
                            {
                                _infoUpdate();
                            });
                    }
                    else
                    {
                        p.currentTimeObserver.reset();
                        _infoUpdate();
                    }
                });

            p.settingsObserver = ftk::Observer<models::ExportSettings>::create(
                p.settings->observeExport(),
                [this](const models::ExportSettings& value)
                {
                    FTK_P();
                    p.baseEdit->setText(value.imageBase);
                    p.zeroPadEdit->setValue(value.imageZeroPad);
                    auto i = std::find(p.exts.begin(), p.exts.end(), value.imageExt);
                    p.extComboBox->setCurrentIndex(i != p.exts.end() ? (i - p.exts.begin()) : -1);
                    _infoUpdate();
                });

            p.baseEdit->setCallback(
                [this](const std::string& value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.imageBase = value;
                    p.settings->setExport(options);
                });

            p.zeroPadEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.imageZeroPad = value;
                    p.settings->setExport(options);
                });

            p.extComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    if (value >= 0 && value < static_cast<int>(p.exts.size()))
                    {
                        auto options = p.settings->getExport();
                        options.imageExt = p.exts[value];
                        p.settings->setExport(options);
                    }
                });
        }

        ImageExportWidget::ImageExportWidget() :
            _p(new Private)
        {}

        ImageExportWidget::~ImageExportWidget()
        {}

        std::shared_ptr<ImageExportWidget> ImageExportWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ImageExportWidget>(new ImageExportWidget);
            out->_init(context, app, parent);
            return out;
        }

        void ImageExportWidget::setExportCallback(const std::function<void(void)>& value)
        {
            _p->exportButton->setClickedCallback(value);
        }

        ftk::Size2I ImageExportWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void ImageExportWidget::setGeometry(const ftk::Box2I& value)
        {
            IExportWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void ImageExportWidget::_infoUpdate()
        {
            FTK_P();
            std::string fileText = "-";
            bool exists = false;
            if (p.player)
            {
                const auto options = p.settings->getExport();
                const OTIO_NS::RationalTime time = p.player->getCurrentTime();
                const std::string fileName = getExportFileName(
                    options,
                    models::ExportFileType::Image,
                    time.value());
                if (!fileName.empty())
                {
                    fileText = fileName;
                    exists = std::filesystem::exists(std::filesystem::path(
                        ftk::Path(options.dir, fileName).get()));
                }
            }
            p.fileLabel->setText(fileText);
            p.existsLabel->setVisible(exists);
        }

        struct SeqExportWidget::Private
        {
            std::shared_ptr<tl::Player> player;
            std::shared_ptr<models::SettingsModel> settings;
            std::shared_ptr<models::TimeUnitsModel> timeUnitsModel;
            std::vector<std::string> exts;

            std::shared_ptr<ftk::LineEdit> baseEdit;
            std::shared_ptr<ftk::IntEdit> zeroPadEdit;
            std::shared_ptr<ftk::ComboBox> extComboBox;
            std::shared_ptr<ftk::Label> fileLabel;
            std::shared_ptr<ftk::Label> existsLabel;
            std::shared_ptr<ftk::Label> rangeLabel;
            std::shared_ptr<ftk::PushButton> exportButton;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<models::ExportSettings> > settingsObserver;
            std::shared_ptr<ftk::Observer<OTIO_NS::TimeRange> > inOutRangeObserver;
            std::shared_ptr<ftk::Observer<tl::TimeUnits> > timeUnitsObserver;
        };

        void SeqExportWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IExportWidget::_init(context, "djv::app::SeqExportWidget", parent);
            FTK_P();

            p.settings = app->getSettingsModel();
            p.timeUnitsModel = app->getTimeUnitsModel();
            p.exts = getImageExts(context);

            p.baseEdit = ftk::LineEdit::create(context);
            p.baseEdit->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.baseEdit, "Export.SeqBaseName");
            p.zeroPadEdit = ftk::IntEdit::create(context);
            p.zeroPadEdit->setRange(0, 16);
            ftk::setScreenshotTag(p.zeroPadEdit, "Export.SeqZeroPad");
            p.extComboBox = ftk::ComboBox::create(context, p.exts);
            p.extComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.extComboBox, "Export.SeqExt");

            p.fileLabel = ftk::Label::create(context);
            p.existsLabel = ftk::Label::create(context, "(exists)");
            p.existsLabel->setTextRole(ftk::ColorRole::Yellow);
            p.rangeLabel = ftk::Label::create(context);

            p.exportButton = ftk::PushButton::create(context, "Export Sequence");
            ftk::setScreenshotTag(p.exportButton, "Export.SeqExport");

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto formLayout = ftk::FormLayout::create(context, p.layout);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Base name:", p.baseEdit);
            formLayout->addRow("Zero padding:", p.zeroPadEdit);
            formLayout->addRow("Extension:", p.extComboBox);
            auto fileLayout = ftk::HorizontalLayout::create(context);
            fileLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            ftk::setScreenshotTag(fileLayout, "Export.SeqFile");
            p.fileLabel->setParent(fileLayout);
            p.existsLabel->setParent(fileLayout);
            formLayout->addRow("File:", fileLayout);
            ftk::setScreenshotTag(p.rangeLabel, "Export.SeqRange");
            formLayout->addRow("Range:", p.rangeLabel);
            p.layout->addSpacer(ftk::SizeRole::Spacing);
            p.exportButton->setParent(p.layout);

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    FTK_P();
                    p.player = value;
                    p.exportButton->setEnabled(value.get());
                    if (value)
                    {
                        p.inOutRangeObserver = ftk::Observer<OTIO_NS::TimeRange>::create(
                            value->observeInOutRange(),
                            [this](const OTIO_NS::TimeRange&)
                            {
                                _infoUpdate();
                            });
                    }
                    else
                    {
                        p.inOutRangeObserver.reset();
                        _infoUpdate();
                    }
                });

            p.settingsObserver = ftk::Observer<models::ExportSettings>::create(
                p.settings->observeExport(),
                [this](const models::ExportSettings& value)
                {
                    FTK_P();
                    p.baseEdit->setText(value.seqBase);
                    p.zeroPadEdit->setValue(value.seqZeroPad);
                    auto i = std::find(p.exts.begin(), p.exts.end(), value.seqExt);
                    p.extComboBox->setCurrentIndex(i != p.exts.end() ? (i - p.exts.begin()) : -1);
                    _infoUpdate();
                });

            p.timeUnitsObserver = ftk::Observer<tl::TimeUnits>::create(
                p.timeUnitsModel->observeTimeUnits(),
                [this](tl::TimeUnits)
                {
                    _infoUpdate();
                });

            p.baseEdit->setCallback(
                [this](const std::string& value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.seqBase = value;
                    p.settings->setExport(options);
                });

            p.zeroPadEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.seqZeroPad = value;
                    p.settings->setExport(options);
                });

            p.extComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    if (value >= 0 && value < static_cast<int>(p.exts.size()))
                    {
                        auto options = p.settings->getExport();
                        options.seqExt = p.exts[value];
                        p.settings->setExport(options);
                    }
                });
        }

        SeqExportWidget::SeqExportWidget() :
            _p(new Private)
        {}

        SeqExportWidget::~SeqExportWidget()
        {}

        std::shared_ptr<SeqExportWidget> SeqExportWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SeqExportWidget>(new SeqExportWidget);
            out->_init(context, app, parent);
            return out;
        }

        void SeqExportWidget::setExportCallback(const std::function<void(void)>& value)
        {
            _p->exportButton->setClickedCallback(value);
        }

        ftk::Size2I SeqExportWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void SeqExportWidget::setGeometry(const ftk::Box2I& value)
        {
            IExportWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void SeqExportWidget::_infoUpdate()
        {
            FTK_P();
            std::string fileText = "-";
            bool exists = false;
            std::string rangeText = "-";
            if (p.player)
            {
                const auto options = p.settings->getExport();
                const OTIO_NS::TimeRange range = p.player->getInOutRange();
                const std::string firstName = getExportFileName(
                    options,
                    models::ExportFileType::Seq,
                    range.start_time().value());
                const std::string lastName = getExportFileName(
                    options,
                    models::ExportFileType::Seq,
                    range.end_time_inclusive().value());
                if (!firstName.empty())
                {
                    fileText = ftk::Format("{0} - {1}").
                        arg(firstName).
                        arg(lastName);
                    exists = getSeqExists(options, range);
                }
                rangeText = getRangeText(range, p.timeUnitsModel);
            }
            p.fileLabel->setText(fileText);
            p.existsLabel->setVisible(exists);
            p.rangeLabel->setText(rangeText);
        }

        struct MovieExportWidget::Private
        {
            std::shared_ptr<tl::Player> player;
            std::shared_ptr<models::SettingsModel> settings;
            std::shared_ptr<models::TimeUnitsModel> timeUnitsModel;
            std::vector<std::string> exts;
            std::vector<std::string> codecs;
            std::vector<std::string> audioCodecs;

            std::shared_ptr<ftk::LineEdit> baseEdit;
            std::shared_ptr<ftk::ComboBox> extComboBox;
            std::shared_ptr<ftk::ComboBox> codecComboBox;
            std::shared_ptr<ftk::ComboBox> audioCodecComboBox;
            std::shared_ptr<ftk::Label> fileLabel;
            std::shared_ptr<ftk::Label> existsLabel;
            std::shared_ptr<ftk::Label> rangeLabel;
            std::shared_ptr<ftk::PushButton> exportButton;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<models::ExportSettings> > settingsObserver;
            std::shared_ptr<ftk::Observer<OTIO_NS::TimeRange> > inOutRangeObserver;
            std::shared_ptr<ftk::Observer<tl::TimeUnits> > timeUnitsObserver;
        };

        void MovieExportWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IExportWidget::_init(context, "djv::app::MovieExportWidget", parent);
            FTK_P();

            p.settings = app->getSettingsModel();
            p.timeUnitsModel = app->getTimeUnitsModel();

            auto ioSystem = context->getSystem<tl::WriteSystem>();
            for (const auto& ext : ioSystem->getExts(static_cast<int>(tl::FileType::Media)))
            {
                if (std::find(movieExts.begin(), movieExts.end(), ext) != movieExts.end())
                {
                    p.exts.push_back(ext);
                }
            }
#if defined(TLRENDER_FFMPEG_PLUGIN)
            auto ffmpegPlugin = ioSystem->getPlugin<tl::ffmpeg::WritePlugin>();
            p.codecs = ffmpegPlugin->getCodecs();
            p.audioCodecs.push_back("Auto");
            for (const auto& codec : ffmpegPlugin->getAudioCodecs())
            {
                if (std::find(movieAudioCodecs.begin(), movieAudioCodecs.end(), codec) != movieAudioCodecs.end())
                {
                    p.audioCodecs.push_back(codec);
                }
            }
#endif // TLRENDER_FFMPEG_PLUGIN

            p.baseEdit = ftk::LineEdit::create(context);
            p.baseEdit->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.baseEdit, "Export.MovieBaseName");
            p.extComboBox = ftk::ComboBox::create(context, p.exts);
            p.extComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.extComboBox, "Export.MovieExt");
            p.codecComboBox = ftk::ComboBox::create(context, p.codecs);
            p.codecComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.codecComboBox, "Export.MovieCodec");
            p.audioCodecComboBox = ftk::ComboBox::create(context, p.audioCodecs);
            p.audioCodecComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.audioCodecComboBox, "Export.MovieAudioCodec");

            p.fileLabel = ftk::Label::create(context);
            p.existsLabel = ftk::Label::create(context, "(exists)");
            p.existsLabel->setTextRole(ftk::ColorRole::Yellow);
            p.rangeLabel = ftk::Label::create(context);

            p.exportButton = ftk::PushButton::create(context, "Export Movie");
            ftk::setScreenshotTag(p.exportButton, "Export.MovieExport");

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto formLayout = ftk::FormLayout::create(context, p.layout);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Base name:", p.baseEdit);
            formLayout->addRow("Extension:", p.extComboBox);
            formLayout->addRow("Codec:", p.codecComboBox);
            formLayout->addRow("Audio codec:", p.audioCodecComboBox);
            auto fileLayout = ftk::HorizontalLayout::create(context);
            fileLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            ftk::setScreenshotTag(fileLayout, "Export.MovieFile");
            p.fileLabel->setParent(fileLayout);
            p.existsLabel->setParent(fileLayout);
            formLayout->addRow("File:", fileLayout);
            ftk::setScreenshotTag(p.rangeLabel, "Export.MovieRange");
            formLayout->addRow("Range:", p.rangeLabel);
            p.layout->addSpacer(ftk::SizeRole::Spacing);
            p.exportButton->setParent(p.layout);

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    FTK_P();
                    p.player = value;
                    p.exportButton->setEnabled(value.get());
                    p.audioCodecComboBox->setEnabled(
                        !value || value->getIOInfo().audio.isValid());
                    if (value)
                    {
                        p.inOutRangeObserver = ftk::Observer<OTIO_NS::TimeRange>::create(
                            value->observeInOutRange(),
                            [this](const OTIO_NS::TimeRange&)
                            {
                                _infoUpdate();
                            });
                    }
                    else
                    {
                        p.inOutRangeObserver.reset();
                        _infoUpdate();
                    }
                });

            p.settingsObserver = ftk::Observer<models::ExportSettings>::create(
                p.settings->observeExport(),
                [this](const models::ExportSettings& value)
                {
                    FTK_P();
                    p.baseEdit->setText(value.movieBase);
                    auto i = std::find(p.exts.begin(), p.exts.end(), value.movieExt);
                    p.extComboBox->setCurrentIndex(i != p.exts.end() ? (i - p.exts.begin()) : -1);
                    i = std::find(p.codecs.begin(), p.codecs.end(), value.movieCodec);
                    p.codecComboBox->setCurrentIndex(i != p.codecs.end() ? (i - p.codecs.begin()) : -1);
                    i = std::find(p.audioCodecs.begin(), p.audioCodecs.end(), value.movieAudioCodec);
                    p.audioCodecComboBox->setCurrentIndex(i != p.audioCodecs.end() ? (i - p.audioCodecs.begin()) : -1);
                    _infoUpdate();
                });

            p.timeUnitsObserver = ftk::Observer<tl::TimeUnits>::create(
                p.timeUnitsModel->observeTimeUnits(),
                [this](tl::TimeUnits)
                {
                    _infoUpdate();
                });

            p.baseEdit->setCallback(
                [this](const std::string& value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.movieBase = value;
                    p.settings->setExport(options);
                });

            p.extComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    if (value >= 0 && value < static_cast<int>(p.exts.size()))
                    {
                        auto options = p.settings->getExport();
                        options.movieExt = p.exts[value];
                        p.settings->setExport(options);
                    }
                });

            p.codecComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    if (value >= 0 && value < static_cast<int>(p.codecs.size()))
                    {
                        auto options = p.settings->getExport();
                        options.movieCodec = p.codecs[value];
                        p.settings->setExport(options);
                    }
                });

            p.audioCodecComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    if (value >= 0 && value < static_cast<int>(p.audioCodecs.size()))
                    {
                        auto options = p.settings->getExport();
                        options.movieAudioCodec = p.audioCodecs[value];
                        p.settings->setExport(options);
                    }
                });
        }

        MovieExportWidget::MovieExportWidget() :
            _p(new Private)
        {}

        MovieExportWidget::~MovieExportWidget()
        {}

        std::shared_ptr<MovieExportWidget> MovieExportWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<MovieExportWidget>(new MovieExportWidget);
            out->_init(context, app, parent);
            return out;
        }

        void MovieExportWidget::setExportCallback(const std::function<void(void)>& value)
        {
            _p->exportButton->setClickedCallback(value);
        }

        ftk::Size2I MovieExportWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void MovieExportWidget::setGeometry(const ftk::Box2I& value)
        {
            IExportWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void MovieExportWidget::_infoUpdate()
        {
            FTK_P();
            std::string fileText = "-";
            bool exists = false;
            std::string rangeText = "-";
            if (p.player)
            {
                const auto options = p.settings->getExport();
                const OTIO_NS::TimeRange range = p.player->getInOutRange();
                const std::string fileName = getExportFileName(
                    options,
                    models::ExportFileType::Movie,
                    range.start_time().value());
                if (!fileName.empty())
                {
                    fileText = fileName;
                    exists = std::filesystem::exists(std::filesystem::path(
                        ftk::Path(options.dir, fileName).get()));
                }
                rangeText = getRangeText(range, p.timeUnitsModel);
            }
            p.fileLabel->setText(fileText);
            p.existsLabel->setVisible(exists);
            p.rangeLabel->setText(rangeText);
        }
    }
}
