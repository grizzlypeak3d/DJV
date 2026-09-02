// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ReviewMenu.h>

#include <djv/App/App.h>
#include <djv/App/ReviewActions.h>
#include <djv/Models/RecentFilesModel.h>

namespace djv
{
    namespace app
    {
        struct ReviewMenu::Private
        {
            std::weak_ptr<App> app;
            std::shared_ptr<ftk::Menu> recentMenu;

            std::shared_ptr<ftk::ListObserver<ftk::Path> > recentObserver;
        };

        void ReviewMenu::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ReviewActions>& reviewActions,
            const std::shared_ptr<IWidget>& parent)
        {
            Menu::_init(context, parent);
            FTK_P();

            p.app = app;

            auto actions = reviewActions->getActions();
            addAction(actions["Open"]);
            addAction(actions["Save"]);
            addAction(actions["SaveAs"]);
            addAction(actions["Close"]);
            p.recentMenu = addSubMenu("Recent");
            addDivider();
            addAction(actions["Draw"]);
            addAction(actions["Erase"]);
            addAction(actions["Undo"]);
            addAction(actions["Redo"]);
            addAction(actions["ClearDrawing"]);
            addDivider();
            addAction(actions["AddNote"]);
            addAction(actions["AddRange"]);
            addAction(actions["PrevFrame"]);
            addAction(actions["NextFrame"]);

            p.recentObserver = ftk::ListObserver<ftk::Path>::create(
                app->getRecentReviewsModel()->observeRecent(),
                [this](const std::vector<ftk::Path>& value)
                {
                    _recentUpdate(value);
                });
        }

        ReviewMenu::ReviewMenu() :
            _p(new Private)
        {}

        ReviewMenu::~ReviewMenu()
        {}

        std::shared_ptr<ReviewMenu> ReviewMenu::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ReviewActions>& reviewActions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ReviewMenu>(new ReviewMenu);
            out->_init(context, app, reviewActions, parent);
            return out;
        }

        void ReviewMenu::close()
        {
            Menu::close();
            FTK_P();
            p.recentMenu->close();
        }

        void ReviewMenu::_recentUpdate(const std::vector<ftk::Path>& value)
        {
            FTK_P();
            p.recentMenu->clear();
            for (auto i = value.rbegin(); i != value.rend(); ++i)
            {
                const auto path = *i;
                auto weak = std::weak_ptr<ReviewMenu>(std::dynamic_pointer_cast<ReviewMenu>(shared_from_this()));
                auto action = ftk::Action::create(
                    path.get(),
                    [weak, path]
                    {
                        if (auto widget = weak.lock())
                        {
                            if (auto app = widget->_p->app.lock())
                            {
                                app->openReview(std::filesystem::u8path(path.get()));
                            }
                            widget->close();
                        }
                    });
                p.recentMenu->addAction(action);
            }
        }
    }
}
