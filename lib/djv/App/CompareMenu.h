// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/Models/Export.h>
#include <djv/Models/FilesModel.h>

#include <ftk/UI/Menu.h>

namespace djv
{
    namespace app
    {
        class App;
        class CompareActions;

        //! Compare menu.
        class DJV_APP_API_TYPE CompareMenu : public ftk::Menu
        {
            FTK_NON_COPYABLE(CompareMenu);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<CompareActions>&,
                const std::shared_ptr<IWidget>& parent);

            CompareMenu();

        public:
            DJV_APP_API ~CompareMenu();

            DJV_APP_API static std::shared_ptr<CompareMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<CompareActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_APP_API void close() override;

        private:
            void _filesUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >&);
            void _bUpdate(const std::vector<int>&);

            FTK_PRIVATE();
        };
    }
}
