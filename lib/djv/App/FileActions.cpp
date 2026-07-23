// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/FileActions.h>

#include <djv/App/App.h>
#include <djv/Models/FilesModel.h>

namespace djv
{
    namespace app
    {
        struct FileActions::Private
        {
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<models::FilesModelItem> > > aObserver;
        };

        void FileActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            IActions::_init(context, app, "File");
            FTK_P();

            // Register the commands.
            auto appWeak = std::weak_ptr<App>(app);
            _addCommand(
                "Open",
                "Open a file.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->openDialog();
                    }
                });

            _addCommand(
                "OpenAudio",
                "Open a file with a separate audio file.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->openSeparateAudioDialog();
                    }
                });

            _addCommand(
                "Close",
                "Close the current file.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->close();
                    }
                });

            _addCommand(
                "CloseAll",
                "Close all files.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->closeAll();
                    }
                });

            _addCommand(
                "Reload",
                "Reload the current file.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->reload();
                    }
                });

            _addCommand(
                "Next",
                "Change to the next file.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->next();
                    }
                });

            _addCommand(
                "Prev",
                "Change to the previous file.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->prev();
                    }
                });

            _addCommand(
                "NextLayer",
                "Change to the next layer.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->nextLayer();
                    }
                });

            _addCommand(
                "PrevLayer",
                "Change to the previous layer.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->prevLayer();
                    }
                });

            _addCommand(
                "Exit",
                "Exit the application.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->exit();
                    }
                });

            // Create the actions.
            _actions["Open"] = ftk::Action::create(
                "Open",
                "FileOpen",
                _command("Open"));
            _actions["OpenAudio"] = ftk::Action::create(
                "Open With Audio",
                "FileOpenAudio",
                _command("OpenAudio"));
            _actions["Close"] = ftk::Action::create(
                "Close",
                "FileClose",
                _command("Close"));
            _actions["CloseAll"] = ftk::Action::create(
                "Close All",
                "FileCloseAll",
                _command("CloseAll"));
            _actions["Reload"] = ftk::Action::create(
                "Reload",
                "FileReload",
                _command("Reload"));
            _actions["Next"] = ftk::Action::create(
                "Next",
                "Next",
                _command("Next"));
            _actions["Prev"] = ftk::Action::create(
                "Previous",
                "Prev",
                _command("Prev"));
            _actions["NextLayer"] = ftk::Action::create(
                "Next Layer",
                "Next",
                _command("NextLayer"));
            _actions["PrevLayer"] = ftk::Action::create(
                "Previous Layer",
                "Prev",
                _command("PrevLayer"));
            _actions["Exit"] = ftk::Action::create(
                "Exit",
                _command("Exit"));

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.filesObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    FTK_P();
                    _actions["Close"]->setEnabled(!value.empty());
                    _actions["CloseAll"]->setEnabled(!value.empty());
                    _actions["Reload"]->setEnabled(!value.empty());
                    _actions["Next"]->setEnabled(value.size() > 1);
                    _actions["Prev"]->setEnabled(value.size() > 1);
                });

            p.aObserver = ftk::Observer<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeA(),
                [this](const std::shared_ptr<models::FilesModelItem>& value)
                {
                    _actions["NextLayer"]->setEnabled(value ? value->videoLayers.size() > 1 : false);
                    _actions["PrevLayer"]->setEnabled(value ? value->videoLayers.size() > 1 : false);
                });
        }

        FileActions::FileActions() :
            _p(new Private)
        {}

        FileActions::~FileActions()
        {}

        std::shared_ptr<FileActions> FileActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            auto out = std::shared_ptr<FileActions>(new FileActions);
            out->_init(context, app);
            return out;
        }
    }
}
