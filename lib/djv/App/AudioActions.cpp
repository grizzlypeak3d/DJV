// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/AudioActions.h>

#include <djv/App/App.h>
#include <djv/Models/AudioModel.h>

namespace djv
{
    namespace app
    {
        struct AudioActions::Private
        {
            std::shared_ptr<ftk::Observer<float> > volumeObserver;
            std::shared_ptr<ftk::Observer<bool> > muteObserver;
        };

        void AudioActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            IActions::_init(context, app, "Audio");
            FTK_P();

            auto appWeak = std::weak_ptr<App>(app);

            // Register the commands.
            _addCommand(
                "VolumeUp",
                "Increase the audio volume.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getAudioModel()->volumeUp();
                    }
                });

            _addCommand(
                "VolumeDown",
                "Decrease the audio volume.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getAudioModel()->volumeDown();
                    }
                });

            _addCheckCommand(
                "Mute",
                "Toggle the audio mute.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        app->getAudioModel()->setMute(value);
                    }
                });

            // Create the actions.
            _actions["VolumeUp"] = ftk::Action::create(
                "Volume Up",
                _command("VolumeUp"));
            _actions["VolumeDown"] = ftk::Action::create(
                "Volume Down",
                _command("VolumeDown"));
            _actions["Mute"] = ftk::Action::create(
                "Mute",
                "Mute",
                _checkCommand("Mute"));

            // Register the shortcuts.
            _addShortcut("VolumeUp", "Volume up", ftk::Key::Period);
            _addShortcut("VolumeDown", "Volume down", ftk::Key::Comma);
            _addShortcut("Mute", "Mute", ftk::Key::M);

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.volumeObserver = ftk::Observer<float>::create(
                app->getAudioModel()->observeVolume(),
                [this](float value)
                {
                    _actions["VolumeUp"]->setEnabled(value < 1.F);
                    _actions["VolumeDown"]->setEnabled(value > 0.F);
                });

            p.muteObserver = ftk::Observer<bool>::create(
                app->getAudioModel()->observeMute(),
                [this](bool value)
                {
                    _actions["Mute"]->setChecked(value);
                });
        }

        AudioActions::AudioActions() :
            _p(new Private)
        {}

        AudioActions::~AudioActions()
        {}

        std::shared_ptr<AudioActions> AudioActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            auto out = std::shared_ptr<AudioActions>(new AudioActions);
            out->_init(context, app);
            return out;
        }
    }
}
