// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Widgets/TabBar.h>

#include <djvApp/Models/FilesModel.h>
#include <djvApp/App.h>

#include <ftk/UI/TabBar.h>
#include <ftk/Core/String.h>

namespace djv
{
    namespace app
    {
        struct TabBar::Private
        {
            int aIndex = -1;
            std::shared_ptr<ftk::TabBar> tabBar;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<int> > aIndexObserver;
        };

        void TabBar::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(context, "tl::play::TabBar", parent);
            FTK_P();

            p.tabBar = ftk::TabBar::create(context, shared_from_this());
            p.tabBar->setTabsClosable(true);

            std::weak_ptr<App> appWeak(app);
            p.tabBar->setCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->setA(value);
                    }
                });
            p.tabBar->setTabCloseCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->close(value);
                    }
                });

            p.filesObserver = ftk::ListObserver<std::shared_ptr<FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<FilesModelItem> >& value)
                {
                    FTK_P();
                    p.tabBar->clearTabs();
                    for (const auto& item : value)
                    {
                        p.tabBar->addTab(
                            ftk::elide(item->path.getFileName()),
                            item->path.get());
                    }
                    p.tabBar->setCurrentTab(p.aIndex);
                });

            p.aIndexObserver = ftk::Observer<int>::create(
                app->getFilesModel()->observeAIndex(),
                [this](int value)
                {
                    FTK_P();
                    p.aIndex = value;
                    p.tabBar->setCurrentTab(value);
                });
        }

        TabBar::TabBar() :
            _p(new Private)
        {}

        TabBar::~TabBar()
        {}

        std::shared_ptr<TabBar> TabBar::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<TabBar>(new TabBar);
            out->_init(context, app, parent);
            return out;
        }

        void TabBar::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->tabBar->setGeometry(value);
        }

        void TabBar::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            setSizeHint(_p->tabBar->getSizeHint());
        }
    }
}
