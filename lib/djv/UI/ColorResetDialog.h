// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <ftk/UI/IDialog.h>

namespace djv
{
    namespace ui
    {
        //! The sections of the color tool to reset.
        struct DJV_UI_API_TYPE ColorResetGroups
        {
            bool ocio   = true;
            bool lut    = true;
            bool color  = true;
            bool levels = true;

            bool operator == (const ColorResetGroups&) const = default;
        };

        //! Color reset dialog: which sections of the color tool to reset.
        //! The sections that have something to reset are ticked; the ones
        //! already at their defaults are shown but cannot be ticked, so the
        //! dialog says what resetting would change.
        class DJV_UI_API_TYPE ColorResetDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(ColorResetDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const ColorResetGroups& set,
                const std::shared_ptr<IWidget>& parent);

            ColorResetDialog();

        public:
            DJV_UI_API virtual ~ColorResetDialog();

            //! Create a new dialog; "set" is the sections that differ from
            //! their defaults.
            DJV_UI_API static std::shared_ptr<ColorResetDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const ColorResetGroups& set,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the callback, run when "Reset" is clicked; the dialog
            //! then closes itself.
            DJV_UI_API void setCallback(const std::function<void(const ColorResetGroups&)>&);

            //! The cancel button: the key that takes the dialog at face
            //! value is the one that changes nothing.
            DJV_UI_API std::shared_ptr<ftk::IWidget> getKeyFocus() const override;

        private:
            FTK_PRIVATE();
        };
    }
}
