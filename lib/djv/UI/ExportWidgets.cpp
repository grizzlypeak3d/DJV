// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/ExportWidgets.h>

#include <djv/Models/TimeUnitsModel.h>

#include <tlRender/Timeline/Player.h>

#include <tlRender/IO/System.h>
#if defined(TLRENDER_FFMPEG_PLUGIN)
#include <tlRender/IO/FFmpeg.h>
#include <tlRender/IO/FFmpegCmd.h>
#endif // TLRENDER_FFMPEG_PLUGIN

#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/LineEdit.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Path.h>
#include <ftk/Core/String.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>

namespace djv
{
    namespace ui
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

            std::vector<std::string> getMovieExts(const std::shared_ptr<ftk::Context>& context)
            {
                std::vector<std::string> out;
                auto ioSystem = context->getSystem<tl::WriteSystem>();
                for (const auto& ext : ioSystem->getExts(static_cast<int>(tl::FileType::Media)))
                {
                    if (std::find(movieExts.begin(), movieExts.end(), ext) != movieExts.end())
                    {
                        out.push_back(ext);
                    }
                }
                return out;
            }

            // Whether the number in a file name is a run of '#'. Only that is
            // replaced by the frame number: the path reads trailing digits as
            // a frame number too, and replacing those would write
            // "shot_v002.exr" as "shot_v023.exr".
            bool isFrameTemplate(const ftk::Path& path)
            {
                const std::string num = path.getNum();
                return !num.empty() &&
                    std::string::npos == num.find_first_not_of('#');
            }

            // A name typed or pasted with one of the extensions on it takes
            // that extension, rather than being written as "shot.exr.tif".
            bool splitExt(
                std::string& name,
                std::string& ext,
                const std::vector<std::string>& exts)
            {
                const std::string lower = ftk::toLower(name);
                for (const auto& i : exts)
                {
                    if (lower.size() > i.size() &&
                        0 == lower.compare(lower.size() - i.size(), i.size(), i))
                    {
                        name.resize(name.size() - i.size());
                        ext = i;
                        return true;
                    }
                }
                return false;
            }

            // What is wrong with an export file name, or nothing.
            std::string getFileNameError(
                const std::string& fileName,
                const std::string& ext,
                models::ExportFileType fileType,
                const std::vector<std::string>& exts)
            {
                std::string out;
                const ftk::Path path(fileName + ext);
                if (fileName.empty())
                {
                    out = "No file name";
                }
                else if (path.hasDir())
                {
                    out = "No directory; that is set above";
                }
                else if (std::find(exts.begin(), exts.end(), ext) == exts.end())
                {
                    out = "No extension";
                }
                else if (models::ExportFileType::Seq == fileType &&
                    !isFrameTemplate(path))
                {
                    out = "Needs # where the frame number goes";
                }
                else if (models::ExportFileType::Movie == fileType &&
                    isFrameTemplate(path))
                {
                    out = "A movie has no frame number";
                }
                return out;
            }

            std::string getFileNameTooltip(models::ExportFileType fileType)
            {
                std::string out = "The output file name.";
                switch (fileType)
                {
                case models::ExportFileType::Image:
                    out +=
                        "\nFrame numbers are specified with # characters (e.g., render.####.tif).\n"
                        "Digits are part of the name (e.g., shot_v001.jpg).";
                    break;
                case models::ExportFileType::Seq:
                    out +=
                        "\nFrame numbers are specified with # characters (e.g., render.####.tif).";
                    break;
                default: break;
                }
                return out;
            }
        }

        std::string getExportFileName(
            const models::ExportSettings& options,
            models::ExportFileType fileType,
            int64_t frame)
        {
            std::string out;
            switch (fileType)
            {
            case models::ExportFileType::Image:
                out = options.imageFileName + options.imageExt;
                break;
            case models::ExportFileType::Seq:
                out = options.seqFileName + options.seqExt;
                break;
            case models::ExportFileType::Movie:
                out = options.movieFileName + options.movieExt;
                break;
            default: break;
            }
            const ftk::Path path(out);
            if (isFrameTemplate(path))
            {
                out = path.getFrame(frame, true);
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
                const ftk::Path path(options.seqFileName + options.seqExt);
                const std::string base = path.getBase();
                const std::string ext = path.getExt();
                std::error_code ec;
                // The Path helpers throughout: everything else here is
                // UTF-8, and a plain conversion goes through the code page
                // the system happens to be set to, which a name outside it
                // has no mapping in.
                for (const auto& entry :
                    std::filesystem::directory_iterator(
                        ftk::toFileSystem(options.dir), ec))
                {
                    const std::string fileName = ftk::fromFileSystem(entry.path().filename());
                    if (fileName.size() > base.size() + ext.size() &&
                        0 == fileName.compare(0, base.size(), base) &&
                        0 == fileName.compare(
                            fileName.size() - ext.size(),
                            ext.size(),
                            ext))
                    {
                        const std::string digits = fileName.substr(
                            base.size(),
                            fileName.size() - base.size() - ext.size());
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

        bool getExportExists(
            const models::ExportSettings& options,
            models::ExportFileType fileType,
            const OTIO_NS::TimeRange& range)
        {
            bool out = false;
            switch (fileType)
            {
            case models::ExportFileType::Seq:
                // Any frame of the range, since the export writes all of
                // them and overwrites whichever are already there.
                out = getSeqExists(options, range);
                break;
            default:
            {
                const std::string fileName = getExportFileName(
                    options,
                    fileType,
                    static_cast<int64_t>(range.start_time().value()));
                if (!fileName.empty())
                {
                    out = std::filesystem::exists(ftk::toFileSystem(
                        ftk::Path(options.dir, fileName).get()));
                }
                break;
            }
            }
            return out;
        }

        IExportWidget::~IExportWidget()
        {}

        struct ImageExportWidget::Private
        {
            std::shared_ptr<tl::Player> player;
            std::shared_ptr<models::SettingsModel> settings;
            std::vector<std::string> exts;

            std::shared_ptr<ftk::LineEdit> fileNameEdit;
            std::shared_ptr<ftk::ComboBox> extComboBox;
            std::shared_ptr<ftk::Label> fileLabel;
            std::shared_ptr<ftk::PushButton> exportButton;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<models::ExportSettings> > settingsObserver;
            std::shared_ptr<ftk::Observer<OTIO_NS::RationalTime> > currentTimeObserver;
        };

        void ImageExportWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            IExportWidget::_init(context, "djv::app::ImageExportWidget", parent);
            FTK_P();

            p.settings = settingsModel;
            p.exts = getImageExts(context);

            p.fileNameEdit = ftk::LineEdit::create(context);
            p.fileNameEdit->setHStretch(ftk::Stretch::Expanding);
            p.fileNameEdit->setTooltip(getFileNameTooltip(
                models::ExportFileType::Image));
            ftk::setScreenshotTag(p.fileNameEdit, "Export.ImageFileName");
            p.extComboBox = ftk::ComboBox::create(context, p.exts);
            p.extComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.extComboBox, "Export.ImageExt");

            p.fileLabel = ftk::Label::create(context);

            p.exportButton = ftk::PushButton::create(context, "Export Image");
            ftk::setScreenshotTag(p.exportButton, "Export.ImageExport");

            p.layout = ftk::VerticalLayout::create(context);

            _setWidget(p.layout);
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto formLayout = ftk::FormLayout::create(context, p.layout);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("File name:", p.fileNameEdit);
            formLayout->addRow("Extension:", p.extComboBox);
            ftk::setScreenshotTag(p.fileLabel, "Export.ImageFile");
            formLayout->addRow("Output:", p.fileLabel);
            p.layout->addSpacer(ftk::SizeRole::Spacing);
            p.exportButton->setParent(p.layout);

            p.settingsObserver = ftk::Observer<models::ExportSettings>::create(
                p.settings->observeExport(),
                [this](const models::ExportSettings& value)
                {
                    FTK_P();
                    auto options = value;
                    if (splitExt(options.imageFileName, options.imageExt, p.exts))
                    {
                        p.settings->setExport(options);
                        return;
                    }
                    if (p.fileNameEdit->getText() != value.imageFileName)
                    {
                        p.fileNameEdit->setText(value.imageFileName);
                    }
                    const auto j = std::find(p.exts.begin(), p.exts.end(), value.imageExt);
                    p.extComboBox->setCurrentIndex(j != p.exts.end() ? (j - p.exts.begin()) : -1);
                    _infoUpdate();
                });

            p.fileNameEdit->setCallback(
                [this](const std::string& value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.imageFileName = value;
                    splitExt(options.imageFileName, options.imageExt, p.exts);
                    p.settings->setExport(options);
                    if (p.fileNameEdit->getText() != options.imageFileName)
                    {
                        p.fileNameEdit->setText(options.imageFileName);
                    }
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

            p.fileNameEdit->setTextChangedCallback(
                [this](const std::string&)
                {
                    _infoUpdate();
                });
        }

        ImageExportWidget::ImageExportWidget() :
            _p(new Private)
        {}

        ImageExportWidget::~ImageExportWidget()
        {}

        std::shared_ptr<ImageExportWidget> ImageExportWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ImageExportWidget>(new ImageExportWidget);
            out->_init(context, settingsModel, parent);
            return out;
        }

        void ImageExportWidget::setPlayer(const std::shared_ptr<tl::Player>& value)
        {
                FTK_P();
                p.player = value;
                _infoUpdate();
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
        }

        void ImageExportWidget::setExportCallback(const std::function<void(void)>& value)
        {
            _p->exportButton->setClickedCallback(value);
        }

        void ImageExportWidget::_infoUpdate()
        {
            FTK_P();
            // From the text being typed rather than the setting, which only
            // changes once the edit is committed, so that what is wrong with
            // a name shows while it is being typed.
            auto options = p.settings->getExport();
            options.imageFileName = p.fileNameEdit->getText();
            splitExt(options.imageFileName, options.imageExt, p.exts);
            const std::string error = getFileNameError(
                options.imageFileName,
                options.imageExt,
                models::ExportFileType::Image,
                p.exts);
            std::string fileText = error.empty() ? "-" : error;
            if (p.player && error.empty())
            {
                fileText = getExportFileName(
                    options,
                    models::ExportFileType::Image,
                    static_cast<int64_t>(p.player->getCurrentTime().value()));
            }
            p.fileLabel->setText(fileText);
            p.fileLabel->setTextRole(error.empty() ?
                ftk::ColorRole::Text :
                ftk::ColorRole::Red);
            p.exportButton->setEnabled(p.player && error.empty());
        }

        struct SeqExportWidget::Private
        {
            std::shared_ptr<tl::Player> player;
            std::shared_ptr<models::SettingsModel> settings;
            std::shared_ptr<models::TimeUnitsModel> timeUnitsModel;
            std::vector<std::string> exts;

            std::shared_ptr<ftk::LineEdit> fileNameEdit;
            std::shared_ptr<ftk::ComboBox> extComboBox;
            std::shared_ptr<ftk::Label> fileLabel;
            std::shared_ptr<ftk::Label> rangeLabel;
            std::shared_ptr<ftk::PushButton> exportButton;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<models::ExportSettings> > settingsObserver;
            std::shared_ptr<ftk::Observer<OTIO_NS::TimeRange> > inOutRangeObserver;
            std::shared_ptr<ftk::Observer<tl::TimeUnits> > timeUnitsObserver;
        };

        void SeqExportWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            IExportWidget::_init(context, "djv::app::SeqExportWidget", parent);
            FTK_P();

            p.settings = settingsModel;
            p.timeUnitsModel = timeUnitsModel;
            p.exts = getImageExts(context);

            p.fileNameEdit = ftk::LineEdit::create(context);
            p.fileNameEdit->setHStretch(ftk::Stretch::Expanding);
            p.fileNameEdit->setTooltip(getFileNameTooltip(
                models::ExportFileType::Seq));
            ftk::setScreenshotTag(p.fileNameEdit, "Export.SeqFileName");
            p.extComboBox = ftk::ComboBox::create(context, p.exts);
            p.extComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.extComboBox, "Export.SeqExt");

            p.fileLabel = ftk::Label::create(context);
            p.rangeLabel = ftk::Label::create(context);

            p.exportButton = ftk::PushButton::create(context, "Export Sequence");
            ftk::setScreenshotTag(p.exportButton, "Export.SeqExport");

            p.layout = ftk::VerticalLayout::create(context);

            _setWidget(p.layout);
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto formLayout = ftk::FormLayout::create(context, p.layout);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("File name:", p.fileNameEdit);
            formLayout->addRow("Extension:", p.extComboBox);
            ftk::setScreenshotTag(p.fileLabel, "Export.SeqFile");
            formLayout->addRow("Output:", p.fileLabel);
            ftk::setScreenshotTag(p.rangeLabel, "Export.SeqRange");
            formLayout->addRow("Range:", p.rangeLabel);
            p.layout->addSpacer(ftk::SizeRole::Spacing);
            p.exportButton->setParent(p.layout);

            p.settingsObserver = ftk::Observer<models::ExportSettings>::create(
                p.settings->observeExport(),
                [this](const models::ExportSettings& value)
                {
                    FTK_P();
                    auto options = value;
                    if (splitExt(options.seqFileName, options.seqExt, p.exts))
                    {
                        p.settings->setExport(options);
                        return;
                    }
                    if (p.fileNameEdit->getText() != value.seqFileName)
                    {
                        p.fileNameEdit->setText(value.seqFileName);
                    }
                    const auto j = std::find(p.exts.begin(), p.exts.end(), value.seqExt);
                    p.extComboBox->setCurrentIndex(j != p.exts.end() ? (j - p.exts.begin()) : -1);
                    _infoUpdate();
                });

            p.timeUnitsObserver = ftk::Observer<tl::TimeUnits>::create(
                p.timeUnitsModel->observeTimeUnits(),
                [this](tl::TimeUnits)
                {
                    _infoUpdate();
                });

            p.fileNameEdit->setCallback(
                [this](const std::string& value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.seqFileName = value;
                    splitExt(options.seqFileName, options.seqExt, p.exts);
                    p.settings->setExport(options);
                    if (p.fileNameEdit->getText() != options.seqFileName)
                    {
                        p.fileNameEdit->setText(options.seqFileName);
                    }
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

            p.fileNameEdit->setTextChangedCallback(
                [this](const std::string&)
                {
                    _infoUpdate();
                });
        }

        SeqExportWidget::SeqExportWidget() :
            _p(new Private)
        {}

        SeqExportWidget::~SeqExportWidget()
        {}

        std::shared_ptr<SeqExportWidget> SeqExportWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SeqExportWidget>(new SeqExportWidget);
            out->_init(context, settingsModel, timeUnitsModel, parent);
            return out;
        }

        void SeqExportWidget::setPlayer(const std::shared_ptr<tl::Player>& value)
        {
                FTK_P();
                p.player = value;
                _infoUpdate();
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
        }

        void SeqExportWidget::setExportCallback(const std::function<void(void)>& value)
        {
            _p->exportButton->setClickedCallback(value);
        }

        void SeqExportWidget::_infoUpdate()
        {
            FTK_P();
            // From the text being typed rather than the setting, which only
            // changes once the edit is committed, so that what is wrong with
            // a name shows while it is being typed.
            auto options = p.settings->getExport();
            options.seqFileName = p.fileNameEdit->getText();
            splitExt(options.seqFileName, options.seqExt, p.exts);
            const std::string error = getFileNameError(
                options.seqFileName,
                options.seqExt,
                models::ExportFileType::Seq,
                p.exts);
            std::string fileText = error.empty() ? "-" : error;
            std::string rangeText = "-";
            if (p.player)
            {
                const OTIO_NS::TimeRange range = p.player->getInOutRange();
                if (error.empty())
                {
                    fileText = ftk::Format("{0} - {1}").
                        arg(getExportFileName(
                            options,
                            models::ExportFileType::Seq,
                            static_cast<int64_t>(range.start_time().value()))).
                        arg(getExportFileName(
                            options,
                            models::ExportFileType::Seq,
                            static_cast<int64_t>(range.end_time_inclusive().value())));
                }
                rangeText = getRangeText(range, p.timeUnitsModel);
            }
            p.fileLabel->setText(fileText);
            p.fileLabel->setTextRole(error.empty() ?
                ftk::ColorRole::Text :
                ftk::ColorRole::Red);
            p.rangeLabel->setText(rangeText);
            p.exportButton->setEnabled(p.player && error.empty());
        }

        struct MovieExportWidget::Private
        {
            std::shared_ptr<tl::Player> player;
            std::shared_ptr<models::SettingsModel> settings;
            std::shared_ptr<models::TimeUnitsModel> timeUnitsModel;
            std::vector<std::string> exts;
            std::vector<std::string> audioCodecs;
            std::vector<std::string> presets;

            std::shared_ptr<ftk::LineEdit> fileNameEdit;
            std::shared_ptr<ftk::ComboBox> extComboBox;
            std::shared_ptr<ftk::ComboBox> audioCodecComboBox;
            std::shared_ptr<ftk::ComboBox> presetComboBox;
            std::shared_ptr<ftk::Label> fileLabel;
            std::shared_ptr<ftk::Label> rangeLabel;
            std::shared_ptr<ftk::PushButton> exportButton;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<models::ExportSettings> > settingsObserver;
            std::shared_ptr<ftk::Observer<OTIO_NS::TimeRange> > inOutRangeObserver;
            std::shared_ptr<ftk::Observer<tl::TimeUnits> > timeUnitsObserver;
        };

        void MovieExportWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            IExportWidget::_init(context, "djv::app::MovieExportWidget", parent);
            FTK_P();

            p.settings = settingsModel;
            p.timeUnitsModel = timeUnitsModel;

            p.exts = getMovieExts(context);
#if defined(TLRENDER_FFMPEG_PLUGIN)
            auto ioSystem = context->getSystem<tl::WriteSystem>();
            auto ffmpegPlugin = ioSystem->getPlugin<tl::ffmpeg::WritePlugin>();
            p.audioCodecs.push_back("Auto");
            for (const auto& codec : ffmpegPlugin->getAudioCodecs())
            {
                if (std::find(movieAudioCodecs.begin(), movieAudioCodecs.end(), codec) != movieAudioCodecs.end())
                {
                    p.audioCodecs.push_back(codec);
                }
            }
#endif // TLRENDER_FFMPEG_PLUGIN

            p.fileNameEdit = ftk::LineEdit::create(context);
            p.fileNameEdit->setHStretch(ftk::Stretch::Expanding);
            p.fileNameEdit->setTooltip(getFileNameTooltip(
                models::ExportFileType::Movie));
            ftk::setScreenshotTag(p.fileNameEdit, "Export.MovieFileName");
            p.extComboBox = ftk::ComboBox::create(context, p.exts);
            p.extComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.extComboBox, "Export.MovieExt");
            p.audioCodecComboBox = ftk::ComboBox::create(context, p.audioCodecs);
            p.audioCodecComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.audioCodecComboBox, "Export.MovieAudioCodec");
#if defined(TLRENDER_FFMPEG_PLUGIN)
            // The presets are the whole surface, so choosing an output does
            // not mean picking through every encoder FFmpeg has; what they
            // cannot express is what tlbake is for.
            for (const auto& preset : tl::ffmpeg::getWritePresets())
            {
                p.presets.push_back(preset.name);
            }
#endif // TLRENDER_FFMPEG_PLUGIN
            p.presetComboBox = ftk::ComboBox::create(context, p.presets);
            p.presetComboBox->setHStretch(ftk::Stretch::Expanding);
            p.presetComboBox->setTooltip(
                "What to export. The command line presets use the FFmpeg "
                "application, with its encoders, and write video only.");
            ftk::setScreenshotTag(p.presetComboBox, "Export.MoviePreset");

            p.fileLabel = ftk::Label::create(context);
            p.rangeLabel = ftk::Label::create(context);

            p.exportButton = ftk::PushButton::create(context, "Export Movie");
            ftk::setScreenshotTag(p.exportButton, "Export.MovieExport");

            p.layout = ftk::VerticalLayout::create(context);

            _setWidget(p.layout);
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto formLayout = ftk::FormLayout::create(context, p.layout);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("File name:", p.fileNameEdit);
            formLayout->addRow("Extension:", p.extComboBox);
            formLayout->addRow("Preset:", p.presetComboBox);
            formLayout->addRow("Audio codec:", p.audioCodecComboBox);
            ftk::setScreenshotTag(p.fileLabel, "Export.MovieFile");
            formLayout->addRow("Output:", p.fileLabel);
            ftk::setScreenshotTag(p.rangeLabel, "Export.MovieRange");
            formLayout->addRow("Range:", p.rangeLabel);
            p.layout->addSpacer(ftk::SizeRole::Spacing);
            p.exportButton->setParent(p.layout);

            p.settingsObserver = ftk::Observer<models::ExportSettings>::create(
                p.settings->observeExport(),
                [this](const models::ExportSettings& value)
                {
                    FTK_P();
                    auto options = value;
                    if (splitExt(options.movieFileName, options.movieExt, p.exts))
                    {
                        p.settings->setExport(options);
                        return;
                    }
                    if (p.fileNameEdit->getText() != value.movieFileName)
                    {
                        p.fileNameEdit->setText(value.movieFileName);
                    }
                    const auto j = std::find(p.exts.begin(), p.exts.end(), value.movieExt);
                    p.extComboBox->setCurrentIndex(j != p.exts.end() ? (j - p.exts.begin()) : -1);
                    auto i = std::find(p.audioCodecs.begin(), p.audioCodecs.end(), value.movieAudioCodec);
                    p.audioCodecComboBox->setCurrentIndex(i != p.audioCodecs.end() ? (i - p.audioCodecs.begin()) : -1);
                    i = std::find(p.presets.begin(), p.presets.end(), value.moviePreset);
                    p.presetComboBox->setCurrentIndex(i != p.presets.end() ? (i - p.presets.begin()) : -1);
                    _audioUpdate();
                    _infoUpdate();
                });

            p.timeUnitsObserver = ftk::Observer<tl::TimeUnits>::create(
                p.timeUnitsModel->observeTimeUnits(),
                [this](tl::TimeUnits)
                {
                    _infoUpdate();
                });

            p.fileNameEdit->setCallback(
                [this](const std::string& value)
                {
                    FTK_P();
                    auto options = p.settings->getExport();
                    options.movieFileName = value;
                    splitExt(options.movieFileName, options.movieExt, p.exts);
                    p.settings->setExport(options);
                    if (p.fileNameEdit->getText() != options.movieFileName)
                    {
                        p.fileNameEdit->setText(options.movieFileName);
                    }
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

            p.fileNameEdit->setTextChangedCallback(
                [this](const std::string&)
                {
                    _infoUpdate();
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

            p.presetComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    if (value >= 0 && value < static_cast<int>(p.presets.size()))
                    {
                        auto options = p.settings->getExport();
                        options.moviePreset = p.presets[value];
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
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<MovieExportWidget>(new MovieExportWidget);
            out->_init(context, settingsModel, timeUnitsModel, parent);
            return out;
        }

        void MovieExportWidget::_audioUpdate()
        {
            FTK_P();
            // The audio codec has something to say only when the file has
            // audio and the preset is not a command line one, which writes
            // video only.
            bool presetCmd = false;
#if defined(TLRENDER_FFMPEG_PLUGIN)
            const std::string presetName = p.settings->getExport().moviePreset;
            for (const auto& preset : tl::ffmpeg::getWritePresets())
            {
                if (preset.name == presetName)
                {
                    presetCmd = preset.command;
                    break;
                }
            }
#endif // TLRENDER_FFMPEG_PLUGIN
            const bool hasAudio =
                !p.player || p.player->getIOInfo().audio.isValid();
            p.audioCodecComboBox->setEnabled(hasAudio && !presetCmd);
        }

        void MovieExportWidget::setPlayer(const std::shared_ptr<tl::Player>& value)
        {
                FTK_P();
                p.player = value;
                _infoUpdate();
                _audioUpdate();
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
        }

        void MovieExportWidget::setExportCallback(const std::function<void(void)>& value)
        {
            _p->exportButton->setClickedCallback(value);
        }

        void MovieExportWidget::_infoUpdate()
        {
            FTK_P();
            // From the text being typed rather than the setting, which only
            // changes once the edit is committed, so that what is wrong with
            // a name shows while it is being typed.
            auto options = p.settings->getExport();
            options.movieFileName = p.fileNameEdit->getText();
            splitExt(options.movieFileName, options.movieExt, p.exts);
            const std::string error = getFileNameError(
                options.movieFileName,
                options.movieExt,
                models::ExportFileType::Movie,
                p.exts);
            std::string fileText = error.empty() ? "-" : error;
            std::string rangeText = "-";
            if (p.player)
            {
                const OTIO_NS::TimeRange range = p.player->getInOutRange();
                if (error.empty())
                {
                    fileText = getExportFileName(
                        options,
                        models::ExportFileType::Movie,
                        static_cast<int64_t>(range.start_time().value()));
                }
                rangeText = getRangeText(range, p.timeUnitsModel);
            }
            p.fileLabel->setText(fileText);
            p.fileLabel->setTextRole(error.empty() ?
                ftk::ColorRole::Text :
                ftk::ColorRole::Red);
            p.rangeLabel->setText(rangeText);
            p.exportButton->setEnabled(p.player && error.empty());
        }
    }
}
