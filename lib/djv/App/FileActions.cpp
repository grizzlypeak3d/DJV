// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/FileActions.h>

#include <djv/App/App.h>
#include <djv/Models/FilesModel.h>

#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>


#include <algorithm>

namespace djv
{
    namespace app
    {
        namespace
        {
            // The file a command acts on: the "A" file, or the one at the
            // index given as "file".
            std::shared_ptr<models::FilesModelItem> commandFile(
                const std::shared_ptr<App>& app,
                const nlohmann::json& args)
            {
                const auto& files = app->getFilesModel()->getFiles();
                if (args.is_object() && args.contains("file"))
                {
                    const int index = args.at("file").get<int>();
                    if (index < 0 || index >= static_cast<int>(files.size()))
                    {
                        throw std::invalid_argument(ftk::Format(
                            "No file {0}; there are {1}").arg(index).arg(files.size()));
                    }
                    return files[index];
                }
                auto a = app->getFilesModel()->getA();
                if (!a)
                {
                    throw std::invalid_argument("No file is open");
                }
                return a;
            }
        }

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
                        app->closeFile();
                    }
                });

            _addCommand(
                "CloseAll",
                "Close all files.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->closeAllFiles();
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
                        app->getFilesModel()->nextMediaReferenceKey();
                    }
                });

            // By name, for scripts: the cycle above depends on which keys a
            // timeline happens to use.
            _addCommand(
                "MediaReference",
                "Set a file's media reference by name, e.g., { \"key\": \"Proxy\" }; "
                "\"As Authored\" or an empty key is the references the clips were "
                "authored with. It is the \"A\" file's unless \"file\" gives the "
                "index of another in the file list.",
                [appWeak](const nlohmann::json& args)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto item = commandFile(app, args);
                        const std::string key = args.is_object() && args.contains("key") ?
                            args.at("key").get<std::string>() :
                            std::string();
                        // A file that has not been opened yet takes the key
                        // as it is, and checks it when it opens.
                        if (!item->mediaReferenceKeysKnown)
                        {
                            app->getFilesModel()->setMediaReferenceKey(item, key);
                            return;
                        }
                        if (item->mediaReferenceKeys.empty() && !key.empty())
                        {
                            throw std::invalid_argument(ftk::Format(
                                "{0} has only the one media reference").
                                arg(item->path.getFileName()));
                        }
                        const auto found = models::findMediaReferenceKey(*item, key);
                        if (!found.has_value())
                        {
                            std::vector<std::string> names;
                            for (const auto& i : item->mediaReferenceKeys)
                            {
                                names.push_back(models::getMediaReferenceLabel(i));
                            }
                            throw std::invalid_argument(ftk::Format(
                                "{0} has no media reference \"{1}\"; it has {2}").
                                arg(item->path.getFileName()).
                                arg(key).
                                arg(ftk::join(names, ", ")));
                        }
                        app->getFilesModel()->setMediaReferenceKey(item, found.value());
                    }
                });

            _addCommand(
                "Layer",
                "Set a file's layer by name or index, e.g., { \"layer\": \"diffuse\" }. "
                "It is the \"A\" file's unless \"file\" gives the index of another "
                "in the file list.",
                [appWeak](const nlohmann::json& args)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto item = commandFile(app, args);
                        if (!args.is_object() || !args.contains("layer"))
                        {
                            throw std::invalid_argument("No \"layer\" given");
                        }
                        const auto& value = args.at("layer");
                        const std::string layer = value.is_number_integer() ?
                            std::to_string(value.get<int64_t>()) :
                            value.get<std::string>();
                        const auto found = models::findLayer(*item, layer);
                        if (!found.has_value())
                        {
                            throw std::invalid_argument(ftk::Format(
                                "{0} has no layer \"{1}\"; it has {2}").
                                arg(item->path.getFileName()).
                                arg(layer).
                                arg(ftk::join(item->videoLayers, ", ")));
                        }
                        app->getFilesModel()->setLayer(item, static_cast<int>(found.value()));
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
                        // exit() prompts about an unsaved review itself, for
                        // every way of quitting.
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
            _actions["OpenPlaylist"] = ftk::Action::create(
                "Open Playlist",
                _command("OpenPlaylist"));
            // In the tooltips map rather than set on the action, so the
            // shortcuts update does not put the command's plainer wording
            // back over it.
            _tooltips["OpenPlaylist"] =
                "Open a playlist into the file list. Opening a \".otio\" "
                "file normally plays it as a timeline.";
            _actions["SavePlaylist"] = ftk::Action::create(
                "Save Playlist",
                _command("SavePlaylist"));
            _tooltips["SavePlaylist"] =
                "Save the file list as a \".otio\" playlist.";
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
                    // The keys are only listed when there is a choice.
                    _actions["NextMediaReference"]->setEnabled(
                        value ? !value->mediaReferenceKeys.empty() : false);
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
