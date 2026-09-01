// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/Models/Export.h>

#include <ftk/UI/Menu.h>

#include <ftk/Core/Path.h>

namespace djv
{
    namespace app
    {
        class App;
        class ReviewActions;

        //! Review menu.
        class DJV_APP_API_TYPE ReviewMenu : public ftk::Menu
        {
            FTK_NON_COPYABLE(ReviewMenu);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<ReviewActions>&,
                const std::shared_ptr<IWidget>& parent);

            ReviewMenu();

        public:
            DJV_APP_API ~ReviewMenu();

            DJV_APP_API static std::shared_ptr<ReviewMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<ReviewActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_APP_API void close() override;

        private:
            void _recentUpdate(const std::vector<ftk::Path>&);

            FTK_PRIVATE();
        };
    }
}
