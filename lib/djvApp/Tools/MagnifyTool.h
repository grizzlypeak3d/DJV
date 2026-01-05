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

        //! Magnification level.
        enum class MagnifyLevel
        {
            _2X,
            _4X,
            _8X,
            _16X,
            _32X,
            _64X,
            _128X,

            Count,
            First = _2X
        };
        FTK_ENUM(MagnifyLevel);

        //! Get a magnification level.
        int getMagnifyLevel(MagnifyLevel);

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
            void _videoDataUpdate();

            FTK_PRIVATE();
        };
    }
}
