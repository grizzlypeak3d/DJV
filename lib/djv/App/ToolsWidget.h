// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IWidget.h>

namespace djv
{
    namespace app
    {
        class App;
        class IToolWidget;
        class MainWindow;

        //! Tools widget.
        class ToolsWidget : public ftk::IWidget
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
            virtual ~ToolsWidget();

            //! Create a new widget.
            static std::shared_ptr<ToolsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Get the active tool widget, or null if no tool is active.
            //! Get an open tool by name, or null when it is not open.
            std::shared_ptr<IToolWidget> getToolWidget(const std::string&) const;

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _widgetUpdate(const std::vector<std::string>&);

            FTK_PRIVATE();
        };
    }
}
