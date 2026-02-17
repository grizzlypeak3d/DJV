// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IDialog.h>

namespace djv
{
    namespace ui
    {
        //! About dialog.
        class AboutDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(AboutDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            AboutDialog();

        public:
            virtual ~AboutDialog();

            static std::shared_ptr<AboutDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
