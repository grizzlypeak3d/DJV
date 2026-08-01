// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IDialog.h>

namespace djv
{
    namespace app
    {
        //! A dialog asking for the name of a review range.
        //!
        //! The field starts filled with the frame range and selected, so that
        //! confirming straight away keeps that default and typing replaces it.
        class RangeNameDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(RangeNameDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::string& title,
                const std::string& text,
                const std::string& name,
                const std::shared_ptr<IWidget>& parent);

            RangeNameDialog();

        public:
            virtual ~RangeNameDialog();

            static std::shared_ptr<RangeNameDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const std::string& title,
                const std::string& text,
                const std::string& name,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Called with the name when the dialog is accepted. Cancelling
            //! closes the dialog without calling back.
            void setCallback(const std::function<void(const std::string&)>&);

        private:
            FTK_PRIVATE();
        };
    }
}
