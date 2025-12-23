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

        //! Magnify tool.
        class MagnifyTool : public IToolWidget
        {
            FTK_NON_COPYABLE(MagnifyTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent);

            MagnifyTool();

        public:
            virtual ~MagnifyTool();

            static std::shared_ptr<MagnifyTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };
    }
}
