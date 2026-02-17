// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/FilesModel.h>

#include <ftk/UI/Menu.h>

#include <filesystem>

namespace djv
{
    namespace app
    {
        class App;
        class FileActions;

        //! File menu.
        class FileMenu : public ftk::Menu
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
            ~FileMenu();

            static std::shared_ptr<FileMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<FileActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void close() override;

        private:
            void _filesUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >&);
            void _aUpdate(const std::shared_ptr<models::FilesModelItem>&);
            void _aIndexUpdate(int);
            void _layersUpdate(const std::vector<int>&);
            void _recentUpdate(const std::vector<std::filesystem::path>&);

            FTK_PRIVATE();
        };
    }
}
