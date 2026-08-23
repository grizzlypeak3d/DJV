// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/App/IToolWidget.h>
#include <djv/Models/Export.h>

namespace djv
{
    namespace app
    {
        //! Magnification level.
        enum class DJV_APP_API_TYPE MagnifyLevel
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
        FTK_ENUM(DJV_APP_API, MagnifyLevel);

        //! Get a magnification level.
        DJV_APP_API int getMagnifyLevel(MagnifyLevel);

        //! Magnify tool.
        class DJV_APP_API_TYPE MagnifyTool : public IToolWidget
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
            DJV_APP_API virtual ~MagnifyTool();

            DJV_APP_API static std::shared_ptr<MagnifyTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_APP_API void setGeometry(const ftk::Box2I&) override;

        private:
            void _widgetUpdate();
            void _videoUpdate();

            FTK_PRIVATE();
        };
    }
}
