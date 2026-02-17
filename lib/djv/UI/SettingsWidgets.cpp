// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/SettingsWidgets.h>

#include <djv/Models/TimeUnitsModel.h>

#if defined(TLRENDER_USD)
#include <tlRender/IO/USD.h>
#endif // TLRENDER_USD

#include <ftk/UI/Bellows.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/DoubleEdit.h>
#include <ftk/UI/FloatEdit.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/IntEdit.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/LineEdit.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace ui
    {
        ISettingsWidget::~ISettingsWidget()
        {}

        struct AdvancedSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::CheckBox> compatCheckBox;
            std::shared_ptr<ftk::IntEdit> audioBufferFramesEdit;
            std::shared_ptr<ftk::IntEdit> videoRequestsEdit;
            std::shared_ptr<ftk::IntEdit> audioRequestsEdit;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<models::AdvancedSettings> > settingsObserver;
        };

        void AdvancedSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::AdvancedSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.compatCheckBox = ftk::CheckBox::create(context);
            p.compatCheckBox->setHStretch(ftk::Stretch::Expanding);
            p.compatCheckBox->setTooltip("Enable workarounds for timelines that may not conform exactly to specification.");

            p.audioBufferFramesEdit = ftk::IntEdit::create(context);
            p.audioBufferFramesEdit->setRange(1, 1000000);
            p.audioBufferFramesEdit->setStep(256);
            p.audioBufferFramesEdit->setLargeStep(1024);

            p.videoRequestsEdit = ftk::IntEdit::create(context);
            p.videoRequestsEdit->setRange(1, 64);

            p.audioRequestsEdit = ftk::IntEdit::create(context);
            p.audioRequestsEdit->setRange(1, 64);

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            auto label = ftk::Label::create(context, "Changes are applied to new files.", p.layout);
            auto formLayout = ftk::FormLayout::create(context, p.layout);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Compatibility:", p.compatCheckBox);
            formLayout->addRow("Audio buffer frames:", p.audioBufferFramesEdit);
            formLayout->addRow("Video requests:", p.videoRequestsEdit);
            formLayout->addRow("Audio requests:", p.audioRequestsEdit);

            p.settingsObserver = ftk::Observer<models::AdvancedSettings>::create(
                settings->observeAdvanced(),
                [this](const models::AdvancedSettings& value)
                {
                    FTK_P();
                    p.compatCheckBox->setChecked(value.compat);
                    p.audioBufferFramesEdit->setValue(value.audioBufferFrameCount);
                    p.videoRequestsEdit->setValue(value.videoRequestMax);
                    p.audioRequestsEdit->setValue(value.audioRequestMax);
                });

            p.compatCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    auto settings = p.settings->getAdvanced();
                    settings.compat = value;
                    p.settings->setAdvanced(settings);
                });

            p.audioBufferFramesEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    auto settings = p.settings->getAdvanced();
                    settings.audioBufferFrameCount = value;
                    p.settings->setAdvanced(settings);
                });

            p.videoRequestsEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    auto settings = p.settings->getAdvanced();
                    settings.videoRequestMax = value;
                    p.settings->setAdvanced(settings);
                });

            p.audioRequestsEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    auto settings = p.settings->getAdvanced();
                    settings.audioRequestMax = value;
                    p.settings->setAdvanced(settings);
                });
        }

        AdvancedSettingsWidget::AdvancedSettingsWidget() :
            _p(new Private)
        {}

        AdvancedSettingsWidget::~AdvancedSettingsWidget()
        {}

        std::shared_ptr<AdvancedSettingsWidget> AdvancedSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<AdvancedSettingsWidget>(new AdvancedSettingsWidget);
            out->_init(context, settings, parent);
            return out;
        }

        ftk::Size2I AdvancedSettingsWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void AdvancedSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        struct CacheSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::FloatEdit> videoEdit;
            std::shared_ptr<ftk::FloatEdit> audioEdit;
            std::shared_ptr<ftk::FloatEdit> readBehindEdit;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::PlayerCacheOptions> > settingsObserver;
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

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Video cache (GB):", p.videoEdit);
            p.layout->addRow("Audio cache (GB):", p.audioEdit);
            p.layout->addRow("Read behind (seconds):", p.readBehindEdit);

            p.settingsObserver = ftk::Observer<tl::PlayerCacheOptions>::create(
                settings->observeCache(),
                [this](const tl::PlayerCacheOptions& value)
                {
                    FTK_P();
                    p.videoEdit->setValue(value.videoGB);
                    p.audioEdit->setValue(value.audioGB);
                    p.readBehindEdit->setValue(value.readBehind);
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
                    tl::PlayerCacheOptions settings = p.settings->getCache();
                    settings.readBehind = value;
                    p.settings->setCache(settings);
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

        ftk::Size2I CacheSettingsWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void CacheSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

#if defined(FTK_NFD)
        struct FileBrowserSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::CheckBox> nfdCheckBox;
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

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->addRow("Native file dialog:", p.nfdCheckBox);

            p.settingsObserver = ftk::Observer<models::FileBrowserSettings>::create(
                settings->observeFileBrowser(),
                [this](const models::FileBrowserSettings& value)
                {
                    FTK_P();
                    p.nfdCheckBox->setChecked(value.nativeFileDialog);
                });

            p.nfdCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    auto settings = p.settings->getFileBrowser();
                    settings.nativeFileDialog = value;
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

        ftk::Size2I FileBrowserSettingsWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void FileBrowserSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }
#endif // FTK_NFD

        struct ImageSeqSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::ComboBox> audioComboBox;
            std::shared_ptr<ftk::LineEdit> audioExtensionsEdit;
            std::shared_ptr<ftk::LineEdit> audioFileNameEdit;
            std::shared_ptr<ftk::IntEdit> maxDigitsEdit;
            std::shared_ptr<ftk::DoubleEdit> defaultSpeedEdit;
            std::shared_ptr<ftk::IntEdit> threadsEdit;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<models::ImageSeqSettings> > settingsObserver;
        };

        void ImageSeqSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
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

            p.threadsEdit = ftk::IntEdit::create(context);
            p.threadsEdit->setRange(1, 64);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Open audio files:", p.audioComboBox);
            p.layout->addRow("Audio file extensions:", p.audioExtensionsEdit);
            p.layout->addRow("Audio file name:", p.audioFileNameEdit);
            p.layout->addRow("Maximum digits:", p.maxDigitsEdit);
            p.layout->addRow("Default speed (FPS):", p.defaultSpeedEdit);
            p.layout->addRow("I/O threads:", p.threadsEdit);

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
                    p.threadsEdit->setValue(value.io.threadCount);
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

            p.threadsEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    models::ImageSeqSettings settings = p.settings->getImageSeq();
                    settings.io.threadCount = value;
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
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ImageSeqSettingsWidget>(new ImageSeqSettingsWidget);
            out->_init(context, settings, parent);
            return out;
        }

        ftk::Size2I ImageSeqSettingsWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void ImageSeqSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
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

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
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

        ftk::Size2I MiscSettingsWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void MiscSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
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

            p.frameShuttleScaleEdit = ftk::FloatEdit::create(context);
            p.frameShuttleScaleEdit->setRange(.1F, 10.F);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            for (const auto mouseAction : models::getMouseActionEnums())
            {
                auto hLayout = ftk::HorizontalLayout::create(context);
                hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
                hLayout->setHStretch(ftk::Stretch::Expanding);
                p.buttonComboBoxes[mouseAction]->setParent(hLayout);
                p.modifierComboBoxes[mouseAction]->setParent(hLayout);
                p.layout->addRow(ftk::Format("{0}:").
                    arg(models::getMouseActionLabels()[static_cast<size_t>(mouseAction)]),
                    hLayout);
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
                        auto j = p.buttonComboBoxes.find(i.first);
                        if (j != p.buttonComboBoxes.end())
                        {
                            j->second->setCurrentIndex(static_cast<int>(i.second.button));
                        }
                        j = p.modifierComboBoxes.find(i.first);
                        if (j != p.modifierComboBoxes.end())
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
                        if (index >= 0 && index < p.modifiers.size())
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

        ftk::Size2I MouseSettingsWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void MouseSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
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

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
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

        ftk::Size2I TimeSettingsWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void TimeSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

#if defined(TLRENDER_FFMPEG)
        struct FFmpegSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            std::shared_ptr<ftk::CheckBox> yuvToRGBCheckBox;
            std::shared_ptr<ftk::IntEdit> threadsEdit;
            std::shared_ptr<ftk::VerticalLayout> layout;

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

            p.threadsEdit = ftk::IntEdit::create(context);
            p.threadsEdit->setRange(0, 64);

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            auto label = ftk::Label::create(
                context,
                "FFmpeg plugin settings.",
                p.layout);
            label = ftk::Label::create(
                context,
                "Changes are applied to new files.",
                p.layout);
            auto formLayout = ftk::FormLayout::create(context, p.layout);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("YUV to RGB conversion:", p.yuvToRGBCheckBox);
            formLayout->addRow("I/O threads:", p.threadsEdit);

            p.optionsObserver = ftk::Observer<tl::ffmpeg::Options>::create(
                settings->observeFFmpeg(),
                [this](const tl::ffmpeg::Options& value)
                {
                    FTK_P();
                    p.yuvToRGBCheckBox->setChecked(value.yuvToRgb);
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

        ftk::Size2I FFmpegSettingsWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void FFmpegSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

#endif // TLRENDER_FFMPEG

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
            std::shared_ptr<ftk::VerticalLayout> layout;

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

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            auto label = ftk::Label::create(
                context,
                "Universal scene description (USD) plugin settings.",
                p.layout);
            label = ftk::Label::create(
                context,
                "Changes are applied to new files.",
                p.layout);
            auto formLayout = ftk::FormLayout::create(context, p.layout);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Render width:", p.renderWidthEdit);
            formLayout->addRow("Render complexity:", p.complexitySlider);
            formLayout->addRow("Draw mode:", p.drawModeComboBox);
            formLayout->addRow("Enable lighting:", p.lightingCheckBox);
            formLayout->addRow("Enable sRGB color space:", p.sRGBCheckBox);
            formLayout->addRow("Stage cache size:", p.stageCacheEdit);
            formLayout->addRow("Disk cache size (GB):", p.diskCacheEdit);

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

        ftk::Size2I USDSettingsWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void USDSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }
#endif // TLRENDER_USD
    }
}
