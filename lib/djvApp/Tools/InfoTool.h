// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djvApp/Tools/IToolWidget.h>

#include <tlRender/Timeline/Player.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Information tool.
        class InfoTool : public IToolWidget
        {
            FTK_NON_COPYABLE(InfoTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            InfoTool();

        public:
            virtual ~InfoTool();

            static std::shared_ptr<InfoTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };
    }
}
