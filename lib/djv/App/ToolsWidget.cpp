// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ToolsWidget.h>

#include <djv/App/App.h>
#include <djv/App/IToolWidget.h>
#include <djv/Models/ToolsModel.h>

#include <ftk/UI/RowLayout.h>
#include <ftk/UI/StackLayout.h>

namespace djv
{
    namespace app
    {
        struct ToolsWidget::Private
        {
            std::shared_ptr<IToolWidget> toolWidget;
            std::shared_ptr<ftk::Observer<std::string> > activeObserver;
        };

        void ToolsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(
                context,
                "djv::app::ToolsWidget",
                parent);
            FTK_P();

            std::weak_ptr<App> appWeak(app);
            std::weak_ptr<MainWindow> mainWindowWeak(mainWindow);
            p.activeObserver = ftk::Observer<std::string>::create(
                app->getToolsModel()->observeActiveTool(),
                [this, appWeak, mainWindowWeak](const std::string& value)
                {
                    FTK_P();
                    if (p.toolWidget)
                    {
                        p.toolWidget->setParent(nullptr);
                        p.toolWidget.reset();
                    }
                    auto app = appWeak.lock();
                    p.toolWidget = app->getToolWidgetFactory()->createTool(
                        value,
                        getContext(),
                        app,
                        mainWindowWeak.lock(),
                        shared_from_this());
                });
        }

        ToolsWidget::ToolsWidget() :
            _p(new Private)
        {}

        ToolsWidget::~ToolsWidget()
        {}

        std::shared_ptr<ToolsWidget> ToolsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ToolsWidget>(new ToolsWidget);
            out->_init(context, app, mainWindow, parent);
            return out;
        }

        const std::shared_ptr<IToolWidget>& ToolsWidget::getToolWidget() const
        {
            return _p->toolWidget;
        }

        ftk::Size2I ToolsWidget::getSizeHint() const
        {
            FTK_P();
            return p.toolWidget ? p.toolWidget->getSizeHint() : ftk::Size2I();
        }

        void ToolsWidget::setGeometry(const ftk::Box2I & value)
        {
            IWidget::setGeometry(value);
            FTK_P();
            if (p.toolWidget)
            {
                p.toolWidget->setGeometry(value);
            }
        }
    }
}
