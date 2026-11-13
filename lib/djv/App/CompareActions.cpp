// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/CompareActions.h>

#include <djv/App/App.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/TimeUnitsModel.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct CompareActions::Private
        {
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > optionsObserver;
            std::shared_ptr<ftk::Observer<tl::CompareTime> > timeObserver;
            std::shared_ptr<ftk::Observer<std::optional<models::HeldFrame> > > heldFrameObserver;
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
                    // Absent when run by name; see the "B" command above.
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
                "Butterfly",
                "Show the same half of the A and B files, one of them "
                "mirrored, so that they meet on the same picture.",
                [appWeak](const nlohmann::json& args)
                {
                    // Absent when run by name; see the "B" command above.
                    const bool value = args.contains("value") ?
                        args.at("value").get<bool>() :
                        true;
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = value ?
                            tl::Compare::Butterfly :
                            tl::Compare::None;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            _addCheckCommand(
                "Overlay",
                "Overlay the A and B files.",
                [appWeak](const nlohmann::json& args)
                {
                    // Absent when run by name; see the "B" command above.
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
                    // Absent when run by name; see the "B" command above.
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
                    // Absent when run by name; see the "B" command above.
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
                    // Absent when run by name; see the "B" command above.
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
                    // Absent when run by name; see the "B" command above.
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

            // The frame store: a frame held to compare against, whatever the
            // time or the file, which a comparison of files cannot do -- a
            // still against a movie's frames, or a frame against another of
            // the same clip (#627).
            _addCommand(
                "HoldFrame",
                "Hold the current frame and compare against it: it becomes "
                "the comparison's B, wiped against A if no other comparison "
                "is chosen, whatever the time or the file, until it is let "
                "go. Holding again holds the frame then current.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto player = app->observePlayer()->get();
                        const auto& a = app->getFilesModel()->getA();
                        if (!player || !a)
                            return;
                        const auto& frames = player->getCurrentVideo();
                        if (frames.empty() || frames.front().layers.empty())
                            return;
                        models::HeldFrame held;
                        held.frame = frames.front();
                        held.label = ftk::Format("{0} {1}").
                            arg(a->path.getFileName()).
                            arg(app->getTimeUnitsModel()->getLabel(held.frame.time));
                        auto filesModel = app->getFilesModel();
                        filesModel->setHeldFrame(held);
                        auto options = filesModel->getCompareOptions();
                        if (tl::Compare::None == options.compare)
                        {
                            options.compare = tl::Compare::Wipe;
                            filesModel->setCompareOptions(options);
                        }
                    }
                });

            _addCommand(
                "ClearHeld",
                "Let go of the held frame.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->setHeldFrame(std::nullopt);
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
            _actions["Butterfly"] = ftk::Action::create(
                "Butterfly",
                "CompareButterfly",
                _checkCommand("Butterfly"));
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

            _actions["HoldFrame"] = ftk::Action::create(
                "Hold Frame",
                _command("HoldFrame"));
            _actions["ClearHeld"] = ftk::Action::create(
                "Clear Held Frame",
                _command("ClearHeld"));

            // Register the shortcuts.
            _addShortcut("Next");
            _addShortcut("Prev");
            _addShortcut("B");
            _addShortcut("Wipe");
            _addShortcut("Butterfly");
            _addShortcut("Overlay");
            _addShortcut("Difference");
            _addShortcut("Horizontal");
            _addShortcut("Vertical");
            _addShortcut("Tile");
            // The first argument names the shortcut where it is stored; the
            // second is what the shortcuts editor shows.
            _addShortcut("Relative", compareTimeLabels[0]);
            _addShortcut("Absolute", compareTimeLabels[1]);
            // No keys yet; they can be set in the shortcuts.
            _addShortcut("HoldFrame");
            _addShortcut("ClearHeld");

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
                    _actions["Butterfly"]->setEnabled(!value.empty());
                    _actions["Overlay"]->setEnabled(!value.empty());
                    _actions["Difference"]->setEnabled(!value.empty());
                    _actions["Horizontal"]->setEnabled(!value.empty());
                    _actions["Vertical"]->setEnabled(!value.empty());
                    _actions["Tile"]->setEnabled(!value.empty());
                    _actions["HoldFrame"]->setEnabled(!value.empty());
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

            p.heldFrameObserver = ftk::Observer<std::optional<models::HeldFrame> >::create(
                app->getFilesModel()->observeHeldFrame(),
                [this](const std::optional<models::HeldFrame>& value)
                {
                    _actions["ClearHeld"]->setEnabled(value.has_value());
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
