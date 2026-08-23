// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>
#include <djv/Models/Export.h>

#include <ftk/UI/IDialog.h>

namespace djv
{
    namespace ui
    {
        //! System information dialog.
        class DJV_UI_API_TYPE SysInfoDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(SysInfoDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::vector<std::string>&,
                const std::shared_ptr<IWidget>& parent);

            SysInfoDialog();

        public:
            DJV_UI_API virtual ~SysInfoDialog();

            DJV_UI_API static std::shared_ptr<SysInfoDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const std::vector<std::string>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_UI_API std::shared_ptr<ftk::IWidget> getKeyFocus() const override;

        private:
            FTK_PRIVATE();
        };
    }
}
