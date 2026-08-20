// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <ftk/UI/IDialog.h>

namespace djv
{
    namespace models
    {
        class AppInfoModel;
    }

    namespace ui
    {
        //! About dialog.
        class DJV_API_TYPE AboutDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(AboutDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::AppInfoModel>&,
                const std::shared_ptr<IWidget>& parent);

            AboutDialog();

        public:
            DJV_API virtual ~AboutDialog();

            DJV_API static std::shared_ptr<AboutDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::AppInfoModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_API std::shared_ptr<ftk::IWidget> getKeyFocus() const override;

        private:
            FTK_PRIVATE();
        };
    }
}
