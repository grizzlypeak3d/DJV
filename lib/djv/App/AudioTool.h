// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/IToolWidget.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Audio tool.
        class AudioTool : public IToolWidget
        {
            FTK_NON_COPYABLE(AudioTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            AudioTool();

        public:
            virtual ~AudioTool();

            static std::shared_ptr<AudioTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };
    }
}
