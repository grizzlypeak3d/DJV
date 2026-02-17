// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/IToolWidget.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Color tool.
        class ColorTool : public IToolWidget
        {
            FTK_NON_COPYABLE(ColorTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            ColorTool();

        public:
            virtual ~ColorTool();

            static std::shared_ptr<ColorTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
