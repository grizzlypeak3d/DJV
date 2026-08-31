// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/Models/Export.h>
#include <djv/Models/FilesModel.h>

#include <ftk/UI/Menu.h>

#include <filesystem>

namespace tl
{
    class Player;
}

namespace djv
{
    namespace app
    {
        class App;
        class FileActions;

        //! File menu.
        class DJV_APP_API_TYPE FileMenu : public ftk::Menu
        {
            FTK_NON_COPYABLE(FileMenu);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<FileActions>&,
                const std::shared_ptr<IWidget>& parent);

            FileMenu();

        public:
            DJV_APP_API ~FileMenu();

            DJV_APP_API static std::shared_ptr<FileMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<FileActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_APP_API void close() override;

        private:
            void _filesUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >&);
            void _aUpdate(const std::shared_ptr<models::FilesModelItem>&);
            void _aIndexUpdate(int);
            void _layersUpdate(const std::vector<int>&);
            void _recentUpdate(const std::vector<ftk::Path>&);
            void _recentPlaylistsUpdate(const std::vector<ftk::Path>&);
            void _recentReviewsUpdate(const std::vector<ftk::Path>&);
            void _setPlayer(const std::shared_ptr<tl::Player>&);
            void _mediaReferencesUpdate();
            void _mediaReferenceKeyUpdate(const std::string&);

            FTK_PRIVATE();
        };
    }
}
