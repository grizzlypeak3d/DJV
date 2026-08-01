// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Review.h>

#include <ftk/Core/ObservableList.h>

namespace djv
{
    namespace models
    {
        //! The review notes.
        //!
        //! Notes are session state: they live in the review file, not in the
        //! application settings.
        class NotesModel : public std::enable_shared_from_this<NotesModel>
        {
            FTK_NON_COPYABLE(NotesModel);

        protected:
            void _init();

            NotesModel();

        public:
            ~NotesModel();

            //! Create a new model.
            static std::shared_ptr<NotesModel> create();

            //! Get the notes.
            const std::vector<ReviewNote>& getNotes() const;

            //! Observe the notes.
            std::shared_ptr<ftk::IObservableList<ReviewNote> > observeNotes() const;

            //! Replace all the notes, e.g. when a review is opened.
            void setNotes(const std::vector<ReviewNote>&);

            //! Add a note. The identifier and creation time are filled in here,
            //! so callers only provide the frame and the text.
            void add(const std::optional<OTIO_NS::RationalTime>&, const std::string& text);

            //! Remove the note with the given identifier.
            void remove(const std::string& id);

            //! Remove all the notes.
            void clear();

        private:
            FTK_PRIVATE();
        };
    }
}
