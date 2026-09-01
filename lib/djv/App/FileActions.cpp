// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/FileActions.h>

#include <djv/App/App.h>
#include <djv/Models/FilesModel.h>

#include <tlRender/Timeline/Player.h>

#include <algorithm>

namespace djv
{
    namespace app
    {
        struct FileActions::Private
        {
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<models::FilesModelItem> > > aObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
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
                "OpenPlaylist",
                "Open a playlist into the file list. Takes an optional "
                "\"fileName\"; without one a file browser is shown.",
                [appWeak](const nlohmann::json& args)
                {
                    if (auto app = appWeak.lock())
                    {
                        if (args.contains("fileName"))
                        {
                            app->openPlaylist(ftk::Path(
                                args.at("fileName").get<std::string>()));
                        }
                        else
                        {
                            app->openPlaylistDialog();
                        }
                    }
                });

            _addCommand(
                "SavePlaylist",
                "Save the file list as a playlist. Takes an optional "
                "\"fileName\"; without one a file browser is shown.",
                [appWeak](const nlohmann::json& args)
                {
                    if (auto app = appWeak.lock())
                    {
                        if (args.contains("fileName"))
                        {
                            app->savePlaylist(ftk::Path(
                                args.at("fileName").get<std::string>()));
                        }
                        else
                        {
                            app->savePlaylistDialog();
                        }
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
                "NextMediaReference",
                "Change to the next media reference.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        if (auto player = app->observePlayer()->get())
                        {
                            // Cycle through the keys used by the timeline,
                            // starting from the one in use. An unset key, which
                            // leaves the clips as they were authored, is not
                            // part of the cycle; it can be chosen from the menu.
                            const auto keys = player->getMediaReferenceKeys();
                            if (!keys.empty())
                            {
                                const auto i = std::find(
                                    keys.begin(),
                                    keys.end(),
                                    player->getMediaReferenceKey());
                                const size_t next = i != keys.end() ?
                                    ((i - keys.begin()) + 1) % keys.size() :
                                    0;
                                player->setMediaReferenceKey(keys[next]);
                            }
                        }
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
                        // Quitting with an unsaved review prompts first, the
                        // same way closing one does.
                        app->confirmClose(
                            [appWeak]
                            {
                                if (auto app = appWeak.lock())
                                {
                                    app->exit();
                                }
                            });
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
            _actions["OpenPlaylist"] = ftk::Action::create(
                "Open Playlist",
                _command("OpenPlaylist"));
            _actions["OpenPlaylist"]->setTooltip(
                "Open a playlist into the file list. Opening a \".otio\" "
                "file normally plays it as a timeline.");
            _actions["SavePlaylist"] = ftk::Action::create(
                "Save Playlist",
                _command("SavePlaylist"));
            _actions["SavePlaylist"]->setTooltip(
                "Save the file list as a \".otio\" playlist.");
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
            _actions["NextMediaReference"] = ftk::Action::create(
                "Next Media Reference",
                "Next",
                _command("NextMediaReference"));
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

            // Register the shortcuts.
            _addShortcut("Open", ftk::KeyShortcut(ftk::Key::O, static_cast<int>(ftk::commandKeyModifier)));
            _addShortcut("OpenAudio",
                ftk::KeyShortcut(
                    ftk::Key::O,
                    static_cast<int>(ftk::KeyModifier::Shift) |
                    static_cast<int>(ftk::commandKeyModifier)));
            _addShortcut("OpenPlaylist", "Open playlist");
            _addShortcut("SavePlaylist", "Save playlist");
            _addShortcut("Close", ftk::KeyShortcut(ftk::Key::E, static_cast<int>(ftk::commandKeyModifier)));
            _addShortcut("CloseAll",
                ftk::KeyShortcut(
                    ftk::Key::E,
                    static_cast<int>(ftk::KeyModifier::Shift) | static_cast<int>(ftk::commandKeyModifier)));
            _addShortcut("Reload",
                ftk::KeyShortcut(
                    ftk::Key::R,
                    static_cast<int>(ftk::KeyModifier::Shift) | static_cast<int>(ftk::commandKeyModifier)));
            _addShortcut("Next", ftk::KeyShortcut(ftk::Key::PageDown, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("Prev", ftk::KeyShortcut(ftk::Key::PageUp, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("NextMediaReference",
                ftk::KeyShortcut(ftk::Key::M, static_cast<int>(ftk::KeyModifier::Shift)));
            _addShortcut("NextLayer", ftk::KeyShortcut(ftk::Key::Equals, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("PrevLayer", ftk::KeyShortcut(ftk::Key::Minus, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("Exit", ftk::KeyShortcut(ftk::Key::Q, static_cast<int>(ftk::commandKeyModifier)));

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.filesObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    FTK_P();
                    _actions["SavePlaylist"]->setEnabled(!value.empty());
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

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    // There is nothing to cycle through unless the timeline
                    // uses more than one media reference key.
                    _actions["NextMediaReference"]->setEnabled(
                        value ? value->getMediaReferenceKeys().size() > 1 : false);
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
