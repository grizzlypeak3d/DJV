// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/IWidget.h>

namespace djv
{
    namespace app
    {
        class App;
        class AudioActions;
        class FrameActions;
        class PlaybackActions;

        //! Bottom tool bar.
        class BottomToolBar : public ftk::IWidget
        {
            FTK_NON_COPYABLE(BottomToolBar);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<PlaybackActions>&,
                const std::shared_ptr<FrameActions>&,
                const std::shared_ptr<AudioActions>&,
                const std::shared_ptr<IWidget>& parent);

            BottomToolBar();

        public:
            ~BottomToolBar();

            static std::shared_ptr<BottomToolBar> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<PlaybackActions>&,
                const std::shared_ptr<FrameActions>&,
                const std::shared_ptr<AudioActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Focus the current frame widget.
            void focusCurrentFrame();

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _playerUpdate(const std::shared_ptr<tl::timeline::Player>&);
            void _showSpeedPopup();
            void _showAudioPopup();

            FTK_PRIVATE();
        };
    }
}
