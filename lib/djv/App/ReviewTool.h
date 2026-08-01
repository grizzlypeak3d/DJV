// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/IToolWidget.h>

#include <djv/Models/Review.h>

namespace djv
{
    namespace app
    {
        //! Review tool: annotations and notes.
        //!
        //! The drawing and notes sections share one panel so that marking up a
        //! frame and commenting on it stay visible together. See
        //! docs/ROADMAP_REVIEW_SESSIONS.md section 7.2.bis.
        class ReviewTool : public IToolWidget
        {
            FTK_NON_COPYABLE(ReviewTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent);

            ReviewTool();

        public:
            virtual ~ReviewTool();

            static std::shared_ptr<ReviewTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            void _drawStateUpdate();
            void _publish();
            void _notesUpdate();
            void _rangesUpdate();
            void _rangeSelectionUpdate();
            void _rangeClicked(const std::string& id);
            void _inOutUpdate();
            void _addRange();

            FTK_PRIVATE();
        };
    }
}
