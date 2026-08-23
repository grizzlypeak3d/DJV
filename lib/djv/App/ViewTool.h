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
        //! View tool.
        class DJV_APP_API_TYPE ViewTool : public IToolWidget
        {
            FTK_NON_COPYABLE(ViewTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent);

            ViewTool();

        public:
            DJV_APP_API virtual ~ViewTool();

            DJV_APP_API static std::shared_ptr<ViewTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
