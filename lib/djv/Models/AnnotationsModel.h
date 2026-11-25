// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <djv/Models/Review.h>
#include <djv/Models/Stroke.h>

#include <ftk/Core/ObservableList.h>
#include <ftk/Core/Observable.h>

#include <optional>

namespace ftk
{
    class CommandStack;
}

namespace djv
{
    namespace models
    {
        //! A stroke as it is drawn, before it is added: what one view is
        //! drawing, for the other views of the same files to draw as well
        //! (a secondary window on a reference monitor).
        struct DJV_MODELS_API_TYPE LiveStroke
        {
            std::string           sourceId;
            OTIO_NS::RationalTime time;
            ReviewStroke          stroke;
            //! The index in getStrokes() of the stroke it stands in for, when
            //! one is moved or its text written again: drawn here, not there.
            std::optional<size_t> replaces;

            DJV_MODELS_API bool operator == (const LiveStroke&) const;
            DJV_MODELS_API bool operator != (const LiveStroke&) const;
        };

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
            DJV_MODELS_API ~AnnotationsModel();

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
            //! in the pixels of the source image, and text
            //! covers what textBox() has it cover in the view.
            DJV_MODELS_API void eraseStrokes(
                const std::string& sourceId,
                const OTIO_NS::RationalTime&,
                const ftk::V2F& pos,
                float radius,
                const TextView& = TextView());

            //! Find the stroke of the given source and frame that passes
            //! within the radius of the position, the one on top where there
            //! are several: its index in getStrokes(), or none. The position
            //! and the radius are in the pixels of the source image, and text
            //! covers what textBox() has it cover in the view.
            DJV_MODELS_API std::optional<size_t> findStroke(
                const std::string& sourceId,
                const OTIO_NS::RationalTime&,
                const ftk::V2F& pos,
                float radius,
                const TextView& = TextView()) const;

            //! Replace a stroke of the given source and frame, by its index
            //! in getStrokes(): moved, recolored, resized, or text written
            //! again. One undo step.
            DJV_MODELS_API void setStroke(
                const std::string& sourceId,
                const OTIO_NS::RationalTime&,
                size_t index,
                const ReviewStroke&);

            //! Remove a stroke of the given source and frame, by its index
            //! in getStrokes().
            DJV_MODELS_API void removeStroke(
                const std::string& sourceId,
                const OTIO_NS::RationalTime&,
                size_t index);

            //! Remove every stroke of the given sources and frame. One call
            //! for all of the sources, so that clearing a compared frame is
            //! a single undo step.
            DJV_MODELS_API void clearFrame(
                const std::vector<std::string>& sourceIds,
                const OTIO_NS::RationalTime&);

            //! Remove every stroke of the given sources, on every frame, as
            //! one undo step: a file's drawings, the others' left.
            DJV_MODELS_API void clearSources(const std::vector<std::string>& sourceIds);

            //! Remove all the annotations, clearing the undo history.
            DJV_MODELS_API void clear();

            //! \name Live Stroke
            //! The stroke being drawn, which is not an annotation until it
            //! is added, and no undo step.
            ///@{

            DJV_MODELS_API const std::optional<LiveStroke>& getLiveStroke() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<std::optional<LiveStroke> > > observeLiveStroke() const;
            DJV_MODELS_API void setLiveStroke(const std::optional<LiveStroke>&);

            ///@}

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
