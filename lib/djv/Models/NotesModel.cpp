// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/NotesModel.h>

#include <algorithm>

namespace djv
{
    namespace models
    {
        struct NotesModel::Private
        {
            std::shared_ptr<ftk::ObservableList<ReviewNote> > notes;
        };

        void NotesModel::_init()
        {
            FTK_P();
            p.notes = ftk::ObservableList<ReviewNote>::create();
        }

        NotesModel::NotesModel() :
            _p(new Private)
        {}

        NotesModel::~NotesModel()
        {}

        std::shared_ptr<NotesModel> NotesModel::create()
        {
            auto out = std::shared_ptr<NotesModel>(new NotesModel);
            out->_init();
            return out;
        }

        const std::vector<ReviewNote>& NotesModel::getNotes() const
        {
            return _p->notes->get();
        }

        std::shared_ptr<ftk::IObservableList<ReviewNote> > NotesModel::observeNotes() const
        {
            return _p->notes;
        }

        void NotesModel::setNotes(const std::vector<ReviewNote>& value)
        {
            _p->notes->setIfChanged(value);
        }

        void NotesModel::add(const std::optional<OTIO_NS::RationalTime>& time, const std::string& text)
        {
            FTK_P();
            ReviewNote note;
            note.id = generateId();
            note.time = time;
            note.created = timestamp();
            note.author = reviewAuthor();
            note.text = text;
            auto notes = p.notes->get();
            notes.push_back(note);
            p.notes->setIfChanged(notes);
        }

        void NotesModel::update(const std::string& id, const std::string& text)
        {
            FTK_P();
            auto notes = p.notes->get();
            const auto i = std::find_if(
                notes.begin(),
                notes.end(),
                [id](const ReviewNote& note) { return note.id == id; });
            if (i != notes.end())
            {
                i->text = text;
                p.notes->setIfChanged(notes);
            }
        }

        void NotesModel::remove(const std::string& id)
        {
            FTK_P();
            auto notes = p.notes->get();
            const auto i = std::find_if(
                notes.begin(),
                notes.end(),
                [id](const ReviewNote& note) { return note.id == id; });
            if (i != notes.end())
            {
                notes.erase(i);
                p.notes->setIfChanged(notes);
            }
        }

        void NotesModel::clear()
        {
            _p->notes->setIfChanged({});
        }
    }
}
