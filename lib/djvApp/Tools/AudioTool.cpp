// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/AudioTool.h>

#include <djvApp/Models/AudioModel.h>
#include <djvApp/App.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/ButtonGroup.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/DoubleEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/IntEditSlider.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct AudioTool::Private
        {
            std::vector<tl::AudioDeviceID> devices;
            tl::AudioInfo info;
            std::vector<bool> channelMute;

            std::shared_ptr<ftk::ComboBox> deviceComboBox;
            std::shared_ptr<ftk::IntEditSlider> volumeSlider;
            std::shared_ptr<ftk::CheckBox> muteCheckBox;
            std::vector<std::shared_ptr<ftk::CheckBox> > channelMuteCheckBoxes;
            std::shared_ptr<ftk::ButtonGroup> channelMuteButtonGroup;
            std::shared_ptr<ftk::DoubleEditSlider> syncOffsetSlider;

            std::shared_ptr<ftk::HorizontalLayout> channelMuteLayout;

            std::shared_ptr<ftk::ListObserver<tl::AudioDeviceID> > devicesObserver;
            std::shared_ptr<ftk::Observer<tl::AudioDeviceID> > deviceObserver;
            std::shared_ptr<ftk::Observer<float> > volumeObserver;
            std::shared_ptr<ftk::Observer<bool> > muteObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::ListObserver<bool> > channelMuteObserver;
            std::shared_ptr<ftk::Observer<double> > syncOffsetObserver;
        };

        void AudioTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                Tool::Audio,
                "djv::app::AudioTool",
                parent);
            FTK_P();

            p.deviceComboBox = ftk::ComboBox::create(context);
            p.deviceComboBox->setTooltip("Audio output device");

            p.volumeSlider = ftk::IntEditSlider::create(context);
            p.volumeSlider->setRange(0, 100);
            p.volumeSlider->setStep(1);
            p.volumeSlider->setLargeStep(10);

            p.muteCheckBox = ftk::CheckBox::create(context);

            p.channelMuteButtonGroup = ftk::ButtonGroup::create(context, ftk::ButtonGroupType::Toggle);

            p.syncOffsetSlider = ftk::DoubleEditSlider::create(context);
            p.syncOffsetSlider->setRange(-1.0, 1.0);
            p.syncOffsetSlider->setDefaultValue(0.0);

            auto formLayout = ftk::FormLayout::create(context);
            formLayout->setMarginRole(ftk::SizeRole::Margin);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Device:", p.deviceComboBox);
            formLayout->addRow("Volume:", p.volumeSlider);
            formLayout->addRow("Mute:", p.muteCheckBox);
            p.channelMuteLayout = ftk::HorizontalLayout::create(context);
            p.channelMuteLayout->setSpacingRole(ftk::SizeRole::SpacingTool);
            formLayout->addRow("Channel mute:", p.channelMuteLayout);
            formLayout->addRow("Sync offset (seconds):", p.syncOffsetSlider);

            auto scrollWidget = ftk::ScrollWidget::create(context);
            scrollWidget->setBorder(false);
            scrollWidget->setWidget(formLayout);
            _setWidget(scrollWidget);

            auto appWeak = std::weak_ptr<App>(app);
            p.deviceComboBox->setIndexCallback(
                [this, appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        if (value >= 0 && value < _p->devices.size())
                        {
                            app->getAudioModel()->setDevice(
                                0 == value ? tl::AudioDeviceID() : _p->devices[value]);
                        }
                    }
                });

            p.volumeSlider->setCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getAudioModel()->setVolume(value / 100.F);
                    }
                });

            p.muteCheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getAudioModel()->setMute(value);
                    }
                });

            p.channelMuteButtonGroup->setCheckedCallback(
                [this, appWeak](int index, bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        std::vector<bool> channelMute = _p->channelMute;
                        if (index >= channelMute.size())
                        {
                            channelMute.resize(index + 1);
                        }
                        channelMute[index] = value;
                        app->getAudioModel()->setChannelMute(channelMute);
                    }
                });

            p.syncOffsetSlider->setCallback(
                [appWeak](double value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getAudioModel()->setSyncOffset(value);
                    }
                });

            p.devicesObserver = ftk::ListObserver<tl::AudioDeviceID>::create(
                app->getAudioModel()->observeDevices(),
                [this](const std::vector<tl::AudioDeviceID>& devices)
                {
                    _p->devices.clear();
                    _p->devices.push_back(tl::AudioDeviceID());
                    _p->devices.insert(_p->devices.end(), devices.begin(), devices.end());
                    std::vector<std::string> names;
                    names.push_back("Default");
                    for (const auto& device : devices)
                    {
                        names.push_back(device.name);
                    }
                    _p->deviceComboBox->setItems(names);
                });

            p.deviceObserver = ftk::Observer<tl::AudioDeviceID>::create(
                app->getAudioModel()->observeDevice(),
                [this](const tl::AudioDeviceID& value)
                {
                    int index = 0;
                    const auto i = std::find(_p->devices.begin(), _p->devices.end(), value);
                    if (i != _p->devices.end())
                    {
                        index = i - _p->devices.begin();
                    }
                    _p->deviceComboBox->setCurrentIndex(index);
                });

            p.volumeObserver = ftk::Observer<float>::create(
                app->getAudioModel()->observeVolume(),
                [this](float value)
                {
                    _p->volumeSlider->setValue(std::roundf(value * 100.F));
                });

            p.muteObserver = ftk::Observer<bool>::create(
                app->getAudioModel()->observeMute(),
                [this](bool value)
                {
                    _p->muteCheckBox->setChecked(value);
                });

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    _p->info = value ? value->getIOInfo().audio : tl::AudioInfo();
                    _widgetUpdate();
                });

            p.channelMuteObserver = ftk::ListObserver<bool>::create(
                app->getAudioModel()->observeChannelMute(),
                [this](const std::vector<bool>& value)
                {
                    _p->channelMute = value;
                    _widgetUpdate();
                });

            p.syncOffsetObserver = ftk::Observer<double>::create(
                app->getAudioModel()->observeSyncOffset(),
                [this](double value)
                {
                    _p->syncOffsetSlider->setValue(value);
                });
        }

        AudioTool::AudioTool() :
            _p(new Private)
        {}

        AudioTool::~AudioTool()
        {}

        std::shared_ptr<AudioTool> AudioTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<AudioTool>(new AudioTool);
            out->_init(context, app, parent);
            return out;
        }

        void AudioTool::_widgetUpdate()
        {
            FTK_P();
            if (p.channelMuteCheckBoxes.size() != p.info.channelCount)
            {
                for (const auto& checkBox : p.channelMuteCheckBoxes)
                {
                    checkBox->setParent(nullptr);
                }
                p.channelMuteCheckBoxes.clear();
                p.channelMuteButtonGroup->clearButtons();
                if (auto context = getContext())
                {
                    for (size_t i = 0; i < p.info.channelCount; ++i)
                    {
                        auto checkBox = ftk::CheckBox::create(
                            context,
                            ftk::Format("{0}").arg(1 + i),
                            p.channelMuteLayout);
                        p.channelMuteCheckBoxes.push_back(checkBox);
                        p.channelMuteButtonGroup->addButton(checkBox);
                    }
                }
            }
            for (size_t i = 0; i < p.channelMute.size() && i < p.channelMuteCheckBoxes.size(); ++i)
            {
                p.channelMuteCheckBoxes[i]->setChecked(p.channelMute[i]);
            }
        }
    }
}
