// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djvApp/Tools/IToolWidget.h>

namespace djv
{
    namespace app
    {
        class App;
        class MainWindow;

        //! Color picker tool.
        class ColorPickerTool : public IToolWidget
        {
            FTK_NON_COPYABLE(ColorPickerTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent);

            ColorPickerTool();

        public:
            virtual ~ColorPickerTool();

            static std::shared_ptr<ColorPickerTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
