// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/ToolsModel.h>

#include <ftk/UI/Bellows.h>

namespace djv
{
    namespace app
    {
        class App;
        class MainWindow;

        //! Base class for tool widgets.
        class IToolWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(IToolWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::string& name,
                const std::string& icon,
                const std::string& objectName,
                const std::shared_ptr<IWidget>& parent);

            IToolWidget();

        public:
            virtual ~IToolWidget() = 0;

            const std::string& getToolName() const;

            //! Scroll to the named section. Tools that lay their content out in
            //! scrollable sections (e.g. bellows) override this; the default
            //! does nothing.
            virtual void scrollTo(const std::string& section);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        protected:
            void _loadSettings(const std::map<std::string, std::shared_ptr<ftk::Bellows> >&);
            void _saveSettings(const std::map<std::string, std::shared_ptr<ftk::Bellows> >&);

            void _setWidget(const std::shared_ptr<ftk::IWidget>&);

            std::weak_ptr<App> _app;
            std::weak_ptr<MainWindow> _mainWindow;

        private:
            FTK_PRIVATE();
        };
        
        //! Tool widget function.
        typedef std::function<std::shared_ptr<IToolWidget>(
            const std::shared_ptr<ftk::Context>&,
            const std::shared_ptr<App>&,
            const std::shared_ptr<MainWindow>&,
            const std::shared_ptr<ftk::IWidget>&)> ToolWidgetFnc;

        //! Tool widget factory.
        class ToolWidgetFactory : public std::enable_shared_from_this<ToolWidgetFactory>
        {
            FTK_NON_COPYABLE(ToolWidgetFactory);

        protected:
            ToolWidgetFactory();

        public:
            ~ToolWidgetFactory();

            static std::shared_ptr<ToolWidgetFactory> create();

            void addTool(const std::string& name, const ToolWidgetFnc&);

            std::shared_ptr<IToolWidget> createTool(
                const std::string&,
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
