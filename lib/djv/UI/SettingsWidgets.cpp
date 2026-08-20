// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/SettingsWidgets.h>

#include <djv/Models/TimeUnitsModel.h>

#if defined(TLRENDER_USD)
#include <tlRender/IO/USD.h>
#endif // TLRENDER_USD

#include <ftk/UI/Bellows.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/DoubleEdit.h>
#include <ftk/UI/FileBrowser.h>
#include <ftk/UI/FileEdit.h>
#include <ftk/UI/FloatEdit.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/IntEdit.h>
#include <ftk/UI/IntEditSlider.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/LineEdit.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace ui
    {
        ISettingsWidget::~ISettingsWidget()
        {}

        struct AudioSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::IntEdit> bufferFramesEdit;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<models::AudioSettings> > settingsObserver;
        };

        void AudioSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::AudioSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.bufferFramesEdit = ftk::IntEdit::create(context);
            p.bufferFramesEdit->setRange(1, 1000000);
            p.bufferFramesEdit->setStep(256);
            p.bufferFramesEdit->setLargeStep(1024);
            p.bufferFramesEdit->setTooltip(
                "The size of the buffer the audio device is given.\n"
                "\n"
                "Increase this if the audio breaks up or crackles during\n"
                "playback. Smaller values reduce the audio latency.");

            p.layout = ftk::FormLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Buffer frames:", p.bufferFramesEdit);

            p.settingsObserver = ftk::Observer<models::AudioSettings>::create(
                settings->observeAudio(),
                [this](const models::AudioSettings& value)
                {
                    _p->bufferFramesEdit->setValue(value.bufferFrameCount);
                });

            p.bufferFramesEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    models::AudioSettings settings = p.settings->getAudio();
                    settings.bufferFrameCount = value;
                    p.settings->setAudio(settings);
                });
        }

        AudioSettingsWidget::AudioSettingsWidget() :
            _p(new Private)
        {}

        AudioSettingsWidget::~AudioSettingsWidget()
        {}

        std::shared_ptr<AudioSettingsWidget> AudioSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<AudioSettingsWidget>(new AudioSettingsWidget);
            out->_init(context, settings, parent);
            return out;
        }

        struct CacheSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::FloatEdit> videoEdit;
            std::shared_ptr<ftk::FloatEdit> audioEdit;
            std::shared_ptr<ftk::FloatEdit> readBehindEdit;
            std::shared_ptr<ftk::FloatEdit> thumbnailEdit;
            std::shared_ptr<ftk::FloatEdit> waveformEdit;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::PlayerCacheOptions> > cacheObserver;
            std::shared_ptr<ftk::Observer<tl::ui::ThumbnailCacheOptions> > thumbnailCacheObserver;
        };

        void CacheSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::CacheSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.videoEdit = ftk::FloatEdit::create(context);
            p.videoEdit->setRange(0.F, 1024.F);
            p.videoEdit->setStep(1.0);
            p.videoEdit->setLargeStep(10.0);

            p.audioEdit = ftk::FloatEdit::create(context);
            p.audioEdit->setRange(0.F, 1024.F);
            p.audioEdit->setStep(1.0);
            p.audioEdit->setLargeStep(10.0);

            p.readBehindEdit = ftk::FloatEdit::create(context);
            p.readBehindEdit->setRange(0.F, 10.F);
            p.readBehindEdit->setStep(0.1);
            p.readBehindEdit->setLargeStep(1.0);

            p.thumbnailEdit = ftk::FloatEdit::create(context);
            p.thumbnailEdit->setRange(0.F, 1024.F);
            p.thumbnailEdit->setStep(1.0);
            p.thumbnailEdit->setLargeStep(10.0);

            p.waveformEdit = ftk::FloatEdit::create(context);
            p.waveformEdit->setRange(0.F, 1024.F);
            p.waveformEdit->setStep(1.0);
            p.waveformEdit->setLargeStep(10.0);

            p.layout = ftk::FormLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto hLayout = ftk::HorizontalLayout::create(context);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.videoEdit->setParent(hLayout);
            ftk::Label::create(context, "GB", hLayout);
            p.layout->addRow("Video cache:", hLayout);
            hLayout = ftk::HorizontalLayout::create(context);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.audioEdit->setParent(hLayout);
            ftk::Label::create(context, "GB", hLayout);
            p.layout->addRow("Audio cache:", hLayout);
            hLayout = ftk::HorizontalLayout::create(context);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.readBehindEdit->setParent(hLayout);
            ftk::Label::create(context, "seconds", hLayout);
            p.layout->addRow("Read behind:", hLayout);
            hLayout = ftk::HorizontalLayout::create(context);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.thumbnailEdit->setParent(hLayout);
            ftk::Label::create(context, "MB", hLayout);
            p.layout->addRow("Thumbnails:", hLayout);
            hLayout = ftk::HorizontalLayout::create(context);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.waveformEdit->setParent(hLayout);
            ftk::Label::create(context, "MB", hLayout);
            p.layout->addRow("Waveforms:", hLayout);

            p.cacheObserver = ftk::Observer<tl::PlayerCacheOptions>::create(
                settings->observeCache(),
                [this](const tl::PlayerCacheOptions& value)
                {
                    FTK_P();
                    p.videoEdit->setValue(value.videoGB);
                    p.audioEdit->setValue(value.audioGB);
                    p.readBehindEdit->setValue(value.readBehind);
                });

            p.thumbnailCacheObserver = ftk::Observer<tl::ui::ThumbnailCacheOptions>::create(
                settings->observeThumbnailCache(),
                [this](const tl::ui::ThumbnailCacheOptions& value)
                {
                    FTK_P();
                    p.thumbnailEdit->setValue(value.thumbnailMB);
                    p.waveformEdit->setValue(value.waveformMB);
                });

            p.videoEdit->setCallback(
                [this](float value)
                {
                    FTK_P();
                    tl::PlayerCacheOptions settings = p.settings->getCache();
                    settings.videoGB = value;
                    p.settings->setCache(settings);
                });

            p.audioEdit->setCallback(
                [this](float value)
                {
                    FTK_P();
                    tl::PlayerCacheOptions settings = p.settings->getCache();
                    settings.audioGB = value;
                    p.settings->setCache(settings);
                });

            p.readBehindEdit->setCallback(
                [this](float value)
                {
                    FTK_P();
                    tl::PlayerCacheOptions options = p.settings->getCache();
                    options.readBehind = value;
                    p.settings->setCache(options);
                });

            p.thumbnailEdit->setCallback(
                [this](float value)
                {
                    FTK_P();
                    tl::ui::ThumbnailCacheOptions options = p.settings->getThumbnailCache();
                    options.thumbnailMB = value;
                    p.settings->setThumbnailCache(options);
                });

            p.waveformEdit->setCallback(
                [this](float value)
                {
                    FTK_P();
                    tl::ui::ThumbnailCacheOptions options = p.settings->getThumbnailCache();
                    options.waveformMB = value;
                    p.settings->setThumbnailCache(options);
                });
        }

        CacheSettingsWidget::CacheSettingsWidget() :
            _p(new Private)
        {}

        CacheSettingsWidget::~CacheSettingsWidget()
        {}

        std::shared_ptr<CacheSettingsWidget> CacheSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<CacheSettingsWidget>(new CacheSettingsWidget);
            out->_init(context, settings, parent);
            return out;
        }

        struct FileBrowserSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::CheckBox> nfdCheckBox;
            std::shared_ptr<ftk::CheckBox> floatingCheckBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<models::FileBrowserSettings> > settingsObserver;
        };

        void FileBrowserSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::FileBrowserSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.nfdCheckBox = ftk::CheckBox::create(context);
            p.nfdCheckBox->setHStretch(ftk::Stretch::Expanding);

            p.floatingCheckBox = ftk::CheckBox::create(context);
            p.floatingCheckBox->setHStretch(ftk::Stretch::Expanding);
            p.floatingCheckBox->setTooltip(
                "Show the file browser in a window of its own, so that it\n"
                "does not cover what is being played while you browse.\n"
                "\n"
                "Takes effect the next time the file browser is opened.");

            p.layout = ftk::FormLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Native file dialog:", p.nfdCheckBox);
            p.layout->addRow("Floating window:", p.floatingCheckBox);

            // The native dialog is not built everywhere: it means linking to
            // the desktop's own, which is not wanted on Linux. Where it is not
            // built the setting is remembered but does nothing, so it is shown
            // as what it is rather than as a choice.
            const bool nativeAvailable = context->getSystem<ftk::FileBrowserSystem>()->
                isNativeFileDialogAvailable();
            p.nfdCheckBox->setEnabled(nativeAvailable);
            if (!nativeAvailable)
            {
                p.nfdCheckBox->setTooltip(
                    "The native file dialog is not available in this build.");
            }

            p.settingsObserver = ftk::Observer<models::FileBrowserSettings>::create(
                settings->observeFileBrowser(),
                [this, nativeAvailable](const models::FileBrowserSettings& value)
                {
                    FTK_P();
                    p.nfdCheckBox->setChecked(value.nativeFileDialog);
                    p.floatingCheckBox->setChecked(value.floating);
                    // The native dialog is a window of its own already, and
                    // is not ours to place -- but only where there is one.
                    p.floatingCheckBox->setEnabled(
                        !(nativeAvailable && value.nativeFileDialog));
                });

            p.nfdCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    auto settings = p.settings->getFileBrowser();
                    settings.nativeFileDialog = value;
                    p.settings->setFileBrowser(settings);
                });

            p.floatingCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    auto settings = p.settings->getFileBrowser();
                    settings.floating = value;
                    p.settings->setFileBrowser(settings);
                });
        }

        FileBrowserSettingsWidget::FileBrowserSettingsWidget() :
            _p(new Private)
        {}

        FileBrowserSettingsWidget::~FileBrowserSettingsWidget()
        {}

        std::shared_ptr<FileBrowserSettingsWidget> FileBrowserSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<FileBrowserSettingsWidget>(new FileBrowserSettingsWidget);
            out->_init(context, settings, parent);
            return out;
        }

        struct OTIOSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::ComboBox> spatialComboBox;
            std::shared_ptr<ftk::CheckBox> compatCheckBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<models::OTIOSettings> > settingsObserver;
        };

        void OTIOSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::OTIOSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.spatialComboBox = ftk::ComboBox::create(context, tl::getSpatialLabels());
            p.spatialComboBox->setHStretch(ftk::Stretch::Expanding);
            p.spatialComboBox->setTooltip(
                "Use the spatial coordinates in OTIO files to position and\n"
                "size the images.\n"
                "\n"
                "* None: Ignore the spatial coordinates.\n"
                "* Coordinates: Use the spatial coordinates where clips\n"
                "  provide them.\n"
                "* Normalize: Use the spatial coordinates, and display clips\n"
                "  without them at the size of the first clip. Use this to\n"
                "  play clips of differing resolutions at the same size.");

            p.compatCheckBox = ftk::CheckBox::create(context);
            p.compatCheckBox->setHStretch(ftk::Stretch::Expanding);
            p.compatCheckBox->setTooltip(
                "Enable workarounds for timelines that may not conform exactly\n"
                "to specification.");

            p.layout = ftk::FormLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Spatial coordinates:", p.spatialComboBox);
            p.layout->addRow("Compatibility:", p.compatCheckBox);

            p.settingsObserver = ftk::Observer<models::OTIOSettings>::create(
                settings->observeOTIO(),
                [this](const models::OTIOSettings& value)
                {
                    FTK_P();
                    p.spatialComboBox->setCurrentIndex(
                        static_cast<int>(value.spatial));
                    p.compatCheckBox->setChecked(value.compat);
                });

            p.spatialComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    models::OTIOSettings settings = p.settings->getOTIO();
                    settings.spatial = static_cast<tl::Spatial>(value);
                    p.settings->setOTIO(settings);
                });

            p.compatCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    models::OTIOSettings settings = p.settings->getOTIO();
                    settings.compat = value;
                    p.settings->setOTIO(settings);
                });
        }

        OTIOSettingsWidget::OTIOSettingsWidget() :
            _p(new Private)
        {}

        OTIOSettingsWidget::~OTIOSettingsWidget()
        {}

        std::shared_ptr<OTIOSettingsWidget> OTIOSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<OTIOSettingsWidget>(new OTIOSettingsWidget);
            out->_init(context, settings, parent);
            return out;
        }

        struct ImageSeqSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::ComboBox> audioComboBox;
            std::shared_ptr<ftk::LineEdit> audioExtensionsEdit;
            std::shared_ptr<ftk::LineEdit> audioFileNameEdit;
            std::shared_ptr<ftk::IntEdit> maxDigitsEdit;
            std::shared_ptr<ftk::DoubleEdit> defaultSpeedEdit;
            std::shared_ptr<ftk::ComboBox> missingFramesComboBox;
            std::shared_ptr<ftk::CheckBox> missingIndicatorCheckBox;
            std::shared_ptr<ftk::IntEditSlider> missingIndicatorWidthEdit;
            std::shared_ptr<ftk::ColorSwatch> missingIndicatorColorSwatch;
            std::shared_ptr<ftk::IntEdit> threadsEdit;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<models::ImageSeqSettings> > settingsObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > foregroundObserver;
        };

        void ImageSeqSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::ImageSeqSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.audioComboBox = ftk::ComboBox::create(context, tl::getImageSeqAudioLabels());
            p.audioComboBox->setHStretch(ftk::Stretch::Expanding);
            p.audioComboBox->setTooltip(
                "Open audio files for image sequences.\n"
                "\n"
                "* None: Do not open audio files.\n"
                "* Ext: Find audio files by extension.\n"
                "* Filename: Specify the file name to open.");

            p.audioExtensionsEdit = ftk::LineEdit::create(context);
            p.audioExtensionsEdit->setHStretch(ftk::Stretch::Expanding);
            p.audioExtensionsEdit->setTooltip(
                "List of audio file extensions to search for.\n"
                "\n"
                "Example: .wav .mp3");

            p.audioFileNameEdit = ftk::LineEdit::create(context);
            p.audioFileNameEdit->setHStretch(ftk::Stretch::Expanding);
            p.audioFileNameEdit->setTooltip("Audio file name to open.");

            p.maxDigitsEdit = ftk::IntEdit::create(context);
            p.maxDigitsEdit->setTooltip(
                "Maximum number of digits allowed in a frame number.");

            p.defaultSpeedEdit = ftk::DoubleEdit::create(context);
            p.defaultSpeedEdit->setRange(1.0, 120.0);

            p.missingFramesComboBox = ftk::ComboBox::create(
                context, tl::getMissingFramesLabels());
            p.missingFramesComboBox->setHStretch(ftk::Stretch::Expanding);
            p.missingFramesComboBox->setTooltip(
                "What to show for a frame a sequence does not have, which\n"
                "happens with a render still in progress.\n"
                "\n"
                "* Error: The frame does not read.\n"
                "* Hold: Repeat the nearest frame before it.\n"
                "* Black: A blank frame.\n"
                "* Skip: Leave it out, so only the frames that are there\n"
                "  play and the sequence is as long as they are.\n"
                "* Gaps: Leave a hole, so every frame keeps the time it\n"
                "  had and the gaps show on the timeline.\n"
                "\n"
                "The first three are decided as each frame is read, and a\n"
                "timeline that says what it wants is followed instead. The\n"
                "last two decide what the timeline is built from, so they\n"
                "only apply to an image sequence opened on its own, and\n"
                "changing to or from one opens the file again.");

            p.missingIndicatorCheckBox = ftk::CheckBox::create(context);
            p.missingIndicatorCheckBox->setTooltip(
                "Mark a picture that stands in for a frame the sequence does\n"
                "not have, so a render still in progress does not look\n"
                "finished.\n"
                "\n"
                "A cross is drawn over the picture, and the heads up display\n"
                "time says which frame is being held.\n"
                "\n"
                "Applies to Hold and Black. The others either show nothing or\n"
                "leave no missing frames to mark. Unlike the setting above,\n"
                "this is only what is drawn, so it applies to what is already\n"
                "open.");
            ftk::setScreenshotTag(
                p.missingIndicatorCheckBox, "Settings.MissingIndicator.Enabled");

            p.missingIndicatorWidthEdit = ftk::IntEditSlider::create(context);
            p.missingIndicatorWidthEdit->setRange(1, 40);
            ftk::setScreenshotTag(
                p.missingIndicatorWidthEdit, "Settings.MissingIndicator.LineWidth");

            p.missingIndicatorColorSwatch = ftk::ColorSwatch::create(context);
            p.missingIndicatorColorSwatch->setEditable(true);
            p.missingIndicatorColorSwatch->setHAlign(ftk::HAlign::Left);
            ftk::setScreenshotTag(
                p.missingIndicatorColorSwatch, "Settings.MissingIndicator.Color");

            p.threadsEdit = ftk::IntEdit::create(context);
            p.threadsEdit->setRange(1, 64);

            p.layout = ftk::FormLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Open audio files:", p.audioComboBox);
            p.layout->addRow("Audio file extensions:", p.audioExtensionsEdit);
            p.layout->addRow("Audio file name:", p.audioFileNameEdit);
            p.layout->addRow("Maximum digits:", p.maxDigitsEdit);
            auto hLayout = ftk::HorizontalLayout::create(context);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.defaultSpeedEdit->setParent(hLayout);
            ftk::Label::create(context, "FPS", hLayout);
            p.layout->addRow("Default speed:", hLayout);
            p.layout->addRow("Missing frames:", p.missingFramesComboBox);
            p.layout->addRow("Indicate missing:", p.missingIndicatorCheckBox);
            p.layout->addRow("Indicator width:", p.missingIndicatorWidthEdit);
            p.layout->addRow("Indicator color:", p.missingIndicatorColorSwatch);
            p.layout->addRow("I/O threads:", p.threadsEdit);

            // The indicator lives with the viewport's other drawing rather
            // than with the sequence settings, since it changes only what is
            // drawn. It is shown here because this is where someone deciding
            // what to do about missing frames is looking.
            p.foregroundObserver = ftk::Observer<tl::ForegroundOptions>::create(
                viewportModel->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
                {
                    FTK_P();
                    p.missingIndicatorCheckBox->setChecked(
                        value.missingIndicator.enabled);
                    p.missingIndicatorWidthEdit->setValue(
                        value.missingIndicator.width);
                    p.missingIndicatorColorSwatch->setColor(
                        value.missingIndicator.color);
                });

            p.missingIndicatorCheckBox->setCheckedCallback(
                [viewportModel](bool value)
                {
                    auto options = viewportModel->getForegroundOptions();
                    options.missingIndicator.enabled = value;
                    viewportModel->setForegroundOptions(options);
                });

            p.missingIndicatorWidthEdit->setCallback(
                [viewportModel](int value)
                {
                    auto options = viewportModel->getForegroundOptions();
                    options.missingIndicator.width = value;
                    viewportModel->setForegroundOptions(options);
                });

            p.missingIndicatorColorSwatch->setCallback(
                [viewportModel](const ftk::Color4F& value)
                {
                    auto options = viewportModel->getForegroundOptions();
                    options.missingIndicator.color = value;
                    viewportModel->setForegroundOptions(options);
                });

            p.settingsObserver = ftk::Observer<models::ImageSeqSettings>::create(
                settings->observeImageSeq(),
                [this](const models::ImageSeqSettings& value)
                {
                    FTK_P();
                    p.audioComboBox->setCurrentIndex(static_cast<int>(value.audio));
                    p.audioExtensionsEdit->setText(ftk::join(value.audioExts, ' '));
                    p.layout->setRowVisible(p.audioExtensionsEdit, tl::ImageSeqAudio::Ext == value.audio);
                    p.audioFileNameEdit->setText(value.audioFileName);
                    p.layout->setRowVisible(p.audioFileNameEdit, tl::ImageSeqAudio::FileName == value.audio);
                    p.maxDigitsEdit->setValue(value.maxDigits);
                    p.defaultSpeedEdit->setValue(value.io.defaultSpeed);
                    p.missingFramesComboBox->setCurrentIndex(
                        static_cast<int>(value.io.missingFrames));
                    p.threadsEdit->setValue(value.readThreadCount);
                });

            p.audioComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    models::ImageSeqSettings settings = p.settings->getImageSeq();
                    settings.audio = static_cast<tl::ImageSeqAudio>(value);
                    p.settings->setImageSeq(settings);
                });

            p.audioExtensionsEdit->setCallback(
                [this](const std::string& value)
                {
                    FTK_P();
                    models::ImageSeqSettings settings = p.settings->getImageSeq();
                    settings.audioExts = ftk::split(value, ' ');
                    p.settings->setImageSeq(settings);
                });

            p.audioFileNameEdit->setCallback(
                [this](const std::string& value)
                {
                    FTK_P();
                    models::ImageSeqSettings settings = p.settings->getImageSeq();
                    settings.audioFileName = value;
                    p.settings->setImageSeq(settings);
                });

            p.maxDigitsEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    models::ImageSeqSettings settings = p.settings->getImageSeq();
                    settings.maxDigits = value;
                    p.settings->setImageSeq(settings);
                });

            p.defaultSpeedEdit->setCallback(
                [this](double value)
                {
                    FTK_P();
                    models::ImageSeqSettings settings = p.settings->getImageSeq();
                    settings.io.defaultSpeed = value;
                    p.settings->setImageSeq(settings);
                });

            p.missingFramesComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    models::ImageSeqSettings settings = p.settings->getImageSeq();
                    settings.io.missingFrames = static_cast<tl::MissingFrames>(value);
                    p.settings->setImageSeq(settings);
                });

            p.threadsEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    models::ImageSeqSettings settings = p.settings->getImageSeq();
                    settings.readThreadCount = value;
                    p.settings->setImageSeq(settings);
                });
        }

        ImageSeqSettingsWidget::ImageSeqSettingsWidget() :
            _p(new Private)
        {}

        ImageSeqSettingsWidget::~ImageSeqSettingsWidget()
        {}

        std::shared_ptr<ImageSeqSettingsWidget> ImageSeqSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ImageSeqSettingsWidget>(new ImageSeqSettingsWidget);
            out->_init(context, settings, viewportModel, parent);
            return out;
        }

        struct MiscSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::CheckBox> tooltipsCheckBox;
            std::shared_ptr<ftk::CheckBox> showSetupCheckBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<models::MiscSettings> > settingsObserver;
        };

        void MiscSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::MiscSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.tooltipsCheckBox = ftk::CheckBox::create(context);
            p.tooltipsCheckBox->setHStretch(ftk::Stretch::Expanding);

            p.showSetupCheckBox = ftk::CheckBox::create(context);
            p.showSetupCheckBox->setHStretch(ftk::Stretch::Expanding);

            p.layout = ftk::FormLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Enable tooltips:", p.tooltipsCheckBox);
            p.layout->addRow("Show setup dialog:", p.showSetupCheckBox);

            p.settingsObserver = ftk::Observer<models::MiscSettings>::create(
                settings->observeMisc(),
                [this](const models::MiscSettings& value)
                {
                    FTK_P();
                    p.tooltipsCheckBox->setChecked(value.tooltipsEnabled);
                    p.showSetupCheckBox->setChecked(value.showSetup);
                });

            p.tooltipsCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    auto settings = p.settings->getMisc();
                    settings.tooltipsEnabled = value;
                    p.settings->setMisc(settings);
                });

            p.showSetupCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    auto settings = p.settings->getMisc();
                    settings.showSetup = value;
                    p.settings->setMisc(settings);
                });
        }

        MiscSettingsWidget::MiscSettingsWidget() :
            _p(new Private)
        {}

        MiscSettingsWidget::~MiscSettingsWidget()
        {}

        std::shared_ptr<MiscSettingsWidget> MiscSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<MiscSettingsWidget>(new MiscSettingsWidget);
            out->_init(context, settings, parent);
            return out;
        }

        struct MouseSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;
            std::vector<std::string> buttonLabels;
            std::vector<ftk::KeyModifier> modifiers;
            std::vector<std::string> modifierLabels;
            std::shared_ptr<ftk::FloatEdit> wheelScaleEdit;
            std::shared_ptr<ftk::FloatEdit> frameShuttleScaleEdit;
            std::map<models::MouseAction, std::shared_ptr<ftk::ComboBox> > buttonComboBoxes;
            std::map<models::MouseAction, std::shared_ptr<ftk::ComboBox> > modifierComboBoxes;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<models::MouseSettings> > settingsObserver;
        };

        void MouseSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::MouseSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.modifiers.push_back(ftk::KeyModifier::None);
            p.modifiers.push_back(ftk::KeyModifier::Shift);
            p.modifiers.push_back(ftk::KeyModifier::Control);
            p.modifiers.push_back(ftk::KeyModifier::Alt);
#if defined(__APPLE__)
            p.modifiers.push_back(ftk::KeyModifier::Super);
#endif // __APPLE__
            p.modifierLabels.push_back(ftk::to_string(ftk::KeyModifier::None));
            p.modifierLabels.push_back(ftk::to_string(ftk::KeyModifier::Shift));
            p.modifierLabels.push_back(ftk::to_string(ftk::KeyModifier::Control));
            p.modifierLabels.push_back(ftk::to_string(ftk::KeyModifier::Alt));
#if defined(__APPLE__)
            p.modifierLabels.push_back(ftk::to_string(ftk::KeyModifier::Super));
#endif // __APPLE__

            for (const auto mouseAction : models::getMouseActionEnums())
            {
                p.buttonComboBoxes[mouseAction] = ftk::ComboBox::create(
                    context,
                    ftk::getMouseButtonLabels());
                p.buttonComboBoxes[mouseAction]->setHStretch(ftk::Stretch::Expanding);
                p.modifierComboBoxes[mouseAction] = ftk::ComboBox::create(context, p.modifierLabels);
                p.modifierComboBoxes[mouseAction]->setHStretch(ftk::Stretch::Expanding);
            }

            p.wheelScaleEdit = ftk::FloatEdit::create(context);
            p.wheelScaleEdit->setRange(.5F, 5.F);
            ftk::setScreenshotTag(p.wheelScaleEdit, "Mouse.WheelScale");

            p.frameShuttleScaleEdit = ftk::FloatEdit::create(context);
            p.frameShuttleScaleEdit->setRange(.1F, 10.F);
            ftk::setScreenshotTag(p.frameShuttleScaleEdit, "Mouse.FrameShuttleScale");

            p.layout = ftk::FormLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            std::map<models::MouseAction, std::string> mouseActionLabels =
            {
                { models::MouseAction::PanView, "Pan view" },
                { models::MouseAction::CompareWipe, "Compare wipe" },
                { models::MouseAction::Pick, "Pick" },
                { models::MouseAction::FrameShuttle, "Frame shuttle" }
            };
            std::map<models::MouseAction, std::string> mouseActionScreenshots =
            {
                { models::MouseAction::PanView, "Mouse.PanView" },
                { models::MouseAction::CompareWipe, "Mouse.CompareWipe" },
                { models::MouseAction::Pick, "Mouse.Pick" },
                { models::MouseAction::FrameShuttle, "Mouse.FrameShuttle" }
            };
            for (const auto mouseAction : models::getMouseActionEnums())
            {
                auto hLayout = ftk::HorizontalLayout::create(context);
                hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
                hLayout->setHStretch(ftk::Stretch::Expanding);
                p.buttonComboBoxes[mouseAction]->setParent(hLayout);
                p.modifierComboBoxes[mouseAction]->setParent(hLayout);
                p.layout->addRow(ftk::Format("{0}:").
                    arg(mouseActionLabels[mouseAction]),
                    hLayout);
                ftk::setScreenshotTag(hLayout, mouseActionScreenshots[mouseAction]);
            }
            p.layout->addRow("Wheel scale:", p.wheelScaleEdit);
            p.layout->addRow("Frame shuttle scale:", p.frameShuttleScaleEdit);

            p.settingsObserver = ftk::Observer<models::MouseSettings>::create(
                settings->observeMouse(),
                [this](const models::MouseSettings& value)
                {
                    FTK_P();
                    for (const auto& i : value.bindings)
                    {
                        if (auto j = p.buttonComboBoxes.find(i.first);
                            j != p.buttonComboBoxes.end())
                        {
                            j->second->setCurrentIndex(static_cast<int>(i.second.button));
                        }
                        if (auto j = p.modifierComboBoxes.find(i.first);
                            j != p.modifierComboBoxes.end())
                        {
                            const auto k = std::find(p.modifiers.begin(), p.modifiers.end(), i.second.modifier);
                            if (k != p.modifiers.end())
                            {

                                j->second->setCurrentIndex(k - p.modifiers.begin());
                            }
                        }
                    }
                    p.wheelScaleEdit->setValue(value.wheelScale);
                    p.frameShuttleScaleEdit->setValue(value.frameShuttleScale);
                });

            for (const auto mouseAction : models::getMouseActionEnums())
            {
                p.buttonComboBoxes[mouseAction]->setIndexCallback(
                    [this, mouseAction](int index)
                    {
                        FTK_P();
                        auto settings = p.settings->getMouse();
                        settings.bindings[mouseAction].button = static_cast<ftk::MouseButton>(index);
                        p.settings->setMouse(settings);
                    });
                p.modifierComboBoxes[mouseAction]->setIndexCallback(
                    [this, mouseAction](int index)
                    {
                        FTK_P();
                        if (index >= 0 && index < static_cast<int>(p.modifiers.size()))
                        {
                            auto settings = p.settings->getMouse();
                            settings.bindings[mouseAction].modifier = p.modifiers[index];
                            p.settings->setMouse(settings);
                        }
                    });
            }

            p.wheelScaleEdit->setCallback(
                [this](float value)
                {
                    FTK_P();
                    auto settings = p.settings->getMouse();
                    settings.wheelScale = value;
                    p.settings->setMouse(settings);
                });

            p.frameShuttleScaleEdit->setCallback(
                [this](float value)
                {
                    FTK_P();
                    auto settings = p.settings->getMouse();
                    settings.frameShuttleScale = value;
                    p.settings->setMouse(settings);
                });
        }

        MouseSettingsWidget::MouseSettingsWidget() :
            _p(new Private)
        {}

        MouseSettingsWidget::~MouseSettingsWidget()
        {}

        std::shared_ptr<MouseSettingsWidget> MouseSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<MouseSettingsWidget>(new MouseSettingsWidget);
            out->_init(context, settings, parent);
            return out;
        }

        struct PlaybackSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;
            std::shared_ptr<ftk::CheckBox> startPlaybackCheckBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<models::PlaybackSettings> > settingsObserver;
        };

        void PlaybackSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::PlaybackSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.startPlaybackCheckBox = ftk::CheckBox::create(context);

            p.layout = ftk::FormLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Start playback on open:", p.startPlaybackCheckBox);

            p.settingsObserver = ftk::Observer<models::PlaybackSettings>::create(
                settings->observePlayback(),
                [this](const models::PlaybackSettings& value)
                {
                    FTK_P();
                    p.startPlaybackCheckBox->setChecked(value.startPlayback);
                });

            p.startPlaybackCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    auto settings = p.settings->getPlayback();
                    settings.startPlayback = value;
                    p.settings->setPlayback(settings);
                });
        }

        PlaybackSettingsWidget::PlaybackSettingsWidget() :
            _p(new Private)
        {}

        PlaybackSettingsWidget::~PlaybackSettingsWidget()
        {}

        std::shared_ptr<PlaybackSettingsWidget> PlaybackSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<PlaybackSettingsWidget>(new PlaybackSettingsWidget);
            out->_init(context, settings, parent);
            return out;
        }

        struct TimeSettingsWidget::Private
        {
            std::shared_ptr<models::TimeUnitsModel> timeUnitsModel;

            std::shared_ptr<ftk::ComboBox> timeUnitsComboBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::TimeUnits> > timeUnitsObserver;
        };

        void TimeSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::TimeSettingsWidget", parent);
            FTK_P();

            p.timeUnitsModel = timeUnitsModel;

            p.timeUnitsComboBox = ftk::ComboBox::create(context, tl::getTimeUnitsLabels());

            p.layout = ftk::FormLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Time units:", p.timeUnitsComboBox);

            p.timeUnitsComboBox->setIndexCallback(
                [this](int value)
                {
                    _p->timeUnitsModel->setTimeUnits(static_cast<tl::TimeUnits>(value));
                });

            p.timeUnitsObserver = ftk::Observer<tl::TimeUnits>::create(
                p.timeUnitsModel->observeTimeUnits(),
                [this](tl::TimeUnits value)
                {
                    _p->timeUnitsComboBox->setCurrentIndex(static_cast<int>(value));
                });

        }

        TimeSettingsWidget::TimeSettingsWidget() :
            _p(new Private)
        {}

        TimeSettingsWidget::~TimeSettingsWidget()
        {}

        std::shared_ptr<TimeSettingsWidget> TimeSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<TimeSettingsWidget>(new TimeSettingsWidget);
            out->_init(context, timeUnitsModel, parent);
            return out;
        }

#if defined(TLRENDER_FFMPEG_PLUGIN)
        struct FFmpegSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::CheckBox> yuvToRGBCheckBox;
            std::shared_ptr<ftk::CheckBox> hwAccelCheckBox;
            std::shared_ptr<ftk::IntEdit> threadsEdit;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::ffmpeg::Options> > optionsObserver;
        };

        void FFmpegSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::FFmpegSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.yuvToRGBCheckBox = ftk::CheckBox::create(context);
            p.yuvToRGBCheckBox->setHStretch(ftk::Stretch::Expanding);
            p.yuvToRGBCheckBox->setTooltip(
                "Convert YUV to RGB on the CPU when reading. When disabled, YUV "
                "frames are kept and converted on the GPU.");
            ftk::setScreenshotTag(p.yuvToRGBCheckBox, "FFmpeg.YUVtoRGB");

            p.hwAccelCheckBox = ftk::CheckBox::create(context);
            p.hwAccelCheckBox->setHStretch(ftk::Stretch::Expanding);
            p.hwAccelCheckBox->setTooltip(
                "Use the GPU to decode video when possible. Falls back to software "
                "decoding automatically when hardware decoding is unavailable for a "
                "file. Takes effect the next time a file is opened.");
            ftk::setScreenshotTag(p.hwAccelCheckBox, "FFmpeg.HardwareDecoding");

            p.threadsEdit = ftk::IntEdit::create(context);
            p.threadsEdit->setRange(0, 64);
            p.threadsEdit->setTooltip(
                "Number of threads used for decoding. Set to 0 to choose the thread "
                "count automatically.");
            ftk::setScreenshotTag(p.threadsEdit, "FFmpeg.Threads");

            p.layout = ftk::FormLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("YUV to RGB conversion:", p.yuvToRGBCheckBox);
            p.layout->addRow("Hardware decoding:", p.hwAccelCheckBox);
            p.layout->addRow("I/O threads:", p.threadsEdit);

            p.optionsObserver = ftk::Observer<tl::ffmpeg::Options>::create(
                settings->observeFFmpeg(),
                [this](const tl::ffmpeg::Options& value)
                {
                    FTK_P();
                    p.yuvToRGBCheckBox->setChecked(value.yuvToRgb);
                    p.hwAccelCheckBox->setChecked(value.hwAccel);
                    p.threadsEdit->setValue(value.threadCount);
                });

            p.yuvToRGBCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    tl::ffmpeg::Options options = p.settings->getFFmpeg();
                    options.yuvToRgb = value;
                    p.settings->setFFmpeg(options);
                });

            p.hwAccelCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    tl::ffmpeg::Options options = p.settings->getFFmpeg();
                    options.hwAccel = value;
                    p.settings->setFFmpeg(options);
                });

            p.threadsEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    tl::ffmpeg::Options options = p.settings->getFFmpeg();
                    options.threadCount = value;
                    p.settings->setFFmpeg(options);
                });
        }

        FFmpegSettingsWidget::FFmpegSettingsWidget() :
            _p(new Private)
        {}

        FFmpegSettingsWidget::~FFmpegSettingsWidget()
        {}

        std::shared_ptr<FFmpegSettingsWidget> FFmpegSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<FFmpegSettingsWidget>(new FFmpegSettingsWidget);
            out->_init(context, settings, parent);
            return out;
        }
#endif // TLRENDER_FFMPEG_PLUGIN

#if defined(TLRENDER_FFMPEG_CMD)
        struct FFmpegCmdSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::FileEdit> ffmpegEdit;
            std::shared_ptr<ftk::FileEdit> ffprobeEdit;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::ffmpeg_cmd::Options> > optionsObserver;
        };

        void FFmpegCmdSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::FFmpegCmdSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.ffmpegEdit = ftk::FileEdit::create(context);
            ftk::setScreenshotTag(p.ffmpegEdit, "FFmpegCmd.FFmpeg");

            p.ffprobeEdit = ftk::FileEdit::create(context);
            ftk::setScreenshotTag(p.ffprobeEdit, "FFmpegCmd.FFprobe");

            p.layout = ftk::FormLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("ffmpeg location:", p.ffmpegEdit);
            p.layout->addRow("ffprobe location:", p.ffprobeEdit);

            p.optionsObserver = ftk::Observer<tl::ffmpeg_cmd::Options>::create(
                settings->observeFFmpegCmd(),
                [this](const tl::ffmpeg_cmd::Options& value)
                {
                    FTK_P();
                    p.ffmpegEdit->setPath(ftk::Path(value.ffmpegPath));
                    p.ffprobeEdit->setPath(ftk::Path(value.ffprobePath));
                });

            p.ffmpegEdit->setCallback(
                [this](const ftk::Path& value)
                {
                    FTK_P();
                    tl::ffmpeg_cmd::Options options = p.settings->getFFmpegCmd();
                    options.ffmpegPath = value.get();
                    p.settings->setFFmpegCmd(options);
                });

            p.ffprobeEdit->setCallback(
                [this](const ftk::Path& value)
                {
                    FTK_P();
                    tl::ffmpeg_cmd::Options options = p.settings->getFFmpegCmd();
                    options.ffprobePath = value.get();
                    p.settings->setFFmpegCmd(options);
                });
        }

        FFmpegCmdSettingsWidget::FFmpegCmdSettingsWidget() :
            _p(new Private)
        {}

        FFmpegCmdSettingsWidget::~FFmpegCmdSettingsWidget()
        {}

        std::shared_ptr<FFmpegCmdSettingsWidget> FFmpegCmdSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<FFmpegCmdSettingsWidget>(new FFmpegCmdSettingsWidget);
            out->_init(context, settings, parent);
            return out;
        }
#endif // TLRENDER_FFMPEG_CMD

#if defined(TLRENDER_USD)
        struct USDSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::IntEdit> renderWidthEdit;
            std::shared_ptr<ftk::FloatEditSlider> complexitySlider;
            std::shared_ptr<ftk::ComboBox> drawModeComboBox;
            std::shared_ptr<ftk::CheckBox> lightingCheckBox;
            std::shared_ptr<ftk::CheckBox> sRGBCheckBox;
            std::shared_ptr<ftk::IntEdit> stageCacheEdit;
            std::shared_ptr<ftk::IntEdit> diskCacheEdit;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::usd::Options> > optionsObserver;
        };

        void USDSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::USDSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.renderWidthEdit = ftk::IntEdit::create(context);
            p.renderWidthEdit->setRange(1, 8192);

            p.complexitySlider = ftk::FloatEditSlider::create(context);

            p.drawModeComboBox = ftk::ComboBox::create(context, tl::usd::getDrawModeLabels());
            p.drawModeComboBox->setHStretch(ftk::Stretch::Expanding);

            p.lightingCheckBox = ftk::CheckBox::create(context);
            p.lightingCheckBox->setHStretch(ftk::Stretch::Expanding);

            p.sRGBCheckBox = ftk::CheckBox::create(context);
            p.sRGBCheckBox->setHStretch(ftk::Stretch::Expanding);

            p.stageCacheEdit = ftk::IntEdit::create(context);
            p.stageCacheEdit->setRange(0, 10);

            p.diskCacheEdit = ftk::IntEdit::create(context);
            p.diskCacheEdit->setRange(0, 1024);

            p.layout = ftk::FormLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Render width:", p.renderWidthEdit);
            p.layout->addRow("Render complexity:", p.complexitySlider);
            p.layout->addRow("Draw mode:", p.drawModeComboBox);
            p.layout->addRow("Enable lighting:", p.lightingCheckBox);
            p.layout->addRow("Enable sRGB color space:", p.sRGBCheckBox);
            p.layout->addRow("Stage cache size:", p.stageCacheEdit);
            auto hLayout = ftk::HorizontalLayout::create(context);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.diskCacheEdit->setParent(hLayout);
            ftk::Label::create(context, "GB", hLayout);
            p.layout->addRow("Disk cache size:", hLayout);

            p.optionsObserver = ftk::Observer<tl::usd::Options>::create(
                settings->observeUSD(),
                [this](const tl::usd::Options& value)
                {
                    FTK_P();
                    p.renderWidthEdit->setValue(value.renderWidth);
                    p.complexitySlider->setValue(value.complexity);
                    p.drawModeComboBox->setCurrentIndex(static_cast<int>(value.drawMode));
                    p.lightingCheckBox->setChecked(value.enableLighting);
                    p.sRGBCheckBox->setChecked(value.sRGB);
                    p.stageCacheEdit->setValue(value.stageCacheCount);
                    p.diskCacheEdit->setValue(value.diskCacheGB);
                });

            p.renderWidthEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    tl::usd::Options options = p.settings->getUSD();
                    options.renderWidth = value;
                    p.settings->setUSD(options);
                });

            p.complexitySlider->setCallback(
                [this](float value)
                {
                    FTK_P();
                    tl::usd::Options options = p.settings->getUSD();
                    options.complexity = value;
                    p.settings->setUSD(options);
                });

            p.drawModeComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    tl::usd::Options options = p.settings->getUSD();
                    options.drawMode = static_cast<tl::usd::DrawMode>(value);
                    p.settings->setUSD(options);
                });

            p.lightingCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    tl::usd::Options options = p.settings->getUSD();
                    options.enableLighting = value;
                    p.settings->setUSD(options);
                });

            p.sRGBCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    tl::usd::Options options = p.settings->getUSD();
                    options.sRGB = value;
                    p.settings->setUSD(options);
                });

            p.stageCacheEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    tl::usd::Options options = p.settings->getUSD();
                    options.stageCacheCount = value;
                    p.settings->setUSD(options);
                });

            p.diskCacheEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    tl::usd::Options options = p.settings->getUSD();
                    options.diskCacheGB = value;
                    p.settings->setUSD(options);
                });
        }

        USDSettingsWidget::USDSettingsWidget() :
            _p(new Private)
        {}

        USDSettingsWidget::~USDSettingsWidget()
        {}

        std::shared_ptr<USDSettingsWidget> USDSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<USDSettingsWidget>(new USDSettingsWidget);
            out->_init(context, settings, parent);
            return out;
        }
#endif // TLRENDER_USD
    }
}
