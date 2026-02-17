// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IDialog.h>

namespace djv
{
    namespace ui
    {
        //! System information dialog.
        class SysInfoDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(SysInfoDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::IWindow>&,
                const std::shared_ptr<IWidget>& parent);

            SysInfoDialog();

        public:
            virtual ~SysInfoDialog();

            static std::shared_ptr<SysInfoDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::IWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
