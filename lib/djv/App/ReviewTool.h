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
        //! Review tool: drawing and markers.
        //!
        //! The drawing and marker sections share one panel so that marking up
        //! a frame and commenting on it stay visible together.
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

            //! Add a marker about the current frame, edited in place in
            //! the list.
            void addNote();

            //! Add a marker for the timeline in/out points.
            void addRange();

            void setGeometry(const ftk::Box2I&) override;
            void keyPressEvent(ftk::KeyEvent&) override;
            void keyReleaseEvent(ftk::KeyEvent&) override;

        private:
            void _drawStateUpdate();
            //! Move the key focus along the list, and follow a marker's
            //! frames. Without a focused row, the list is entered at its
            //! selection when the cursor is over it.
            bool _navigate(bool down, const ftk::V2I& pos);
            void _goToRow(size_t index);
            void _rowFocus(const std::string& id, bool value);
            //! What the header delete button would delete: the focused row,
            //! or failing that the active one -- the applied span, the
            //! marker on the current frame.
            std::string _deleteTarget() const;
            void _deleteButtonUpdate();
            void _deleteMarker();
            //! Seek, releasing the in/out range when the target lies
            //! outside it.
            void _seekTo(const OTIO_NS::RationalTime&);
            void _editMarker(const std::string& id);
            void _commitMarker();
            void _editFocus(const std::shared_ptr<ftk::TextEdit>&, bool);
            void _markerClicked(const std::string& id);
            //! Seek to a marker's frames; a span narrows the timeline
            //! in/out points to itself on the way.
            void _goToRange(const OTIO_NS::TimeRange&);
            void _markersUpdate();
            void _selectionUpdate();
            void _inOutUpdate();

            FTK_PRIVATE();
        };
    }
}
