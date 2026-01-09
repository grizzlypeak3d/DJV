// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Menus/CompareMenu.h>

#include <djvApp/Actions/CompareActions.h>
#include <djvApp/App.h>

namespace djv
{
    namespace app
    {
        struct CompareMenu::Private
        {
            std::weak_ptr<App> app;

            std::vector<std::shared_ptr<ftk::Action> > bActions;
            std::map<std::string, std::shared_ptr<ftk::Menu> > menus;

            std::shared_ptr<ftk::ListObserver<std::shared_ptr<FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::ListObserver<int> > bIndexesObserver;
        };

        void CompareMenu::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<CompareActions>& compareActions,
            const std::shared_ptr<IWidget>& parent)
        {
            Menu::_init(context, parent);
            FTK_P();

            p.app = app;

            p.menus["B"] = addSubMenu("B");
            auto actions = compareActions->getActions();
            addAction(actions["Next"]);
            addAction(actions["Prev"]);
            addDivider();
            addAction(actions["A"]);
            addAction(actions["B"]);
            addAction(actions["ABToggle"]);
            addAction(actions["Wipe"]);
            addAction(actions["Overlay"]);
            addAction(actions["Difference"]);
            addAction(actions["Horizontal"]);
            addAction(actions["Vertical"]);
            addAction(actions["Tile"]);
            addDivider();
            p.menus["Time"] = addSubMenu("Time");
            const auto timeLabels = tl::getCompareTimeLabels();
            for (const auto& label : timeLabels)
            {
                p.menus["Time"]->addAction(actions[label]);
            }

            p.filesObserver = ftk::ListObserver<std::shared_ptr<FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<FilesModelItem> >& value)
                {
                    _filesUpdate(value);
                });

            p.bIndexesObserver = ftk::ListObserver<int>::create(
                app->getFilesModel()->observeBIndexes(),
                [this](const std::vector<int>& value)
                {
                    _bUpdate(value);
                });
        }

        CompareMenu::CompareMenu() :
            _p(new Private)
        {}

        CompareMenu::~CompareMenu()
        {}

        std::shared_ptr<CompareMenu> CompareMenu::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<CompareActions>& compareActions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<CompareMenu>(new CompareMenu);
            out->_init(context, app, compareActions, parent);
            return out;
        }

        void CompareMenu::close()
        {
            Menu::close();
            FTK_P();
            for (const auto& menu : p.menus)
            {
                menu.second->close();
            }
        }

        void CompareMenu::_filesUpdate(
            const std::vector<std::shared_ptr<FilesModelItem> >& value)
        {
            FTK_P();
            p.menus["B"]->clear();
            p.bActions.clear();
            if (auto app = p.app.lock())
            {
                const auto bIndexes = app->getFilesModel()->getBIndexes();
                for (size_t i = 0; i < value.size(); ++i)
                {
                    auto action = ftk::Action::create(
                        value[i]->path.getFileName(),
                        [this, i]
                        {
                            close();
                            if (auto app = _p->app.lock())
                            {
                                app->getFilesModel()->toggleB(i);
                            }
                        });
                    const auto j = std::find(bIndexes.begin(), bIndexes.end(), i);
                    action->setChecked(j != bIndexes.end());
                    p.menus["B"]->addAction(action);
                    p.bActions.push_back(action);
                }
            }
        }

        void CompareMenu::_bUpdate(const std::vector<int>& value)
        {
            FTK_P();
            for (int i = 0; i < p.bActions.size(); ++i)
            {
                const auto j = std::find(value.begin(), value.end(), i);
                p.menus["B"]->setChecked(
                    p.bActions[i],
                    j != value.end());
            }
        }
    }
}
