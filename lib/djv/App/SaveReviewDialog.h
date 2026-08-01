// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IDialog.h>

namespace djv
{
    namespace app
    {
        //! The user's choice when prompted to save a review before closing.
        enum class SaveReviewResult
        {
            Save,
            Discard,
            Cancel
        };

        //! A three-button dialog asking whether to save the current review
        //! before closing: Save, Don't Save, or Cancel.
        class SaveReviewDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(SaveReviewDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::string& title,
                const std::string& text,
                const std::shared_ptr<IWidget>& parent);

            SaveReviewDialog();

        public:
            virtual ~SaveReviewDialog();

            static std::shared_ptr<SaveReviewDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const std::string& title,
                const std::string& text,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setCallback(const std::function<void(SaveReviewResult)>&);

        private:
            FTK_PRIVATE();
        };
    }
}
