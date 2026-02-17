// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/IToolWidget.h>

namespace djv
{
    namespace models
    {
        struct ExportSettings;
    }

    namespace app
    {
        class App;

        //! Export tool.
        class ExportTool : public IToolWidget
        {
            FTK_NON_COPYABLE(ExportTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            ExportTool();

        public:
            virtual ~ExportTool();

            static std::shared_ptr<ExportTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            void _widgetUpdate(const models::ExportSettings&);
            void _export();
            bool _exportFrame();

            FTK_PRIVATE();
        };
    }
}
