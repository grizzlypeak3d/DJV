// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/SettingsToolPrivate.h>

#include <djvApp/Models/TimeUnitsModel.h>
#include <djvApp/App.h>

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
    namespace app
    {
        ISettingsWidget::~ISettingsWidget()
        {}

        struct AdvancedSettingsWidget::Private
        {
            std::shared_ptr<SettingsModel> model;

            std::shared_ptr<ftk::CheckBox> compatCheckBox;
            std::shared_ptr<ftk::IntEdit> audioBufferFramesEdit;
            std::shared_ptr<ftk::IntEdit> videoRequestsEdit;
            std::shared_ptr<ftk::IntEdit> audioRequestsEdit;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<AdvancedSettings> > settingsObserver;
        };

        void AdvancedSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::app::AdvancedSettingsWidget", parent);
            FTK_P();

            p.model = app->getSettingsModel();

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
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto label = ftk::Label::create(context, "Changes are applied to new files.", p.layout);
            auto formLayout = ftk::FormLayout::create(context, p.layout);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Compatibility:", p.compatCheckBox);
            formLayout->addRow("Audio buffer frames:", p.audioBufferFramesEdit);
            formLayout->addRow("Video requests:", p.videoRequestsEdit);
            formLayout->addRow("Audio requests:", p.audioRequestsEdit);

            p.settingsObserver = ftk::Observer<AdvancedSettings>::create(
                p.model->observeAdvanced(),
                [this](const AdvancedSettings& value)
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
                    auto settings = p.model->getAdvanced();
                    settings.compat = value;
                    p.model->setAdvanced(settings);
                });

            p.audioBufferFramesEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    auto settings = p.model->getAdvanced();
                    settings.audioBufferFrameCount = value;
                    p.model->setAdvanced(settings);
                });

            p.videoRequestsEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    auto settings = p.model->getAdvanced();
                    settings.videoRequestMax = value;
                    p.model->setAdvanced(settings);
                });

            p.audioRequestsEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    auto settings = p.model->getAdvanced();
                    settings.audioRequestMax = value;
                    p.model->setAdvanced(settings);
                });
        }

        AdvancedSettingsWidget::AdvancedSettingsWidget() :
            _p(new Private)
        {}

        AdvancedSettingsWidget::~AdvancedSettingsWidget()
        {}

        std::shared_ptr<AdvancedSettingsWidget> AdvancedSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<AdvancedSettingsWidget>(new AdvancedSettingsWidget);
            out->_init(context, app, parent);
            return out;
        }

        void AdvancedSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void AdvancedSettingsWidget::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            ISettingsWidget::sizeHintEvent(event);
            setSizeHint(_p->layout->getSizeHint());
        }

        struct CacheSettingsWidget::Private
        {
            std::shared_ptr<SettingsModel> model;

            std::shared_ptr<ftk::FloatEdit> videoEdit;
            std::shared_ptr<ftk::FloatEdit> audioEdit;
            std::shared_ptr<ftk::FloatEdit> readBehindEdit;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::timeline::PlayerCacheOptions> > settingsObserver;
        };

        void CacheSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::app::CacheSettingsWidget", parent);
            FTK_P();

            p.model = app->getSettingsModel();

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

            p.settingsObserver = ftk::Observer<tl::timeline::PlayerCacheOptions>::create(
                p.model->observeCache(),
                [this](const tl::timeline::PlayerCacheOptions& value)
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
                    tl::timeline::PlayerCacheOptions settings = p.model->getCache();
                    settings.videoGB = value;
                    p.model->setCache(settings);
                });

            p.audioEdit->setCallback(
                [this](float value)
                {
                    FTK_P();
                    tl::timeline::PlayerCacheOptions settings = p.model->getCache();
                    settings.audioGB = value;
                    p.model->setCache(settings);
                });

            p.readBehindEdit->setCallback(
                [this](float value)
                {
                    FTK_P();
                    tl::timeline::PlayerCacheOptions settings = p.model->getCache();
                    settings.readBehind = value;
                    p.model->setCache(settings);
                });
        }

        CacheSettingsWidget::CacheSettingsWidget() :
            _p(new Private)
        {}

        CacheSettingsWidget::~CacheSettingsWidget()
        {}

        std::shared_ptr<CacheSettingsWidget> CacheSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<CacheSettingsWidget>(new CacheSettingsWidget);
            out->_init(context, app, parent);
            return out;
        }

        void CacheSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void CacheSettingsWidget::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            ISettingsWidget::sizeHintEvent(event);
            setSizeHint(_p->layout->getSizeHint());
        }

        struct FileBrowserSettingsWidget::Private
        {
            std::shared_ptr<SettingsModel> model;

            std::shared_ptr<ftk::CheckBox> nfdCheckBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<FileBrowserSettings> > settingsObserver;
        };

        void FileBrowserSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::app::FileBrowserSettingsWidget", parent);
            FTK_P();

            p.model = app->getSettingsModel();

            p.nfdCheckBox = ftk::CheckBox::create(context);
            p.nfdCheckBox->setHStretch(ftk::Stretch::Expanding);
#if !defined(FTK_NFD)
            p.nfdCheckBox->setEnabled(false);
#endif // FTK_NFD

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Native file dialog:", p.nfdCheckBox);

            p.settingsObserver = ftk::Observer<FileBrowserSettings>::create(
                p.model->observeFileBrowser(),
                [this](const FileBrowserSettings& value)
                {
                    FTK_P();
                    p.nfdCheckBox->setChecked(value.nativeFileDialog);
                });

            p.nfdCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    auto settings = p.model->getFileBrowser();
                    settings.nativeFileDialog = value;
                    p.model->setFileBrowser(settings);
                });
        }

        FileBrowserSettingsWidget::FileBrowserSettingsWidget() :
            _p(new Private)
        {}

        FileBrowserSettingsWidget::~FileBrowserSettingsWidget()
        {}

        std::shared_ptr<FileBrowserSettingsWidget> FileBrowserSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<FileBrowserSettingsWidget>(new FileBrowserSettingsWidget);
            out->_init(context, app, parent);
            return out;
        }

        void FileBrowserSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void FileBrowserSettingsWidget::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            ISettingsWidget::sizeHintEvent(event);
            setSizeHint(_p->layout->getSizeHint());
        }

        struct ImageSeqSettingsWidget::Private
        {
            std::shared_ptr<SettingsModel> model;

            std::shared_ptr<ftk::ComboBox> audioComboBox;
            std::shared_ptr<ftk::LineEdit> audioExtensionsEdit;
            std::shared_ptr<ftk::LineEdit> audioFileNameEdit;
            std::shared_ptr<ftk::IntEdit> maxDigitsEdit;
            std::shared_ptr<ftk::DoubleEdit> defaultSpeedEdit;
            std::shared_ptr<ftk::IntEdit> threadsEdit;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<ImageSeqSettings> > settingsObserver;
        };

        void ImageSeqSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::app::ImageSeqSettingsWidget", parent);
            FTK_P();

            p.model = app->getSettingsModel();

            p.audioComboBox = ftk::ComboBox::create(context, tl::timeline::getImageSeqAudioLabels());
            p.audioComboBox->setHStretch(ftk::Stretch::Expanding);
            p.audioComboBox->setTooltip("How audio files are opened for image sequences.");

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
            p.maxDigitsEdit->setTooltip("Maximum number of digits allowed in a frame number.");

            p.defaultSpeedEdit = ftk::DoubleEdit::create(context);
            p.defaultSpeedEdit->setRange(1.0, 120.0);

            p.threadsEdit = ftk::IntEdit::create(context);
            p.threadsEdit->setRange(1, 64);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Audio:", p.audioComboBox);
            p.layout->addRow("Audio extensions:", p.audioExtensionsEdit);
            p.layout->addRow("Audio file name:", p.audioFileNameEdit);
            p.layout->addRow("Maximum digits:", p.maxDigitsEdit);
            p.layout->addRow("Default speed (FPS):", p.defaultSpeedEdit);
            p.layout->addRow("I/O threads:", p.threadsEdit);

            p.settingsObserver = ftk::Observer<ImageSeqSettings>::create(
                p.model->observeImageSeq(),
                [this](const ImageSeqSettings& value)
                {
                    FTK_P();
                    p.audioComboBox->setCurrentIndex(static_cast<int>(value.audio));
                    p.audioExtensionsEdit->setText(ftk::join(value.audioExts, ' '));
                    p.layout->setRowVisible(p.audioExtensionsEdit, tl::timeline::ImageSeqAudio::Ext == value.audio);
                    p.audioFileNameEdit->setText(value.audioFileName);
                    p.layout->setRowVisible(p.audioFileNameEdit, tl::timeline::ImageSeqAudio::FileName == value.audio);
                    p.maxDigitsEdit->setValue(value.maxDigits);
                    p.defaultSpeedEdit->setValue(value.io.defaultSpeed);
                    p.threadsEdit->setValue(value.io.threadCount);
                });

            p.audioComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    ImageSeqSettings settings = p.model->getImageSeq();
                    settings.audio = static_cast<tl::timeline::ImageSeqAudio>(value);
                    p.model->setImageSeq(settings);
                });

            p.audioExtensionsEdit->setTextCallback(
                [this](const std::string& value)
                {
                    FTK_P();
                    ImageSeqSettings settings = p.model->getImageSeq();
                    settings.audioExts = ftk::split(value, ' ');
                    p.model->setImageSeq(settings);
                });

            p.audioFileNameEdit->setTextCallback(
                [this](const std::string& value)
                {
                    FTK_P();
                    ImageSeqSettings settings = p.model->getImageSeq();
                    settings.audioFileName = value;
                    p.model->setImageSeq(settings);
                });

            p.maxDigitsEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    ImageSeqSettings settings = p.model->getImageSeq();
                    settings.maxDigits = value;
                    p.model->setImageSeq(settings);
                });

            p.defaultSpeedEdit->setCallback(
                [this](double value)
                {
                    FTK_P();
                    ImageSeqSettings settings = p.model->getImageSeq();
                    settings.io.defaultSpeed = value;
                    p.model->setImageSeq(settings);
                });

            p.threadsEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    ImageSeqSettings settings = p.model->getImageSeq();
                    settings.io.threadCount = value;
                    p.model->setImageSeq(settings);
                });
        }

        ImageSeqSettingsWidget::ImageSeqSettingsWidget() :
            _p(new Private)
        {}

        ImageSeqSettingsWidget::~ImageSeqSettingsWidget()
        {}

        std::shared_ptr<ImageSeqSettingsWidget> ImageSeqSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ImageSeqSettingsWidget>(new ImageSeqSettingsWidget);
            out->_init(context, app, parent);
            return out;
        }

        void ImageSeqSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void ImageSeqSettingsWidget::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            ISettingsWidget::sizeHintEvent(event);
            setSizeHint(_p->layout->getSizeHint());
        }

        struct MiscSettingsWidget::Private
        {
            std::shared_ptr<SettingsModel> model;

            std::shared_ptr<ftk::CheckBox> tooltipsCheckBox;
            std::shared_ptr<ftk::CheckBox> showSetupCheckBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<MiscSettings> > settingsObserver;
        };

        void MiscSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::app::MiscSettingsWidget", parent);
            FTK_P();

            p.model = app->getSettingsModel();

            p.tooltipsCheckBox = ftk::CheckBox::create(context);
            p.tooltipsCheckBox->setHStretch(ftk::Stretch::Expanding);

            p.showSetupCheckBox = ftk::CheckBox::create(context);
            p.showSetupCheckBox->setHStretch(ftk::Stretch::Expanding);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Enable tooltips:", p.tooltipsCheckBox);
            p.layout->addRow("Show setup dialog:", p.showSetupCheckBox);

            p.settingsObserver = ftk::Observer<MiscSettings>::create(
                p.model->observeMisc(),
                [this](const MiscSettings& value)
                {
                    FTK_P();
                    p.tooltipsCheckBox->setChecked(value.tooltipsEnabled);
                    p.showSetupCheckBox->setChecked(value.showSetup);
                });

            p.tooltipsCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    auto settings = p.model->getMisc();
                    settings.tooltipsEnabled = value;
                    p.model->setMisc(settings);
                });

            p.showSetupCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    auto settings = p.model->getMisc();
                    settings.showSetup = value;
                    p.model->setMisc(settings);
                });
        }

        MiscSettingsWidget::MiscSettingsWidget() :
            _p(new Private)
        {}

        MiscSettingsWidget::~MiscSettingsWidget()
        {}

        std::shared_ptr<MiscSettingsWidget> MiscSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<MiscSettingsWidget>(new MiscSettingsWidget);
            out->_init(context, app, parent);
            return out;
        }

        void MiscSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void MiscSettingsWidget::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            ISettingsWidget::sizeHintEvent(event);
            setSizeHint(_p->layout->getSizeHint());
        }

        struct MouseSettingsWidget::Private
        {
            std::shared_ptr<SettingsModel> model;
            std::vector<std::string> buttonLabels;
            std::vector<ftk::KeyModifier> modifiers;
            std::vector<std::string> modifierLabels;
            std::shared_ptr<ftk::FloatEdit> wheelScaleEdit;

            std::map<MouseAction, std::shared_ptr<ftk::ComboBox> > buttonComboBoxes;
            std::map<MouseAction, std::shared_ptr<ftk::ComboBox> > modifierComboBoxes;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<MouseSettings> > settingsObserver;
        };

        void MouseSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::app::MouseSettingsWidget", parent);
            FTK_P();

            p.model = app->getSettingsModel();

            std::vector<std::string> mouseActionLabels;
            mouseActionLabels.push_back("Pan view");
            mouseActionLabels.push_back("Compare wipe");
            mouseActionLabels.push_back("Color picker");
            mouseActionLabels.push_back("Frame shuttle");

            p.buttonLabels.push_back("None");
            p.buttonLabels.push_back("Left");
            p.buttonLabels.push_back("Middle");
            p.buttonLabels.push_back("Right");
            p.buttonLabels.push_back("Extra 1");
            p.buttonLabels.push_back("Extra 2");

            p.modifiers.push_back(ftk::KeyModifier::None);
            p.modifiers.push_back(ftk::KeyModifier::Shift);
            p.modifiers.push_back(ftk::KeyModifier::Control);
            p.modifiers.push_back(ftk::KeyModifier::Alt);
#if defined(__APPLE__)
            p.modifiers.push_back(ftk::KeyModifier::Super);
#endif // __APPLE__
            p.modifierLabels.push_back("None");
            p.modifierLabels.push_back(ftk::to_string(ftk::KeyModifier::Shift));
            p.modifierLabels.push_back(ftk::to_string(ftk::KeyModifier::Control));
            p.modifierLabels.push_back(ftk::to_string(ftk::KeyModifier::Alt));
#if defined(__APPLE__)
            p.modifierLabels.push_back(ftk::to_string(ftk::KeyModifier::Super));
#endif // __APPLE__

            for (const auto mouseAction : getMouseActionEnums())
            {
                p.buttonComboBoxes[mouseAction] = ftk::ComboBox::create(context, p.buttonLabels);
                p.buttonComboBoxes[mouseAction]->setHStretch(ftk::Stretch::Expanding);
                p.modifierComboBoxes[mouseAction] = ftk::ComboBox::create(context, p.modifierLabels);
                p.modifierComboBoxes[mouseAction]->setHStretch(ftk::Stretch::Expanding);
            }

            p.wheelScaleEdit = ftk::FloatEdit::create(context);
            p.wheelScaleEdit->setRange(1.01F, 4.F);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            for (const auto mouseAction : getMouseActionEnums())
            {
                auto hLayout = ftk::HorizontalLayout::create(context);
                hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
                hLayout->setHStretch(ftk::Stretch::Expanding);
                p.buttonComboBoxes[mouseAction]->setParent(hLayout);
                p.modifierComboBoxes[mouseAction]->setParent(hLayout);
                p.layout->addRow(ftk::Format("{0}:").arg(mouseActionLabels[static_cast<size_t>(mouseAction)]), hLayout);
            }
            p.layout->addRow("Wheel scale:", p.wheelScaleEdit);

            p.settingsObserver = ftk::Observer<MouseSettings>::create(
                p.model->observeMouse(),
                [this](const MouseSettings& value)
                {
                    FTK_P();
                    for (const auto& i : value.bindings)
                    {
                        auto j = p.buttonComboBoxes.find(i.first);
                        if (j != p.buttonComboBoxes.end())
                        {
                            j->second->setCurrentIndex(i.second.button);
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
                });

            for (const auto mouseAction : getMouseActionEnums())
            {
                p.buttonComboBoxes[mouseAction]->setIndexCallback(
                    [this, mouseAction](int index)
                    {
                        FTK_P();
                        auto settings = p.model->getMouse();
                        settings.bindings[mouseAction].button = index;
                        p.model->setMouse(settings);
                    });
                p.modifierComboBoxes[mouseAction]->setIndexCallback(
                    [this, mouseAction](int index)
                    {
                        FTK_P();
                        if (index >= 0 && index < p.modifiers.size())
                        {
                            auto settings = p.model->getMouse();
                            settings.bindings[mouseAction].modifier = p.modifiers[index];
                            p.model->setMouse(settings);
                        }
                    });
            }

            p.wheelScaleEdit->setCallback(
                [this](float value)
                {
                    FTK_P();
                    auto settings = p.model->getMouse();
                    settings.wheelScale = value;
                    p.model->setMouse(settings);
                });
        }

        MouseSettingsWidget::MouseSettingsWidget() :
            _p(new Private)
        {}

        MouseSettingsWidget::~MouseSettingsWidget()
        {}

        std::shared_ptr<MouseSettingsWidget> MouseSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<MouseSettingsWidget>(new MouseSettingsWidget);
            out->_init(context, app, parent);
            return out;
        }

        void MouseSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void MouseSettingsWidget::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            ISettingsWidget::sizeHintEvent(event);
            setSizeHint(_p->layout->getSizeHint());
        }

        struct TimeSettingsWidget::Private
        {
            std::shared_ptr<TimeUnitsModel> timeUnitsModel;

            std::shared_ptr<ftk::ComboBox> timeUnitsComboBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::timeline::TimeUnits> > timeUnitsObserver;
        };

        void TimeSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::app::TimeSettingsWidget", parent);
            FTK_P();

            p.timeUnitsModel = app->getTimeUnitsModel();

            p.timeUnitsComboBox = ftk::ComboBox::create(context, tl::timeline::getTimeUnitsLabels());

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Time units:", p.timeUnitsComboBox);

            p.timeUnitsComboBox->setIndexCallback(
                [this](int value)
                {
                    _p->timeUnitsModel->setTimeUnits(static_cast<tl::timeline::TimeUnits>(value));
                });

            p.timeUnitsObserver = ftk::Observer<tl::timeline::TimeUnits>::create(
                p.timeUnitsModel->observeTimeUnits(),
                [this](tl::timeline::TimeUnits value)
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
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<TimeSettingsWidget>(new TimeSettingsWidget);
            out->_init(context, app, parent);
            return out;
        }

        void TimeSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void TimeSettingsWidget::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            ISettingsWidget::sizeHintEvent(event);
            setSizeHint(_p->layout->getSizeHint());
        }

#if defined(TLRENDER_FFMPEG)
        struct FFmpegSettingsWidget::Private
        {
            std::shared_ptr<SettingsModel> model;

            std::shared_ptr<ftk::CheckBox> yuvToRGBCheckBox;
            std::shared_ptr<ftk::IntEdit> threadsEdit;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<tl::ffmpeg::Options> > optionsObserver;
        };

        void FFmpegSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::app::FFmpegSettingsWidget", parent);
            FTK_P();

            p.model = app->getSettingsModel();

            p.yuvToRGBCheckBox = ftk::CheckBox::create(context);
            p.yuvToRGBCheckBox->setHStretch(ftk::Stretch::Expanding);

            p.threadsEdit = ftk::IntEdit::create(context);
            p.threadsEdit->setRange(0, 64);

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto label = ftk::Label::create(context, "Changes are applied to new files.", p.layout);
            auto formLayout = ftk::FormLayout::create(context, p.layout);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("YUV to RGB conversion:", p.yuvToRGBCheckBox);
            formLayout->addRow("I/O threads:", p.threadsEdit);

            p.optionsObserver = ftk::Observer<tl::ffmpeg::Options>::create(
                p.model->observeFFmpeg(),
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
                    tl::ffmpeg::Options options = p.model->getFFmpeg();
                    options.yuvToRgb = value;
                    p.model->setFFmpeg(options);
                });

            p.threadsEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    tl::ffmpeg::Options options = p.model->getFFmpeg();
                    options.threadCount = value;
                    p.model->setFFmpeg(options);
                });
        }

        FFmpegSettingsWidget::FFmpegSettingsWidget() :
            _p(new Private)
        {}

        FFmpegSettingsWidget::~FFmpegSettingsWidget()
        {}

        std::shared_ptr<FFmpegSettingsWidget> FFmpegSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<FFmpegSettingsWidget>(new FFmpegSettingsWidget);
            out->_init(context, app, parent);
            return out;
        }

        void FFmpegSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void FFmpegSettingsWidget::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            ISettingsWidget::sizeHintEvent(event);
            setSizeHint(_p->layout->getSizeHint());
        }

#endif // TLRENDER_FFMPEG

#if defined(TLRENDER_USD)
        struct USDSettingsWidget::Private
        {
            std::shared_ptr<SettingsModel> model;

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
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::app::USDSettingsWidget", parent);
            FTK_P();

            p.model = app->getSettingsModel();

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
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto label = ftk::Label::create(context, "Changes are applied to new files.", p.layout);
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
                p.model->observeUSD(),
                [this](const tl::usd::Options& value)
                {
                    FTK_P();
                    p.renderWidthEdit->setValue(value.renderWidth);
                    p.complexitySlider->setValue(value.complexity);
                    p.drawModeComboBox->setCurrentIndex(static_cast<int>(value.drawMode));
                    p.lightingCheckBox->setChecked(value.enableLighting);
                    p.sRGBCheckBox->setChecked(value.sRGB);
                    p.stageCacheEdit->setValue(value.stageCache);
                    p.diskCacheEdit->setValue(value.diskCache);
                });

            p.renderWidthEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    tl::usd::Options options = p.model->getUSD();
                    options.renderWidth = value;
                    p.model->setUSD(options);
                });

            p.complexitySlider->setCallback(
                [this](float value)
                {
                    FTK_P();
                    tl::usd::Options options = p.model->getUSD();
                    options.complexity = value;
                    p.model->setUSD(options);
                });

            p.drawModeComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    tl::usd::Options options = p.model->getUSD();
                    options.drawMode = static_cast<tl::usd::DrawMode>(value);
                    p.model->setUSD(options);
                });

            p.lightingCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    tl::usd::Options options = p.model->getUSD();
                    options.enableLighting = value;
                    p.model->setUSD(options);
                });

            p.sRGBCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    tl::usd::Options options = p.model->getUSD();
                    options.sRGB = value;
                    p.model->setUSD(options);
                });

            p.stageCacheEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    tl::usd::Options options = p.model->getUSD();
                    options.stageCache = value;
                    p.model->setUSD(options);
                });

            p.diskCacheEdit->setCallback(
                [this](int value)
                {
                    FTK_P();
                    tl::usd::Options options = p.model->getUSD();
                    options.diskCache = value;
                    p.model->setUSD(options);
                });
        }

        USDSettingsWidget::USDSettingsWidget() :
            _p(new Private)
        {}

        USDSettingsWidget::~USDSettingsWidget()
        {}

        std::shared_ptr<USDSettingsWidget> USDSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<USDSettingsWidget>(new USDSettingsWidget);
            out->_init(context, app, parent);
            return out;
        }

        void USDSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void USDSettingsWidget::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            ISettingsWidget::sizeHintEvent(event);
            setSizeHint(_p->layout->getSizeHint());
        }
#endif // TLRENDER_USD

        struct SettingsTool::Private
        {
            std::shared_ptr<ftk::VerticalLayout> layout;
            std::shared_ptr<ftk::PushButton> saveButton;
            std::shared_ptr<ftk::PushButton> resetButton;
            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
        };

        void SettingsTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                Tool::Settings,
                "djv::app::SettingsTool",
                parent);
            FTK_P();

            auto advancedWidget = AdvancedSettingsWidget::create(context, app);
            auto cacheWidget = CacheSettingsWidget::create(context, app);
            auto fileBrowserWidget = FileBrowserSettingsWidget::create(context, app);
            auto imageSeqWidget = ImageSeqSettingsWidget::create(context, app);
            auto miscWidget = MiscSettingsWidget::create(context, app);
            auto mouseWidget = MouseSettingsWidget::create(context, app);
            auto shortcutsWidget = ShortcutsSettingsWidget::create(context, app);
            auto styleWidget = StyleSettingsWidget::create(context, app);
            auto timeWidget = TimeSettingsWidget::create(context, app);
#if defined(TLRENDER_FFMPEG)
            auto ffmpegWidget = FFmpegSettingsWidget::create(context, app);
#endif // TLRENDER_FFMPEG
#if defined(TLRENDER_USD)
            auto usdWidget = USDSettingsWidget::create(context, app);
#endif // TLRENDER_USD

            auto vLayout = ftk::VerticalLayout::create(context);
            vLayout->setSpacingRole(ftk::SizeRole::None);
            p.bellows["Cache"] = ftk::Bellows::create(context, "Cache", vLayout);
            p.bellows["Cache"]->setWidget(cacheWidget);
            p.bellows["FileBrowser"] = ftk::Bellows::create(context, "File Browser", vLayout);
            p.bellows["FileBrowser"]->setWidget(fileBrowserWidget);
            p.bellows["ImageSeqs"] = ftk::Bellows::create(context, "Image Sequences", vLayout);
            p.bellows["ImageSeqs"]->setWidget(imageSeqWidget);
            p.bellows["Misc"] = ftk::Bellows::create(context, "Miscellaneous", vLayout);
            p.bellows["Misc"]->setWidget(miscWidget);
            p.bellows["Mouse"] = ftk::Bellows::create(context, "Mouse", vLayout);
            p.bellows["Mouse"]->setWidget(mouseWidget);
            p.bellows["Shortcuts"] = ftk::Bellows::create(context, "Keyboard Shortcuts", vLayout);
            p.bellows["Shortcuts"]->setWidget(shortcutsWidget);
            p.bellows["Style"] = ftk::Bellows::create(context, "Style", vLayout);
            p.bellows["Style"]->setWidget(styleWidget);
            p.bellows["Time"] = ftk::Bellows::create(context, "Time", vLayout);
            p.bellows["Time"]->setWidget(timeWidget);
#if defined(TLRENDER_FFMPEG)
            p.bellows["FFmpeg"] = ftk::Bellows::create(context, "FFmpeg", vLayout);
            p.bellows["FFmpeg"]->setWidget(ffmpegWidget);
#endif // TLRENDER_USD
#if defined(TLRENDER_USD)
            p.bellows["USD"] = ftk::Bellows::create(context, "USD", vLayout);
            p.bellows["USD"]->setWidget(usdWidget);
#endif // TLRENDER_USD
            p.bellows["Advanced"] = ftk::Bellows::create(context, "Advanced", vLayout);
            p.bellows["Advanced"]->setWidget(advancedWidget);

            p.saveButton = ftk::PushButton::create(context, "Save");
            p.saveButton->setTooltip("Save the settings. Settings are also saved on exit.");

            p.resetButton = ftk::PushButton::create(context, "Reset");
            p.resetButton->setTooltip("Restore settings to default values.");

            p.layout = ftk::VerticalLayout::create(context);
            p.layout->setSpacingRole(ftk::SizeRole::None);
            auto scrollWidget = ftk::ScrollWidget::create(context, ftk::ScrollType::Both, p.layout);
            scrollWidget->setWidget(vLayout);
            scrollWidget->setBorder(false);
            scrollWidget->setVStretch(ftk::Stretch::Expanding);
            ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.saveButton->setParent(hLayout);
            hLayout->addSpacer(ftk::Stretch::Expanding);
            p.resetButton->setParent(hLayout);
            _setWidget(p.layout);

            _loadSettings(p.bellows);

            std::weak_ptr<App> appWeak(app);
            p.saveButton->setClickedCallback(
                [this, appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getSettingsModel()->save();
                    }
                });

            p.resetButton->setClickedCallback(
                [this, appWeak]
                {
                    if (auto context = getContext())
                    {
                        if (auto dialogSystem = context->getSystem<ftk::DialogSystem>())
                        {
                            dialogSystem->confirm(
                                "Reset Settings",
                                "Reset settings to default values?",
                                getWindow(),
                                [appWeak](bool value)
                                {
                                    if (value)
                                    {
                                        if (auto app = appWeak.lock())
                                        {
                                            app->getSettingsModel()->reset();
                                        }
                                    }
                                });
                        }
                    }
                });
        }

        SettingsTool::SettingsTool() :
            _p(new Private)
        {}

        SettingsTool::~SettingsTool()
        {
            _saveSettings(_p->bellows);
        }

        std::shared_ptr<SettingsTool> SettingsTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SettingsTool>(new SettingsTool);
            out->_init(context, app, parent);
            return out;
        }
    }
}
