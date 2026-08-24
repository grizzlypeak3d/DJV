// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/App/IToolWidget.h>

namespace djv
{
    namespace app
    {
        //! Export tool.
        class DJV_APP_API_TYPE ExportTool : public IToolWidget
        {
            FTK_NON_COPYABLE(ExportTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent);

            ExportTool();

        public:
            DJV_APP_API virtual ~ExportTool();

            DJV_APP_API static std::shared_ptr<ExportTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
