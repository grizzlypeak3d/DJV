// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Actions/CompareActions.h>

#include <djvApp/Models/FilesModel.h>
#include <djvApp/App.h>

namespace djv
{
    namespace app
    {
        struct CompareActions::Private
        {
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<tl::timeline::CompareOptions> > optionsObserver;
            std::shared_ptr<ftk::Observer<tl::timeline::CompareTime> > timeObserver;
        };

        void CompareActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            IActions::_init(context, app, "Compare");
            FTK_P();

            auto appWeak = std::weak_ptr<App>(app);
            _actions["Next"] = ftk::Action::create(
                "Next",
                "Next",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->nextB();
                    }
                });

            _actions["Prev"] = ftk::Action::create(
                "Previous",
                "Prev",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->prevB();
                    }
                });

            _actions["A"] = ftk::Action::create(
                "A",
                "CompareA",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::timeline::Compare::A;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _actions["B"] = ftk::Action::create(
                "B",
                "CompareB",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::timeline::Compare::B;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _actions["Wipe"] = ftk::Action::create(
                "Wipe",
                "CompareWipe",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::timeline::Compare::Wipe;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _actions["Overlay"] = ftk::Action::create(
                "Overlay",
                "CompareOverlay",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::timeline::Compare::Overlay;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _actions["Difference"] = ftk::Action::create(
                "Difference",
                "CompareDifference",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::timeline::Compare::Difference;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _actions["Horizontal"] = ftk::Action::create(
                "Horizontal",
                "CompareHorizontal",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::timeline::Compare::Horizontal;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _actions["Vertical"] = ftk::Action::create(
                "Vertical",
                "CompareVertical",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::timeline::Compare::Vertical;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _actions["Tile"] = ftk::Action::create(
                "Tile",
                "CompareTile",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::timeline::Compare::Tile;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _actions["ABToggle"] = ftk::Action::create(
                "A/B Toggle",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        switch (options.compare)
                        {
                        case tl::timeline::Compare::A:
                            options.compare = tl::timeline::Compare::B;
                            break;
                        case tl::timeline::Compare::B:
                        default:
                            options.compare = tl::timeline::Compare::A;
                            break;
                        }
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _actions["Relative"] = ftk::Action::create(
                "Relative",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->setCompareTime(tl::timeline::CompareTime::Relative);
                    }
                });

            _actions["Absolute"] = ftk::Action::create(
                "Absolute",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->setCompareTime(tl::timeline::CompareTime::Absolute);
                    }
                });

            _tooltips =
            {
                { "Next", "Go to the next B file." },
                { "Prev", "Go to the previous B file." },
                { "A", "Show the A file." },
                { "B", "Show the B file." },
                { "Wipe", "Wipe between the A and B files." },
                { "Overlay", "Overlay the A and B files." },
                { "Difference", "Show the difference between the A and B files." },
                { "Horizontal", "Show the A and B files side by side." },
                { "Vertical", "Show the A and B files over and under." },
                { "Tile", "Show the A and B files tiled." },
            };

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.filesObserver = ftk::ListObserver<std::shared_ptr<FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<FilesModelItem> >& value)
                {
                    FTK_P();
                    _actions["Next"]->setEnabled(value.size() > 1);
                    _actions["Prev"]->setEnabled(value.size() > 1);
                });

            p.optionsObserver = ftk::Observer<tl::timeline::CompareOptions>::create(
                app->getFilesModel()->observeCompareOptions(),
                [this](const tl::timeline::CompareOptions& value)
                {
                    FTK_P();
                    const auto enums = tl::timeline::getCompareEnums();
                    const auto labels = tl::timeline::getCompareLabels();
                    for (size_t i = 0; i < enums.size(); ++i)
                    {
                        _actions[labels[i]]->setChecked(enums[i] == value.compare);
                    }
                });

            p.timeObserver = ftk::Observer<tl::timeline::CompareTime>::create(
                app->getFilesModel()->observeCompareTime(),
                [this](tl::timeline::CompareTime value)
                {
                    const auto enums = tl::timeline::getCompareTimeEnums();
                    const auto labels = tl::timeline::getCompareTimeLabels();
                    for (size_t i = 0; i < enums.size(); ++i)
                    {
                        _actions[labels[i]]->setChecked(enums[i] == value);
                    }
                });
        }

        CompareActions::CompareActions() :
            _p(new Private)
        {}

        CompareActions::~CompareActions()
        {}

        std::shared_ptr<CompareActions> CompareActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            auto out = std::shared_ptr<CompareActions>(new CompareActions);
            out->_init(context, app);
            return out;
        }
    }
}
