// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/TabBar.h>

#include <djv/Models/FilesModel.h>
#include <djv/App/App.h>

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
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesObserver;
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
            p.tabBar->setClosable(true);

            std::weak_ptr<App> appWeak(app);
            p.tabBar->setCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->setA(value);
                    }
                });
            p.tabBar->setCloseCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->close(value);
                    }
                });

            p.filesObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    FTK_P();
                    p.tabBar->clear();
                    for (const auto& item : value)
                    {
                        p.tabBar->addTab(
                            item->path.getFileName(),
                            item->path.get());
                    }
                    p.tabBar->setCurrent(p.aIndex);
                });

            p.aIndexObserver = ftk::Observer<int>::create(
                app->getFilesModel()->observeAIndex(),
                [this](int value)
                {
                    FTK_P();
                    p.aIndex = value;
                    p.tabBar->setCurrent(value);
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

        ftk::Size2I TabBar::getSizeHint() const
        {
            return _p->tabBar->getSizeHint();
        }

        void TabBar::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->tabBar->setGeometry(value);
        }
    }
}
