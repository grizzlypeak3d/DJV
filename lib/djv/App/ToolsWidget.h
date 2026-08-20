// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <ftk/UI/IContainer.h>

namespace djv
{
    namespace app
    {
        class App;
        class IToolWidget;
        class MainWindow;

        //! Tools widget.
        class DJV_API_TYPE ToolsWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ToolsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent);

            ToolsWidget();

        public:
            DJV_API virtual ~ToolsWidget();

            //! Create a new widget.
            DJV_API static std::shared_ptr<ToolsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Get the active tool widget, or null if no tool is active.
            //! Get an open tool by name, or null when it is not open.
            DJV_API std::shared_ptr<IToolWidget> getToolWidget(const std::string&) const;

        private:
            void _widgetUpdate(const std::vector<std::string>&);

            FTK_PRIVATE();
        };
    }
}
