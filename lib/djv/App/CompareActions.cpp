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

            _addCheckCommand(
                "B",
                "Show the B file.",
                [appWeak](const nlohmann::json& args)
                {
                    // Absent when the command is run by name rather than by
                    // the menu -- the command line, the screenshots -- where
                    // asking for a comparison means turning it on.
                    const bool value = args.contains("value") ?
                        args.at("value").get<bool>() :
                        true;
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = value ?
                            tl::Compare::B :
                            tl::Compare::None;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCheckCommand(
                "Wipe",
                "Wipe between the A and B files.",
                [appWeak](const nlohmann::json& args)
                {
                    // Absent when the command is run by name rather than by
                    // the menu -- the command line, the screenshots -- where
                    // asking for a comparison means turning it on.
                    const bool value = args.contains("value") ?
                        args.at("value").get<bool>() :
                        true;
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = value ?
                            tl::Compare::Wipe :
                            tl::Compare::None;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCheckCommand(
                "Overlay",
                "Overlay the A and B files.",
                [appWeak](const nlohmann::json& args)
                {
                    // Absent when the command is run by name rather than by
                    // the menu -- the command line, the screenshots -- where
                    // asking for a comparison means turning it on.
                    const bool value = args.contains("value") ?
                        args.at("value").get<bool>() :
                        true;
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = value ?
                            tl::Compare::Overlay :
                            tl::Compare::None;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCheckCommand(
                "Difference",
                "Show the difference between the A and B files.",
                [appWeak](const nlohmann::json& args)
                {
                    // Absent when the command is run by name rather than by
                    // the menu -- the command line, the screenshots -- where
                    // asking for a comparison means turning it on.
                    const bool value = args.contains("value") ?
                        args.at("value").get<bool>() :
                        true;
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = value ?
                            tl::Compare::Difference :
                            tl::Compare::None;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCheckCommand(
                "Horizontal",
                "Show the A and B files in a horizontal layout.",
                [appWeak](const nlohmann::json& args)
                {
                    // Absent when the command is run by name rather than by
                    // the menu -- the command line, the screenshots -- where
                    // asking for a comparison means turning it on.
                    const bool value = args.contains("value") ?
                        args.at("value").get<bool>() :
                        true;
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = value ?
                            tl::Compare::Horizontal :
                            tl::Compare::None;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCheckCommand(
                "Vertical",
                "Show the A and B files in a vertical layout.",
                [appWeak](const nlohmann::json& args)
                {
                    // Absent when the command is run by name rather than by
                    // the menu -- the command line, the screenshots -- where
                    // asking for a comparison means turning it on.
                    const bool value = args.contains("value") ?
                        args.at("value").get<bool>() :
                        true;
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = value ?
                            tl::Compare::Vertical :
                            tl::Compare::None;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCheckCommand(
                "Tile",
                "Show the A and B files in a tiled layout.",
                [appWeak](const nlohmann::json& args)
                {
                    // Absent when the command is run by name rather than by
                    // the menu -- the command line, the screenshots -- where
                    // asking for a comparison means turning it on.
                    const bool value = args.contains("value") ?
                        args.at("value").get<bool>() :
                        true;
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = value ?
                            tl::Compare::Tile :
                            tl::Compare::None;
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
            _actions["B"] = ftk::Action::create(
                "B",
                "CompareB",
                _checkCommand("B"));
            _actions["Wipe"] = ftk::Action::create(
                "Wipe",
                "CompareWipe",
                _checkCommand("Wipe"));
            _actions["Overlay"] = ftk::Action::create(
                "Overlay",
                "CompareOverlay",
                _checkCommand("Overlay"));
            _actions["Difference"] = ftk::Action::create(
                "Difference",
                "CompareDifference",
                _checkCommand("Difference"));
            _actions["Horizontal"] = ftk::Action::create(
                "Horizontal",
                "CompareHorizontal",
                _checkCommand("Horizontal"));
            _actions["Vertical"] = ftk::Action::create(
                "Vertical",
                "CompareVertical",
                _checkCommand("Vertical"));
            _actions["Tile"] = ftk::Action::create(
                "Tile",
                "CompareTile",
                _checkCommand("Tile"));
            // The keys and the commands keep the enumeration's names, which
            // are what the shortcuts are stored under; only what is shown
            // changes.
            const auto compareTimeLabels = models::getCompareTimeLabels();
            _actions["Relative"] = ftk::Action::create(
                compareTimeLabels[0],
                _command("Relative"));
            _actions["Absolute"] = ftk::Action::create(
                compareTimeLabels[1],
                _command("Absolute"));

            // Register the shortcuts.
            _addShortcut("Next", "Next", ftk::KeyShortcut(ftk::Key::PageDown, static_cast<int>(ftk::KeyModifier::Shift)));
            _addShortcut("Prev", "Previous", ftk::KeyShortcut(ftk::Key::PageUp, static_cast<int>(ftk::KeyModifier::Shift)));
            _addShortcut("B", "B", ftk::KeyShortcut(ftk::Key::B, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("Wipe", "Wipe", ftk::KeyShortcut(ftk::Key::W, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("Overlay", "Overlay");
            _addShortcut("Difference", "Difference");
            _addShortcut("Horizontal", "Horizontal");
            _addShortcut("Vertical", "Vertical");
            _addShortcut("Tile", "Tile", ftk::KeyShortcut(ftk::Key::T, static_cast<int>(ftk::KeyModifier::Control)));
            // The first argument names the shortcut where it is stored; the
            // second is what the shortcuts editor shows.
            _addShortcut("Relative", compareTimeLabels[0]);
            _addShortcut("Absolute", compareTimeLabels[1]);

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.filesObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    FTK_P();
                    _actions["Next"]->setEnabled(value.size() > 1);
                    _actions["Prev"]->setEnabled(value.size() > 1);
                    _actions["B"]->setEnabled(!value.empty());
                    _actions["Wipe"]->setEnabled(!value.empty());
                    _actions["Overlay"]->setEnabled(!value.empty());
                    _actions["Difference"]->setEnabled(!value.empty());
                    _actions["Horizontal"]->setEnabled(!value.empty());
                    _actions["Vertical"]->setEnabled(!value.empty());
                    _actions["Tile"]->setEnabled(!value.empty());
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
                        // Not comparing is the state with none of them on, so
                        // it is not one of them and has no action to tick.
                        if (tl::Compare::None == enums[i])
                            continue;
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
