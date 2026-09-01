// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/IToolWidget.h>

#include <djv/Models/Review.h>

namespace ftk
{
    class TextEdit;
}

namespace djv
{
    namespace app
    {
        //! Review tool: annotations and notes.
        //!
        //! The drawing and notes sections share one panel so that marking up a
        //! frame and commenting on it stay visible together.
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

            //! Start a new note about the current frame, edited in place in
            //! the list.
            void addNote();

            void setGeometry(const ftk::Box2I&) override;
            void keyPressEvent(ftk::KeyEvent&) override;
            void keyReleaseEvent(ftk::KeyEvent&) override;

        private:
            void _drawStateUpdate();
            //! Move the key focus along a list, and follow a note's frame.
            //! Without a focused row, the list under the cursor is entered
            //! at its selection.
            bool _navigate(bool down, const ftk::V2I& pos);
            void _goToNote(size_t index);
            //! Seek, releasing the in/out range when the target lies
            //! outside it.
            void _seekTo(const OTIO_NS::RationalTime&);
            void _editNote(const std::string& id);
            void _commitNote();
            void _editFocus(const std::shared_ptr<ftk::TextEdit>&, bool);
            void _noteClicked(const std::string& id);
            void _notesUpdate();
            void _noteSelectionUpdate();
            void _rangesUpdate();
            void _rangeSelectionUpdate();
            void _rangeClicked(const std::string& id);
            void _inOutUpdate();
            void _addRange();

            FTK_PRIVATE();
        };
    }
}
