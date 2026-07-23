// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/CompareActions.h>

#include <djv/App/App.h>
#include <djv/Models/FilesModel.h>

namespace djv
{
    namespace app
    {
        struct CompareActions::Private
        {
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > optionsObserver;
            std::shared_ptr<ftk::Observer<tl::CompareTime> > timeObserver;
        };

        void CompareActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            IActions::_init(context, app, "Compare");
            FTK_P();

            auto appWeak = std::weak_ptr<App>(app);

            // Register the commands.
            _addCommand(
                "Next",
                "Go to the next B file.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->nextB();
                    }
                });

            _addCommand(
                "Prev",
                "Go to the previous B file.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->prevB();
                    }
                });

            _addCommand(
                "A",
                "Show the A file.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::Compare::A;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCommand(
                "B",
                "Show the B file.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::Compare::B;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCommand(
                "Wipe",
                "Wipe between the A and B files.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::Compare::Wipe;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCommand(
                "Overlay",
                "Overlay the A and B files.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::Compare::Overlay;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCommand(
                "Difference",
                "Show the difference between the A and B files.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::Compare::Difference;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCommand(
                "Horizontal",
                "Show the A and B files side by side.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::Compare::Horizontal;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCommand(
                "Vertical",
                "Show the A and B files over and under.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::Compare::Vertical;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCommand(
                "Tile",
                "Show the A and B files tiled.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = tl::Compare::Tile;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCommand(
                "ABToggle",
                "Toggle between the A and B files.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        switch (options.compare)
                        {
                        case tl::Compare::A: options.compare = tl::Compare::B; break;
                        case tl::Compare::B:
                        default: options.compare = tl::Compare::A; break;
                        }
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCommand(
                "Relative",
                "Compare files using relative time.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->setCompareTime(tl::CompareTime::Relative);
                    }
                });

            _addCommand(
                "Absolute",
                "Compare files using absolute time.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->setCompareTime(tl::CompareTime::Absolute);
                    }
                });

            // Create the actions.
            _actions["Next"] = ftk::Action::create(
                "Next",
                "Next",
                _command("Next"));
            _actions["Prev"] = ftk::Action::create(
                "Previous",
                "Prev",
                _command("Prev"));
            _actions["A"] = ftk::Action::create(
                "A",
                "CompareA",
                _command("A"));
            _actions["B"] = ftk::Action::create(
                "B",
                "CompareB",
                _command("B"));
            _actions["Wipe"] = ftk::Action::create(
                "Wipe",
                "CompareWipe",
                _command("Wipe"));
            _actions["Overlay"] = ftk::Action::create(
                "Overlay",
                "CompareOverlay",
                _command("Overlay"));
            _actions["Difference"] = ftk::Action::create(
                "Difference",
                "CompareDifference",
                _command("Difference"));
            _actions["Horizontal"] = ftk::Action::create(
                "Horizontal",
                "CompareHorizontal",
                _command("Horizontal"));
            _actions["Vertical"] = ftk::Action::create(
                "Vertical",
                "CompareVertical",
                _command("Vertical"));
            _actions["Tile"] = ftk::Action::create(
                "Tile",
                "CompareTile",
                _command("Tile"));
            _actions["ABToggle"] = ftk::Action::create(
                "A/B Toggle",
                _command("ABToggle"));
            _actions["Relative"] = ftk::Action::create(
                "Relative",
                _command("Relative"));
            _actions["Absolute"] = ftk::Action::create(
                "Absolute",
                _command("Absolute"));

            // Register the shortcuts.
            _addShortcut("Next", "Next", ftk::KeyShortcut(ftk::Key::PageDown, static_cast<int>(ftk::KeyModifier::Shift)));
            _addShortcut("Prev", "Previous", ftk::KeyShortcut(ftk::Key::PageUp, static_cast<int>(ftk::KeyModifier::Shift)));
            _addShortcut("A", "A", ftk::KeyShortcut(ftk::Key::A, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("B", "B", ftk::KeyShortcut(ftk::Key::B, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("ABToggle", "A/B Toggle", ftk::KeyShortcut(ftk::Key::A, static_cast<int>(ftk::KeyModifier::Alt)));
            _addShortcut("Wipe", "Wipe", ftk::KeyShortcut(ftk::Key::W, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("Overlay", "Overlay");
            _addShortcut("Difference", "Difference");
            _addShortcut("Horizontal", "Horizontal");
            _addShortcut("Vertical", "Vertical");
            _addShortcut("Tile", "Tile", ftk::KeyShortcut(ftk::Key::T, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("Relative", "Relative");
            _addShortcut("Absolute", "Absolute");

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.filesObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    FTK_P();
                    _actions["Next"]->setEnabled(value.size() > 1);
                    _actions["Prev"]->setEnabled(value.size() > 1);
                    _actions["A"]->setEnabled(!value.empty());
                    _actions["B"]->setEnabled(!value.empty());
                    _actions["Wipe"]->setEnabled(!value.empty());
                    _actions["Overlay"]->setEnabled(!value.empty());
                    _actions["Difference"]->setEnabled(!value.empty());
                    _actions["Horizontal"]->setEnabled(!value.empty());
                    _actions["Vertical"]->setEnabled(!value.empty());
                    _actions["Tile"]->setEnabled(!value.empty());
                    _actions["ABToggle"]->setEnabled(!value.empty());
                });

            p.optionsObserver = ftk::Observer<tl::CompareOptions>::create(
                app->getFilesModel()->observeCompareOptions(),
                [this](const tl::CompareOptions& value)
                {
                    FTK_P();
                    const auto enums = tl::getCompareEnums();
                    const auto labels = tl::getCompareLabels();
                    for (size_t i = 0; i < enums.size(); ++i)
                    {
                        _actions[labels[i]]->setChecked(enums[i] == value.compare);
                    }
                });

            p.timeObserver = ftk::Observer<tl::CompareTime>::create(
                app->getFilesModel()->observeCompareTime(),
                [this](tl::CompareTime value)
                {
                    const auto enums = tl::getCompareTimeEnums();
                    const auto labels = tl::getCompareTimeLabels();
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
