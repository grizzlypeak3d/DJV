// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/FileMenu.h>

#include <djv/App/App.h>
#include <djv/App/FileActions.h>
#include <djv/Models/RecentFilesModel.h>

#include <tlRender/Timeline/Player.h>
#include <tlRender/Timeline/Util.h>
#include <tlRender/IO/System.h>

namespace djv
{
    namespace app
    {
        struct FileMenu::Private
        {
            std::weak_ptr<App> app;
            std::vector<std::string> exts;
            std::shared_ptr<ftk::RecentFilesModel> recentFilesModel;

            std::vector<std::shared_ptr<ftk::Action> > currentActions;
            std::vector<std::shared_ptr<ftk::Action> > layersActions;
            std::vector<std::shared_ptr<ftk::Action> > mediaReferencesActions;
            // The keys the media reference actions set, in the same order.
            // The first is empty, for leaving the clips as authored.
            std::vector<std::string> mediaReferenceKeys;
            std::map<std::string, std::shared_ptr<ftk::Menu> > menus;
            std::shared_ptr<tl::Player> player;

            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<models::FilesModelItem> > > aObserver;
            std::shared_ptr<ftk::Observer<int> > aIndexObserver;
            std::shared_ptr<ftk::ListObserver<int> > layersObserver;
            std::shared_ptr<ftk::ListObserver<std::filesystem::path> > recentObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<std::string> > mediaReferenceKeyObserver;
        };

        void FileMenu::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<FileActions>& fileActions,
            const std::shared_ptr<IWidget>& parent)
        {
            Menu::_init(context, parent);
            FTK_P();

            p.app = app;

            p.exts = tl::getExts(context);

            p.recentFilesModel = app->getRecentFilesModel();

            auto actions = fileActions->getActions();
            addAction(actions["Open"]);
            addAction(actions["NewPlaylist"]);
            addAction(actions["NewPlaylistFolder"]);
            addAction(actions["OpenAudio"]);
            addAction(actions["Close"]);
            addAction(actions["CloseAll"]);
            addAction(actions["Reload"]);
            p.menus["Recent"] = addSubMenu("Recent");
            addDivider();
            p.menus["Current"] = addSubMenu("Current");
            addAction(actions["Next"]);
            addAction(actions["Prev"]);
            addDivider();
            p.menus["Layers"] = addSubMenu("Layers");
            addAction(actions["NextLayer"]);
            addAction(actions["PrevLayer"]);
            addDivider();
            p.menus["MediaReferences"] = addSubMenu("Media References");
            addAction(actions["NextMediaReference"]);
            addDivider();
            addAction(actions["Exit"]);

            p.filesObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    _filesUpdate(value);
                });

            p.aObserver = ftk::Observer<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeA(),
                [this](const std::shared_ptr<models::FilesModelItem>& value)
                {
                    _aUpdate(value);
                });

            p.aIndexObserver = ftk::Observer<int>::create(
                app->getFilesModel()->observeAIndex(),
                [this](int value)
                {
                    _aIndexUpdate(value);
                });

            p.layersObserver = ftk::ListObserver<int>::create(
                app->getFilesModel()->observeLayers(),
                [this](const std::vector<int>& value)
                {
                    _layersUpdate(value);
                });

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    _setPlayer(value);
                });

            p.recentObserver = ftk::ListObserver<std::filesystem::path>::create(
                p.recentFilesModel->observeRecent(),
                [this](const std::vector<std::filesystem::path>& value)
                {
                    _recentUpdate(value);
                });
        }

        FileMenu::FileMenu() :
            _p(new Private)
        {}

        FileMenu::~FileMenu()
        {}

        std::shared_ptr<FileMenu> FileMenu::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<FileActions>& fileActions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<FileMenu>(new FileMenu);
            out->_init(context, app, fileActions, parent);
            return out;
        }

        void FileMenu::close()
        {
            Menu::close();
            FTK_P();
            for (const auto& menu : p.menus)
            {
                menu.second->close();
            }
        }

        void FileMenu::_filesUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
        {
            FTK_P();
            p.menus["Current"]->clear();
            p.currentActions.clear();
            for (size_t i = 0; i < value.size(); ++i)
            {
                auto action = ftk::Action::create(
                    value[i]->path.getFileName(),
                    [this, i]
                    {
                        close();
                        if (auto app = _p->app.lock())
                        {
                            app->getFilesModel()->setA(i);
                        }
                    });
                p.menus["Current"]->addAction(action);
                p.currentActions.push_back(action);
            }
        }

        void FileMenu::_aUpdate(const std::shared_ptr<models::FilesModelItem>& value)
        {
            FTK_P();

            p.menus["Layers"]->clear();
            p.layersActions.clear();
            if (value)
            {
                for (size_t i = 0; i < value->videoLayers.size(); ++i)
                {
                    auto action = ftk::Action::create(
                        value->videoLayers[i],
                        [this, value, i]
                        {
                            close();
                            if (auto app = _p->app.lock())
                            {
                                app->getFilesModel()->setLayer(value, i);
                            }
                        });
                    action->setChecked(i == value->videoLayer);
                    p.menus["Layers"]->addAction(action);
                    p.layersActions.push_back(action);
                }
            }
        }

        void FileMenu::_aIndexUpdate(int value)
        {
            FTK_P();
            for (int i = 0; i < static_cast<int>(p.currentActions.size()); ++i)
            {
                p.menus["Current"]->setChecked(p.currentActions[i], i == value);
            }
        }

        void FileMenu::_layersUpdate(const std::vector<int>& value)
        {
            FTK_P();
            if (auto app = p.app.lock())
            {
                auto a = app->getFilesModel()->getA();
                if (a)
                {
                    for (size_t i = 0; i < p.layersActions.size(); ++i)
                    {
                        p.menus["Layers"]->setChecked(p.layersActions[i], i == a->videoLayer);
                    }
                }
            }
        }

        void FileMenu::_setPlayer(const std::shared_ptr<tl::Player>& value)
        {
            FTK_P();
            p.player = value;

            // The list of keys is rebuilt first, so that the observer below
            // has actions to check when it reports the current key.
            p.mediaReferenceKeyObserver.reset();
            _mediaReferencesUpdate();
            if (p.player)
            {
                p.mediaReferenceKeyObserver = ftk::Observer<std::string>::create(
                    p.player->observeMediaReferenceKey(),
                    [this](const std::string& value)
                    {
                        _mediaReferenceKeyUpdate(value);
                    });
            }
        }

        void FileMenu::_mediaReferencesUpdate()
        {
            FTK_P();
            p.menus["MediaReferences"]->clear();
            p.mediaReferencesActions.clear();
            p.mediaReferenceKeys.clear();
            if (p.player)
            {
                // The keys a timeline uses are not observable, so the list is
                // rebuilt when the player changes.
                std::vector<std::string> keys = { std::string() };
                for (const auto& key : p.player->getMediaReferenceKeys())
                {
                    keys.push_back(key);
                }
                for (const auto& key : keys)
                {
                    // The empty key leaves each clip on the media reference it
                    // was authored with, which is where a timeline starts.
                    auto action = ftk::Action::create(
                        !key.empty() ? key : "As Authored",
                        [this, key]
                        {
                            close();
                            if (_p->player)
                            {
                                _p->player->setMediaReferenceKey(key);
                            }
                        });
                    p.menus["MediaReferences"]->addAction(action);
                    p.mediaReferencesActions.push_back(action);
                    p.mediaReferenceKeys.push_back(key);
                }
                _mediaReferenceKeyUpdate(p.player->getMediaReferenceKey());
            }
        }

        void FileMenu::_mediaReferenceKeyUpdate(const std::string& value)
        {
            FTK_P();
            for (size_t i = 0; i < p.mediaReferencesActions.size(); ++i)
            {
                p.menus["MediaReferences"]->setChecked(
                    p.mediaReferencesActions[i],
                    p.mediaReferenceKeys[i] == value);
            }
        }

        void FileMenu::_recentUpdate(const std::vector<std::filesystem::path>& value)
        {
            FTK_P();
            p.menus["Recent"]->clear();
            for (auto i = value.rbegin(); i != value.rend(); ++i)
            {
                const auto path = *i;
                auto weak = std::weak_ptr<FileMenu>(std::dynamic_pointer_cast<FileMenu>(shared_from_this()));
                auto action = ftk::Action::create(
                    path.u8string(),
                    [weak, path]
                    {
                        if (auto widget = weak.lock())
                        {
                            if (auto app = widget->_p->app.lock())
                            {
                                app->open(ftk::Path(path.u8string()));
                            }
                            widget->close();
                        }
                    });
                p.menus["Recent"]->addAction(action);
            }
        }
    }
}
