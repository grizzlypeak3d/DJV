// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/IToolWidget.h>

#include <djvApp/App.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/Icon.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/ToolButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/Settings.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct IToolWidget::Private
        {
            std::shared_ptr<ftk::Settings> settings;

            Tool tool;
            
            std::shared_ptr<ftk::Icon> icon;
            std::shared_ptr<ftk::Label> label;
            std::shared_ptr<ftk::ToolButton> closeButton;
            std::shared_ptr<ftk::VerticalLayout> toolLayout;
            std::shared_ptr<ftk::VerticalLayout> layout;
        };

        void IToolWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            Tool tool,
            const std::string& objectName,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(context, objectName, parent);
            FTK_P();

            _app = app;

            p.settings = app->getSettings();

            p.tool = tool;

            p.icon = ftk::Icon::create(context, getIcon(tool));
            p.icon->setMarginRole(ftk::SizeRole::MarginSmall);

            p.label = ftk::Label::create(context, to_string(tool));
            p.label->setMarginRole(ftk::SizeRole::MarginSmall);
            p.label->setHStretch(ftk::Stretch::Expanding);

            p.closeButton = ftk::ToolButton::create(context);
            p.closeButton->setIcon("Close");

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::None);
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(ftk::SizeRole::None);
            p.icon->setParent(hLayout);
            p.label->setParent(hLayout);
            p.closeButton->setParent(hLayout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            p.toolLayout = ftk::VerticalLayout::create(context, p.layout);
            p.toolLayout->setSpacingRole(ftk::SizeRole::None);
            p.toolLayout->setHStretch(ftk::Stretch::Expanding);
            p.toolLayout->setVStretch(ftk::Stretch::Expanding);

            auto appWeak = std::weak_ptr<App>(app);
            p.closeButton->setClickedCallback(
                [appWeak, tool]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getToolsModel()->setActiveTool(Tool::None);
                    }
                });
        }

        IToolWidget::IToolWidget() :
            _p(new Private)
        {}

        IToolWidget::~IToolWidget()
        {}

        ftk::Size2I IToolWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void IToolWidget::setGeometry(const ftk::Box2I & value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void IToolWidget::_loadSettings(const std::map<std::string, std::shared_ptr<ftk::Bellows> >& value)
        {
            FTK_P();
            nlohmann::json json;
            p.settings->get(ftk::Format("/{0}/Bellows").arg(getLabel(p.tool)), json);
            for (auto i = json.begin(); i != json.end(); ++i)
            {
                auto j = value.find(i.key());
                if (j != value.end() && i.value().is_boolean())
                {
                    j->second->setOpen(i.value().get<bool>());
                }
            }
        }

        void IToolWidget::_saveSettings(const std::map<std::string, std::shared_ptr<ftk::Bellows> >& value)
        {
            FTK_P();
            nlohmann::json json;
            for (const auto& i : value)
            {
                json[i.first] = i.second->isOpen();
            }
            p.settings->set(ftk::Format("/{0}/Bellows").arg(getLabel(p.tool)), json);
        }

        void IToolWidget::_setWidget(const std::shared_ptr<ftk::IWidget>& value)
        {
            value->setHStretch(ftk::Stretch::Expanding);
            value->setVStretch(ftk::Stretch::Expanding);
            value->setParent(_p->toolLayout);
        }
    }
}
