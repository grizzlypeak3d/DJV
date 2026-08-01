// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Review.h>

#include <ftk/Core/ObservableList.h>
#include <ftk/Core/Observable.h>

namespace ftk
{
    class CommandStack;
}

namespace djv
{
    namespace models
    {
        //! The drawing annotations of a review.
        //!
        //! Adding and erasing strokes go through a command stack, so they can be
        //! undone and redone. Annotations are session state: they live in the
        //! review file, not in the application settings.
        class AnnotationsModel : public std::enable_shared_from_this<AnnotationsModel>
        {
            FTK_NON_COPYABLE(AnnotationsModel);

        protected:
            void _init();

            AnnotationsModel();

        public:
            ~AnnotationsModel();

            //! Create a new model.
            static std::shared_ptr<AnnotationsModel> create();

            //! Get the annotations.
            const std::vector<ReviewAnnotation>& getAnnotations() const;

            //! Observe the annotations.
            std::shared_ptr<ftk::IObservableList<ReviewAnnotation> > observeAnnotations() const;

            //! Get the strokes drawn on the given source and frame, or an empty
            //! list if there are none.
            std::vector<ReviewStroke> getStrokes(
                const std::string& sourceId,
                const OTIO_NS::RationalTime&) const;

            //! Replace all the annotations, e.g. when a review is opened. This
            //! clears the undo history.
            void setAnnotations(const std::vector<ReviewAnnotation>&);

            //! Add a stroke to the given source and frame.
            void addStroke(
                const std::string& sourceId,
                const OTIO_NS::RationalTime&,
                const ReviewStroke&);

            //! Erase the strokes of the given source and frame whose path passes
            //! within the radius of the position. The position and the radius are
            //! in the pixels of the source image.
            void eraseStrokes(
                const std::string& sourceId,
                const OTIO_NS::RationalTime&,
                const ftk::V2F& pos,
                float radius);

            //! Remove every stroke of the given source and frame.
            void clearFrame(
                const std::string& sourceId,
                const OTIO_NS::RationalTime&);

            //! Remove all the annotations, clearing the undo history.
            void clear();

            //! \name Undo
            ///@{

            std::shared_ptr<ftk::IObservable<bool> > observeHasUndo() const;
            std::shared_ptr<ftk::IObservable<bool> > observeHasRedo() const;
            void undo();
            void redo();

            ///@}

        private:
            //! Replace the whole list without touching the undo history.
            void _set(const std::vector<ReviewAnnotation>&);

            //! Push a command that moves from the current list to the given one.
            void _push(const std::vector<ReviewAnnotation>&);

            FTK_PRIVATE();
        };
    }
}
