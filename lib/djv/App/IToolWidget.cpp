// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/IToolWidget.h>

#include <djv/App/App.h>

#include <ftk/UI/Icon.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/ToolButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/Bellows.h>
#include <ftk/UI/Settings.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct IToolWidget::Private
        {
            std::shared_ptr<ftk::Settings> settings;
            std::string name;
            // Weak, because each bellows holds the callback that reads this
            // map: a shared pointer here would be a cycle.
            std::map<std::string, std::weak_ptr<ftk::Bellows> > bellows;
            
            std::shared_ptr<ftk::Icon> icon;
            std::shared_ptr<ftk::Label> label;
            std::shared_ptr<ftk::ToolButton> closeButton;
            std::shared_ptr<ftk::VerticalLayout> toolLayout;
            std::shared_ptr<ftk::VerticalLayout> layout;
        };

        void IToolWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::string& name,
            const std::string& icon,
            const std::string& objectName,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(context, objectName, parent);
            FTK_P();

            _app = app;
            _mainWindow = mainWindow;

            p.settings = app->getSettings();
            p.name = name;

            p.icon = ftk::Icon::create(context, icon);
            p.icon->setMarginRole(ftk::SizeRole::MarginSmall);

            p.label = ftk::Label::create(context, name);
            p.label->setMarginRole(ftk::SizeRole::MarginSmall);
            p.label->setHStretch(ftk::Stretch::Expanding);

            p.closeButton = ftk::ToolButton::create(context);
            p.closeButton->setIcon("Close");

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::Border);
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(ftk::SizeRole::None);
            // Coloured so that each tool reads as its own thing in a stack of
            // them, rather than the panel being one long column of controls.
            hLayout->setBackgroundRole(ftk::ColorRole::Header);
            p.icon->setParent(hLayout);
            p.label->setParent(hLayout);
            p.closeButton->setParent(hLayout);
            p.toolLayout = ftk::VerticalLayout::create(context, p.layout);
            p.toolLayout->setSpacingRole(ftk::SizeRole::None);
            p.toolLayout->setHStretch(ftk::Stretch::Expanding);
            p.toolLayout->setVStretch(ftk::Stretch::Expanding);

            // Closes this tool rather than whatever is open: more than one
            // can be, and closing is how a tool is put away.
            auto appWeak = std::weak_ptr<App>(app);
            p.closeButton->setClickedCallback(
                [appWeak, name]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getToolsModel()->setToolOpen(name, false);
                    }
                });
        }

        IToolWidget::IToolWidget() :
            _p(new Private)
        {}

        IToolWidget::~IToolWidget()
        {}

        const std::string& IToolWidget::getToolName() const
        {
            return _p->name;
        }

        void IToolWidget::scrollTo(const std::string&)
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
            p.settings->get(ftk::Format("/{0}/Bellows").arg(p.name), json);
            for (auto i = json.begin(); i != json.end(); ++i)
            {
                auto j = value.find(i.key());
                if (j != value.end() && i.value().is_boolean())
                {
                    j->second->setOpen(i.value().get<bool>());
                }
            }

            // The state is written when it changes; the write in the tool's
            // destructor is a backstop.
            p.bellows.clear();
            for (const auto& i : value)
            {
                p.bellows[i.first] = i.second;
                i.second->setOpenCallback(
                    [this](bool)
                    {
                        _bellowsSave();
                    });
            }
        }

        void IToolWidget::_bellowsSave()
        {
            FTK_P();
            nlohmann::json json;
            for (const auto& i : p.bellows)
            {
                if (auto bellows = i.second.lock())
                {
                    json[i.first] = bellows->isOpen();
                }
            }
            p.settings->set(ftk::Format("/{0}/Bellows").arg(p.name), json);
        }

        void IToolWidget::_saveSettings(const std::map<std::string, std::shared_ptr<ftk::Bellows> >& value)
        {
            FTK_P();
            nlohmann::json json;
            for (const auto& i : value)
            {
                json[i.first] = i.second->isOpen();
            }
            p.settings->set(ftk::Format("/{0}/Bellows").arg(p.name), json);
        }

        void IToolWidget::_setWidget(const std::shared_ptr<ftk::IWidget>& value)
        {
            value->setHStretch(ftk::Stretch::Expanding);
            value->setVStretch(ftk::Stretch::Expanding);
            value->setParent(_p->toolLayout);
        }

        struct ToolWidgetFactory::Private
        {
            std::map<std::string, ToolWidgetFnc> fncs;
        };

        ToolWidgetFactory::ToolWidgetFactory() :
            _p(new Private)
        {}

        ToolWidgetFactory::~ToolWidgetFactory()
        {}

        std::shared_ptr<ToolWidgetFactory> ToolWidgetFactory::create()
        {
            return std::shared_ptr<ToolWidgetFactory>(new ToolWidgetFactory);
        }

        void ToolWidgetFactory::addTool(
            const std::string& name,
            const ToolWidgetFnc& fnc)
        {
            _p->fncs[name] = fnc;
        }

        std::shared_ptr<IToolWidget> ToolWidgetFactory::createTool(
            const std::string& name,
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            FTK_P();
            std::shared_ptr<IToolWidget> out;
            if (const auto i = p.fncs.find(name); i != p.fncs.end())
            {
                out = i->second(context, app, mainWindow, parent);
            }
            return out;
        }
    }
}
