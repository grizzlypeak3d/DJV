// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/AudioTool.h>

#include <djv/App/App.h>
#include <djv/Models/AudioModel.h>
#include <djv/Models/FilesModel.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/DoubleEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/IntEditSlider.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct AudioTool::Private
        {
            std::vector<tl::AudioDeviceID> devices;
            tl::AudioInfo info;
            int channel = -1;

            std::shared_ptr<ftk::ComboBox> deviceComboBox;
            std::shared_ptr<ftk::IntEditSlider> volumeSlider;
            std::shared_ptr<ftk::CheckBox> muteCheckBox;
            std::shared_ptr<ftk::ComboBox> channelComboBox;
            std::shared_ptr<ftk::DoubleEditSlider> syncOffsetSlider;

            std::shared_ptr<ftk::ListObserver<tl::AudioDeviceID> > devicesObserver;
            std::shared_ptr<ftk::Observer<tl::AudioDeviceID> > deviceObserver;
            std::shared_ptr<ftk::Observer<float> > volumeObserver;
            std::shared_ptr<ftk::Observer<bool> > muteObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<int> > channelObserver;
            std::shared_ptr<ftk::Observer<double> > syncOffsetObserver;
        };

        void AudioTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                mainWindow,
                "Audio",
                "Audio",
                "djv::app::AudioTool",
                parent);
            FTK_P();

            p.deviceComboBox = ftk::ComboBox::create(context);
            p.deviceComboBox->setTooltip("Audio output device");
            ftk::setScreenshotTag(p.deviceComboBox, "Audio.Device");

            p.volumeSlider = ftk::IntEditSlider::create(context);
            p.volumeSlider->setRange(0, 100);
            p.volumeSlider->setStep(1);
            p.volumeSlider->setLargeStep(10);
            ftk::setScreenshotTag(p.volumeSlider, "Audio.Volume");

            p.muteCheckBox = ftk::CheckBox::create(context);
            ftk::setScreenshotTag(p.muteCheckBox, "Audio.Mute");

            p.channelComboBox = ftk::ComboBox::create(context);
            p.channelComboBox->setTooltip("Audio channel to play");
            ftk::setScreenshotTag(p.channelComboBox, "Audio.Channel");

            p.syncOffsetSlider = ftk::DoubleEditSlider::create(context);
            p.syncOffsetSlider->setRange(-1.0, 1.0);
            p.syncOffsetSlider->setDefault(0.0);
            p.syncOffsetSlider->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.syncOffsetSlider, "Audio.SyncOffset");

            auto formLayout = ftk::FormLayout::create(context);
            formLayout->setMarginRole(ftk::SizeRole::Margin);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Device:", p.deviceComboBox);
            formLayout->addRow("Volume:", p.volumeSlider);
            formLayout->addRow("Mute:", p.muteCheckBox);
            formLayout->addRow("Play channel:", p.channelComboBox);
            formLayout->addRow("Sync offset (seconds):", p.syncOffsetSlider);

            _setWidget(formLayout);

            auto appWeak = std::weak_ptr<App>(app);
            p.deviceComboBox->setIndexCallback(
                [this, appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        if (value >= 0 && value < static_cast<int>(_p->devices.size()))
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

            p.channelComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        // The first item is "All".
                        auto filesModel = app->getFilesModel();
                        filesModel->setAudioChannel(filesModel->getA(), value - 1);
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

            p.channelObserver = ftk::Observer<int>::create(
                app->getFilesModel()->observeAudioChannel(),
                [this](int value)
                {
                    _p->channel = value;
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
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<AudioTool>(new AudioTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }

        void AudioTool::_widgetUpdate()
        {
            FTK_P();
            std::vector<std::string> items;
            items.push_back("All");
            for (int i = 0; i < p.info.channelCount; ++i)
            {
                items.push_back(ftk::Format("{0}").arg(1 + i));
            }
            if (p.info.channelCount >= 2)
            {
                items[1] = "L";
                items[2] = "R";
            }
            p.channelComboBox->setItems(items);
            p.channelComboBox->setCurrentIndex(
                p.channel < p.info.channelCount ? p.channel + 1 : 0);
            p.channelComboBox->setEnabled(p.info.channelCount >= 2);
        }
    }
}
