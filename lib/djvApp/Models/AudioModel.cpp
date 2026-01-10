// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Models/AudioModel.h>

#include <tlRender/Core/AudioSystem.h>

#include <ftk/UI/Settings.h>
#include <ftk/Core/Context.h>
#include <ftk/Core/Math.h>

namespace djv
{
    namespace app
    {
        struct AudioModel::Private
        {
            std::shared_ptr<ftk::Settings> settings;
            std::shared_ptr<ftk::ObservableList<tl::AudioDeviceID> > devices;
            std::shared_ptr<ftk::Observable<tl::AudioDeviceID> > device;
            std::shared_ptr<ftk::Observable<float> > volume;
            std::shared_ptr<ftk::Observable<bool> > mute;
            std::shared_ptr<ftk::ObservableList<bool> > channelMute;
            std::shared_ptr<ftk::Observable<double> > syncOffset;
            std::shared_ptr<ftk::ListObserver<tl::AudioDeviceInfo> > devicesObserver;
        };

        void AudioModel::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            FTK_P();

            p.settings = settings;

            p.devices = ftk::ObservableList<tl::AudioDeviceID>::create();
            p.device = ftk::Observable<tl::AudioDeviceID>::create();

            float volume = 1.F;
            p.settings->get("/Audio/Volume", volume);
            p.volume = ftk::Observable<float>::create(volume);

            bool mute = false;
            p.settings->get("/Audio/Mute", mute);
            p.mute = ftk::Observable<bool>::create(mute);

            p.channelMute = ftk::ObservableList<bool>::create();

            p.syncOffset = ftk::Observable<double>::create(0.0);

            auto audioSystem = context->getSystem<tl::AudioSystem>();
            p.devicesObserver = ftk::ListObserver<tl::AudioDeviceInfo>::create(
                audioSystem->observeDevices(),
                [this](const std::vector<tl::AudioDeviceInfo>& devices)
                {
                    std::vector<tl::AudioDeviceID> ids;
                    for (const auto& device : devices)
                    {
                        ids.push_back(device.id);
                    }
                    _p->devices->setIfChanged(ids);
                });
        }

        AudioModel::AudioModel() :
            _p(new Private)
        {}

        AudioModel::~AudioModel()
        {
            FTK_P();
            p.settings->set("/Audio/Volume", p.volume->get());
            p.settings->set("/Audio/Mute", p.mute->get());
        }

        std::shared_ptr<AudioModel> AudioModel::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            auto out = std::shared_ptr<AudioModel>(new AudioModel);
            out->_init(context, settings);
            return out;
        }

        const std::vector<tl::AudioDeviceID>& AudioModel::getDevices()
        {
            return _p->devices->get();
        }

        std::shared_ptr<ftk::IObservableList<tl::AudioDeviceID> > AudioModel::observeDevices() const
        {
            return _p->devices;
        }

        const tl::AudioDeviceID& AudioModel::getDevice() const
        {
            return _p->device->get();
        }

        std::shared_ptr<ftk::IObservable<tl::AudioDeviceID> > AudioModel::observeDevice() const
        {
            return _p->device;
        }

        void AudioModel::setDevice(const tl::AudioDeviceID& value)
        {
            _p->device->setIfChanged(value);
        }

        float AudioModel::getVolume() const
        {
            return _p->volume->get();
        }

        std::shared_ptr<ftk::IObservable<float> > AudioModel::observeVolume() const
        {
            return _p->volume;
        }

        void AudioModel::setVolume(float value)
        {
            const float tmp = ftk::clamp(value, 0.F, 1.F);
            _p->volume->setIfChanged(tmp);
        }

        void AudioModel::volumeUp()
        {
            setVolume(_p->volume->get() + .01F);
        }

        void AudioModel::volumeDown()
        {
            setVolume(_p->volume->get() - .01F);
        }

        bool AudioModel::isMuted() const
        {
            return _p->mute->get();
        }

        std::shared_ptr<ftk::IObservable<bool> > AudioModel::observeMute() const
        {
            return _p->mute;
        }

        void AudioModel::setMute(bool value)
        {
            _p->mute->setIfChanged(value);
        }

        const std::vector<bool>& AudioModel::getChannelMute() const
        {
            return _p->channelMute->get();
        }

        std::shared_ptr<ftk::IObservableList<bool> > AudioModel::observeChannelMute() const
        {
            return _p->channelMute;
        }

        void AudioModel::setChannelMute(const std::vector<bool>& value)
        {
            _p->channelMute->setIfChanged(value);
        }

        double AudioModel::getSyncOffset() const
        {
            return _p->syncOffset->get();
        }

        std::shared_ptr<ftk::IObservable<double> > AudioModel::observeSyncOffset() const
        {
            return _p->syncOffset;
        }

        void AudioModel::setSyncOffset(double value)
        {
            _p->syncOffset->setIfChanged(value);
        }
    }
}
