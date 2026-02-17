// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IWidgetPopup.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Audio popup.
        class AudioPopup : public ftk::IWidgetPopup
        {
            FTK_NON_COPYABLE(AudioPopup);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            AudioPopup();

        public:
            virtual ~AudioPopup();

            static std::shared_ptr<AudioPopup> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
