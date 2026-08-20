// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <ftk/UI/IWidgetPopup.h>

namespace djv
{
    namespace models
    {
        class AudioModel;
    }

    namespace ui
    {
        //! Audio popup.
        class DJV_API_TYPE AudioPopup : public ftk::IWidgetPopup
        {
            FTK_NON_COPYABLE(AudioPopup);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::AudioModel>&,
                const std::shared_ptr<IWidget>& parent);

            AudioPopup();

        public:
            DJV_API virtual ~AudioPopup();

            DJV_API static std::shared_ptr<AudioPopup> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::AudioModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
