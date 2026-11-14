// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <djv/Models/Review.h>

#include <ftk/UI/IDialog.h>

namespace djv
{
    namespace ui
    {
        //! The dialog Detect Cuts asks with before it reads the frames: what
        //! the markers it adds are called, and their color.
        class DJV_UI_API_TYPE DetectCutsDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(DetectCutsDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const models::CutMarkerOptions&,
                const std::shared_ptr<IWidget>& parent);

            DetectCutsDialog();

        public:
            DJV_UI_API virtual ~DetectCutsDialog();

            //! Create a new dialog, starting from the given options.
            DJV_UI_API static std::shared_ptr<DetectCutsDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const models::CutMarkerOptions& = models::CutMarkerOptions(),
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the callback, called with the options when Detect is
            //! clicked. The dialog does not close itself.
            DJV_UI_API void setCallback(
                const std::function<void(const models::CutMarkerOptions&)>&);

            DJV_UI_API std::shared_ptr<ftk::IWidget> getKeyFocus() const override;

        private:
            FTK_PRIVATE();
        };
    }
}
