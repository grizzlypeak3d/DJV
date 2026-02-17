// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ToolsWidget.h>

#include <djv/App/App.h>
#include <djv/App/AudioTool.h>
#include <djv/App/ColorPickerTool.h>
#include <djv/App/ColorTool.h>
#include <djv/App/DiagTool.h>
#include <djv/App/DevicesTool.h>
#include <djv/App/ExportTool.h>
#include <djv/App/FilesTool.h>
#include <djv/App/InfoTool.h>
#include <djv/App/MagnifyTool.h>
#include <djv/App/MessagesTool.h>
#include <djv/App/SettingsTool.h>
#include <djv/App/SysLogTool.h>
#include <djv/App/ViewTool.h>

#include <ftk/UI/RowLayout.h>
#include <ftk/UI/StackLayout.h>

namespace djv
{
    namespace app
    {
        struct ToolsWidget::Private
        {
            std::shared_ptr<IToolWidget> toolWidget;
            std::shared_ptr<ftk::Observer<models::Tool> > activeObserver;
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
            p.activeObserver = ftk::Observer<models::Tool>::create(
                app->getToolsModel()->observeActiveTool(),
                [this, appWeak, mainWindowWeak](models::Tool value)
                {
                    FTK_P();
                    if (p.toolWidget)
                    {
                        p.toolWidget->setParent(nullptr);
                        p.toolWidget.reset();
                    }
                    auto app = appWeak.lock();
                    auto mainWindow = mainWindowWeak.lock();
                    if (app && mainWindow)
                    {
                        auto context = app->getContext();
                        switch (value)
                        {
                        case models::Tool::Files:
                            p.toolWidget = FilesTool::create(context, app, shared_from_this());
                            break;
                        case models::Tool::Export:
                            p.toolWidget = ExportTool::create(context, app, shared_from_this());
                            break;
                        case models::Tool::View:
                            p.toolWidget = ViewTool::create(context, app, mainWindow, shared_from_this());
                            break;
                        case models::Tool::Color:
                            p.toolWidget = ColorTool::create(context, app, shared_from_this());
                            break;
                        case models::Tool::ColorPicker:
                            p.toolWidget = ColorPickerTool::create(context, app, mainWindow, shared_from_this());
                            break;
                        case models::Tool::Magnify:
                            p.toolWidget = MagnifyTool::create(context, app, mainWindow, shared_from_this());
                            break;
                        case models::Tool::Info:
                            p.toolWidget = InfoTool::create(context, app, shared_from_this());
                            break;
                        case models::Tool::Audio:
                            p.toolWidget = AudioTool::create(context, app, shared_from_this());
                            break;
                        case models::Tool::Devices:
                            p.toolWidget = DevicesTool::create(context, app, shared_from_this());
                            break;
                        case models::Tool::Settings:
                            p.toolWidget = SettingsTool::create(context, app, shared_from_this());
                            break;
                        case models::Tool::Messages:
                            p.toolWidget = MessagesTool::create(context, app, shared_from_this());
                            break;
                        case models::Tool::SysLog:
                            p.toolWidget = SysLogTool::create(context, app, shared_from_this());
                            break;
                        case models::Tool::Diag:
                            p.toolWidget = DiagTool::create(context, app, shared_from_this());
                            break;
                        default: break;
                        }
                    }
                    setVisible(value != models::Tool::None);
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
