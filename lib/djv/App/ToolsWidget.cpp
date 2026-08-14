// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ToolsWidget.h>

#include <djv/App/App.h>
#include <djv/App/IToolWidget.h>
#include <djv/Models/ToolsModel.h>

#include <algorithm>

#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>

namespace djv
{
    namespace app
    {
        struct ToolsWidget::Private
        {
            std::weak_ptr<App> app;
            std::weak_ptr<MainWindow> mainWindow;
            // Kept by name so that opening a second tool does not take the
            // first one apart and build it again.
            std::map<std::string, std::shared_ptr<IToolWidget> > toolWidgets;
            std::shared_ptr<ftk::VerticalLayout> layout;
            std::shared_ptr<ftk::ScrollWidget> scrollWidget;
            std::shared_ptr<ftk::ListObserver<std::string> > openObserver;
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

            p.app = app;
            p.mainWindow = mainWindow;

            p.layout = ftk::VerticalLayout::create(context);
            p.layout->setMarginRole(ftk::SizeRole::None);
            p.layout->setSpacingRole(ftk::SizeRole::Border);

            // One scroll area for the whole stack rather than one inside each
            // tool: a tool then takes the height its contents need instead of
            // an equal share of the panel, and there is a single scroll bar
            // rather than one nested in another. Tools whose contents have no
            // natural end -- the logs -- keep their own, which stops at the
            // scroll area size role rather than growing without limit.
            p.scrollWidget = ftk::ScrollWidget::create(
                context, ftk::ScrollType::Both);
            _setWidget(p.scrollWidget);
            p.scrollWidget->setBorder(false);
            p.scrollWidget->setWidget(p.layout);

            p.openObserver = ftk::ListObserver<std::string>::create(
                app->getToolsModel()->observeOpenTools(),
                [this](const std::vector<std::string>& value)
                {
                    _widgetUpdate(value);
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

        std::shared_ptr<IToolWidget> ToolsWidget::getToolWidget(
            const std::string& name) const
        {
            FTK_P();
            const auto i = p.toolWidgets.find(name);
            return i != p.toolWidgets.end() ? i->second : nullptr;
        }

        void ToolsWidget::_widgetUpdate(const std::vector<std::string>& open)
        {
            FTK_P();

            // Take apart only what is no longer open.
            for (auto i = p.toolWidgets.begin(); i != p.toolWidgets.end(); )
            {
                if (std::find(open.begin(), open.end(), i->first) == open.end())
                {
                    i->second->setParent(nullptr);
                    i = p.toolWidgets.erase(i);
                }
                else
                {
                    ++i;
                }
            }

            auto context = getContext();
            auto app = p.app.lock();
            auto mainWindow = p.mainWindow.lock();
            for (const auto& name : open)
            {
                auto i = p.toolWidgets.find(name);
                if (i == p.toolWidgets.end())
                {
                    i = p.toolWidgets.insert(std::make_pair(
                        name,
                        app->getToolWidgetFactory()->createTool(
                            name, context, app, mainWindow, nullptr))).first;
                }
                // Reparented every time so the layout order follows the list
                // rather than the order the widgets were made in.
                i->second->setParent(p.layout);
            }
        }
    }
}
