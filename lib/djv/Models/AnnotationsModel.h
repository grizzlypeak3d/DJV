// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

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
        class DJV_MODELS_API_TYPE AnnotationsModel : public std::enable_shared_from_this<AnnotationsModel>
        {
            FTK_NON_COPYABLE(AnnotationsModel);

        protected:
            DJV_MODELS_API void _init();

            AnnotationsModel();

        public:
            ~AnnotationsModel();

            //! Create a new model.
            DJV_MODELS_API static std::shared_ptr<AnnotationsModel> create();

            //! Get the annotations.
            DJV_MODELS_API const std::vector<ReviewAnnotation>& getAnnotations() const;

            //! Observe the annotations.
            DJV_MODELS_API std::shared_ptr<ftk::IObservableList<ReviewAnnotation> > observeAnnotations() const;

            //! Get the strokes drawn on the given source and frame, or an empty
            //! list if there are none.
            DJV_MODELS_API std::vector<ReviewStroke> getStrokes(
                const std::string& sourceId,
                const OTIO_NS::RationalTime&) const;

            //! Replace all the annotations, e.g. when a review is opened. This
            //! clears the undo history.
            DJV_MODELS_API void setAnnotations(const std::vector<ReviewAnnotation>&);

            //! Add a stroke to the given source and frame.
            DJV_MODELS_API void addStroke(
                const std::string& sourceId,
                const OTIO_NS::RationalTime&,
                const ReviewStroke&);

            //! Erase the strokes of the given source and frame whose path passes
            //! within the radius of the position. The position and the radius are
            //! in the pixels of the source image.
            DJV_MODELS_API void eraseStrokes(
                const std::string& sourceId,
                const OTIO_NS::RationalTime&,
                const ftk::V2F& pos,
                float radius);

            //! Remove every stroke of the given source and frame.
            DJV_MODELS_API void clearFrame(
                const std::string& sourceId,
                const OTIO_NS::RationalTime&);

            //! Remove all the annotations, clearing the undo history.
            DJV_MODELS_API void clear();

            //! \name Undo
            ///@{

            DJV_MODELS_API std::shared_ptr<ftk::IObservable<bool> > observeHasUndo() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<bool> > observeHasRedo() const;
            DJV_MODELS_API void undo();
            DJV_MODELS_API void redo();

            ///@}

        private:
            //! Replace the whole list without touching the undo history.
            DJV_MODELS_API void _set(const std::vector<ReviewAnnotation>&);

            //! Push a command that moves from the current list to the given one.
            DJV_MODELS_API void _push(const std::vector<ReviewAnnotation>&);

            FTK_PRIVATE();
        };
    }
}
