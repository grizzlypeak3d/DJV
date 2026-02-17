// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/AudioMenu.h>

#include <djv/App/AudioActions.h>

namespace djv
{
    namespace app
    {
        void AudioMenu::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<AudioActions>& audioActions,
            const std::shared_ptr<IWidget>& parent)
        {
            Menu::_init(context, parent);

            auto actions = audioActions->getActions();
            addAction(actions["VolumeUp"]);
            addAction(actions["VolumeDown"]);
            addAction(actions["Mute"]);
        }

        AudioMenu::~AudioMenu()
        {}

        std::shared_ptr<AudioMenu> AudioMenu::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<AudioActions>& actions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<AudioMenu>(new AudioMenu);
            out->_init(context, actions, parent);
            return out;
        }
    }
}
