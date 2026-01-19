// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/ToolsWidget.h>

#include <djvApp/Tools/AudioTool.h>
#include <djvApp/Tools/ColorPickerTool.h>
#include <djvApp/Tools/ColorTool.h>
#include <djvApp/Tools/DiagTool.h>
#include <djvApp/Tools/DevicesTool.h>
#include <djvApp/Tools/ExportTool.h>
#include <djvApp/Tools/FilesTool.h>
#include <djvApp/Tools/InfoTool.h>
#include <djvApp/Tools/MagnifyTool.h>
#include <djvApp/Tools/MessagesTool.h>
#include <djvApp/Tools/SettingsTool.h>
#include <djvApp/Tools/SysLogTool.h>
#include <djvApp/Tools/ViewTool.h>
#include <djvApp/App.h>

#include <ftk/UI/RowLayout.h>
#include <ftk/UI/StackLayout.h>

namespace djv
{
    namespace app
    {
        struct ToolsWidget::Private
        {
            std::shared_ptr<IToolWidget> toolWidget;
            std::shared_ptr<ftk::Observer<Tool> > activeObserver;
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
            p.activeObserver = ftk::Observer<Tool>::create(
                app->getToolsModel()->observeActiveTool(),
                [this, appWeak, mainWindowWeak](Tool value)
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
                        case Tool::Files:
                            p.toolWidget = FilesTool::create(context, app, shared_from_this());
                            break;
                        case Tool::Export:
                            p.toolWidget = ExportTool::create(context, app, shared_from_this());
                            break;
                        case Tool::View:
                            p.toolWidget = ViewTool::create(context, app, mainWindow, shared_from_this());
                            break;
                        case Tool::Color:
                            p.toolWidget = ColorTool::create(context, app, shared_from_this());
                            break;
                        case Tool::ColorPicker:
                            p.toolWidget = ColorPickerTool::create(context, app, mainWindow, shared_from_this());
                            break;
                        case Tool::Magnify:
                            p.toolWidget = MagnifyTool::create(context, app, mainWindow, shared_from_this());
                            break;
                        case Tool::Info:
                            p.toolWidget = InfoTool::create(context, app, shared_from_this());
                            break;
                        case Tool::Audio:
                            p.toolWidget = AudioTool::create(context, app, shared_from_this());
                            break;
                        case Tool::Devices:
                            p.toolWidget = DevicesTool::create(context, app, shared_from_this());
                            break;
                        case Tool::Settings:
                            p.toolWidget = SettingsTool::create(context, app, shared_from_this());
                            break;
                        case Tool::Messages:
                            p.toolWidget = MessagesTool::create(context, app, shared_from_this());
                            break;
                        case Tool::SysLog:
                            p.toolWidget = SysLogTool::create(context, app, shared_from_this());
                            break;
                        case Tool::Diag:
                            p.toolWidget = DiagTool::create(context, app, shared_from_this());
                            break;
                        default: break;
                        }
                    }
                    setVisible(value != Tool::None);
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
