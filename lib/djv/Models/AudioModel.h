// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <tlRender/Timeline/AudioSystem.h>
#include <tlRender/Timeline/CompareOptions.h>

#include <ftk/Core/ObservableList.h>
#include <ftk/Core/Observable.h>

namespace ftk
{
    class Settings;
}

namespace djv
{
    namespace models
    {
        //! Audio model.
        class DJV_API_TYPE AudioModel : public std::enable_shared_from_this<AudioModel>
        {
            FTK_NON_COPYABLE(AudioModel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            AudioModel();

        public:
            DJV_API ~AudioModel();

            //! Create a new model.
            DJV_API static std::shared_ptr<AudioModel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            //! Get the output devices.
            DJV_API const std::vector<tl::AudioDeviceID>& getDevices();

            //! Observe the output devices.
            DJV_API std::shared_ptr<ftk::IObservableList<tl::AudioDeviceID> > observeDevices() const;

            //! Get the output device.
            DJV_API const tl::AudioDeviceID& getDevice() const;

            //! Observe the output device.
            DJV_API std::shared_ptr<ftk::IObservable<tl::AudioDeviceID> > observeDevice() const;

            //! Set the output device.
            DJV_API void setDevice(const tl::AudioDeviceID&);

            //! Get the volume.
            DJV_API float getVolume() const;

            //! Observe the volume.
            DJV_API std::shared_ptr<ftk::IObservable<float> > observeVolume() const;

            //! Set the volume.
            DJV_API void setVolume(float);

            //! Increase the volume.
            DJV_API void volumeUp();

            //! Decrease the volume.
            DJV_API void volumeDown();

            //! Get the audio mute.
            DJV_API bool isMuted() const;

            //! Observe the audio mute.
            DJV_API std::shared_ptr<ftk::IObservable<bool> > observeMute() const;

            //! Set the audio mute.
            DJV_API void setMute(bool);

            //! Get the audio channels mute.
            DJV_API const std::vector<bool>& getChannelMute() const;

            //! Observe the audio channels mute.
            DJV_API std::shared_ptr<ftk::IObservableList<bool> > observeChannelMute() const;

            //! Set the audio channels mute.
            DJV_API void setChannelMute(const std::vector<bool>&);

            //! Get the audio sync offset.
            DJV_API double getSyncOffset() const;

            //! Set the audio sync offset.
            DJV_API std::shared_ptr<ftk::IObservable<double> > observeSyncOffset() const;

            //! Set the audio sync offset.
            DJV_API void setSyncOffset(double);

        private:
            FTK_PRIVATE();
        };
    }
}
