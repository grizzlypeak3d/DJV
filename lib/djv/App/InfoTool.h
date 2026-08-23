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
        //! Information tool.
        class DJV_APP_API_TYPE InfoTool : public IToolWidget
        {
            FTK_NON_COPYABLE(InfoTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent);

            InfoTool();

        public:
            DJV_APP_API virtual ~InfoTool();

            DJV_APP_API static std::shared_ptr<InfoTool> create(
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
