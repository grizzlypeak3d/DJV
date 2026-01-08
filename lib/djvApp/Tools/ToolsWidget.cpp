// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/ToolsWidget.h>

#include <djvApp/Tools/AudioTool.h>
#include <djvApp/Tools/ColorPickerTool.h>
#include <djvApp/Tools/ColorTool.h>
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
            std::map<Tool, std::shared_ptr<IToolWidget> > toolWidgets;
            std::shared_ptr<ftk::StackLayout> layout;
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

            p.toolWidgets[Tool::Files] = FilesTool::create(context, app);
            p.toolWidgets[Tool::Export] = ExportTool::create(context, app);
            p.toolWidgets[Tool::View] = ViewTool::create(context, app, mainWindow);
            p.toolWidgets[Tool::Color] = ColorTool::create(context, app);
            p.toolWidgets[Tool::ColorPicker] = ColorPickerTool::create(context, app, mainWindow);
            p.toolWidgets[Tool::Magnify] = MagnifyTool::create(context, app, mainWindow);
            p.toolWidgets[Tool::Info] = InfoTool::create(context, app);
            p.toolWidgets[Tool::Audio] = AudioTool::create(context, app);
            p.toolWidgets[Tool::Devices] = DevicesTool::create(context, app);
            p.toolWidgets[Tool::Settings] = SettingsTool::create(context, app);
            p.toolWidgets[Tool::Messages] = MessagesTool::create(context, app);
            p.toolWidgets[Tool::SysLog] = SysLogTool::create(context, app);

            p.layout = ftk::StackLayout::create(context, shared_from_this());
            for (const auto& widget : p.toolWidgets)
            {
                widget.second->setParent(p.layout);
            }

            p.activeObserver = ftk::Observer<Tool>::create(
                app->getToolsModel()->observeActiveTool(),
                [this](Tool value)
                {
                    FTK_P();
                    auto i = p.toolWidgets.find(value);
                    p.layout->setCurrentWidget(i != p.toolWidgets.end() ? i->second : nullptr);
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
            return _p->layout->getSizeHint();
        }

        void ToolsWidget::setGeometry(const ftk::Box2I & value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }
    }
}
