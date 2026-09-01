// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ReviewActions.h>

#include <djv/App/App.h>
#include <djv/Models/FilesModel.h>

namespace djv
{
    namespace app
    {
        struct ReviewActions::Private
        {
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesObserver;
        };

        void ReviewActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            IActions::_init(context, app, "Review");
            FTK_P();

            // Register the commands.
            auto appWeak = std::weak_ptr<App>(app);
            _addCommand(
                "Open",
                "Open a review, replacing the current session.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->openReviewDialog();
                    }
                });

            _addCommand(
                "Save",
                "Save the current session as a review.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->saveReview();
                    }
                });

            _addCommand(
                "SaveAs",
                "Save the current session as a new review.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->saveReviewAs();
                    }
                });

            _addCommand(
                "Close",
                "Close the review and reset to the startup state.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->closeReview();
                    }
                });

            // Create the actions.
            _actions["Open"] = ftk::Action::create(
                "Open",
                _command("Open"));
            _actions["Save"] = ftk::Action::create(
                "Save",
                _command("Save"));
            _actions["SaveAs"] = ftk::Action::create(
                "Save As...",
                _command("SaveAs"));
            _actions["Close"] = ftk::Action::create(
                "Close",
                _command("Close"));

            // Register the shortcuts.
            // Alt rather than Shift on the command modifier: Shift+Ctrl+O is
            // "Open with audio", and Shift+Ctrl+S is free but keeping the pair
            // symmetrical is worth more than reusing it.
            _addShortcut(
                "Open",
                "Open review",
                ftk::KeyShortcut(
                    ftk::Key::O,
                    static_cast<int>(ftk::KeyModifier::Alt) |
                    static_cast<int>(ftk::commandKeyModifier)));
            _addShortcut(
                "Save",
                "Save review",
                ftk::KeyShortcut(
                    ftk::Key::S,
                    static_cast<int>(ftk::KeyModifier::Alt) |
                    static_cast<int>(ftk::commandKeyModifier)));
            _addShortcut("SaveAs", "Save review as");
            _addShortcut("Close", "Close review");

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.filesObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    // There is nothing to save, and nothing to close, until a
                    // file is open. Opening a review stays available.
                    _actions["Save"]->setEnabled(!value.empty());
                    _actions["SaveAs"]->setEnabled(!value.empty());
                    _actions["Close"]->setEnabled(!value.empty());
                });
        }

        ReviewActions::ReviewActions() :
            _p(new Private)
        {}

        ReviewActions::~ReviewActions()
        {}

        std::shared_ptr<ReviewActions> ReviewActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            auto out = std::shared_ptr<ReviewActions>(new ReviewActions);
            out->_init(context, app);
            return out;
        }
    }
}
