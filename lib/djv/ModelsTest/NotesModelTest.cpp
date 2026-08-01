// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/NotesModelTest.h>

#include <djv/Models/NotesModel.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace models_tests
    {
        NotesModelTest::NotesModelTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::NotesModelTest")
        {}

        std::shared_ptr<NotesModelTest> NotesModelTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<NotesModelTest>(new NotesModelTest(context));
        }

        void NotesModelTest::run()
        {
            _notes();
            _serialize();
        }

        void NotesModelTest::_notes()
        {
            auto model = models::NotesModel::create();

            size_t count = 0;
            auto observer = ftk::ListObserver<models::ReviewNote>::create(
                model->observeNotes(),
                [&count](const std::vector<models::ReviewNote>& value) { count = value.size(); });

            FTK_ASSERT(model->getNotes().empty());

            // Adding fills in the identifier and the creation time; the caller
            // only supplies the frame and the text.
            model->add(OTIO_NS::RationalTime(128.0, 24.0), "First note.");
            model->add(OTIO_NS::RationalTime(256.0, 24.0), "Second note.");
            FTK_ASSERT(2 == model->getNotes().size());
            FTK_ASSERT(2 == count);
            const auto& notes = model->getNotes();
            FTK_ASSERT(!notes[0].id.empty());
            FTK_ASSERT(!notes[0].created.empty());
            FTK_ASSERT(notes[0].id != notes[1].id);
            FTK_ASSERT(128.0 == notes[0].time->value());

            // Removing by identifier leaves the other note alone.
            const std::string id = notes[0].id;
            model->remove(id);
            FTK_ASSERT(1 == model->getNotes().size());
            FTK_ASSERT("Second note." == model->getNotes()[0].text);

            // Removing an unknown identifier is a no-op.
            model->remove("does-not-exist");
            FTK_ASSERT(1 == model->getNotes().size());

            model->clear();
            FTK_ASSERT(model->getNotes().empty());

            const bool ok =
                0 == model->getNotes().size() &&
                0 == count;
            _print(ftk::Format("  notes add/remove/clear -> {0}").arg(ok ? "ok" : "FAILED"));
            if (!ok)
            {
                _error("NotesModel add/remove/clear failed");
            }
        }

        void NotesModelTest::_serialize()
        {
            // A note must survive a round trip through the review file, since
            // that is how it is persisted.
            models::ReviewNote note;
            note.id = "abc123";
            note.time = OTIO_NS::RationalTime(218.0, 24.0);
            note.created = "2026-07-26T18:05:00Z";
            note.text = "Line one.\nLine two.";

            const nlohmann::json json = note;
            const auto out = json.get<models::ReviewNote>();

            FTK_ASSERT(note == out);

            const bool ok = note == out;
            _print(ftk::Format("  note JSON round trip -> {0}").arg(ok ? "ok" : "FAILED"));
            if (!ok)
            {
                _error("ReviewNote JSON round trip failed");
            }
        }
    }
}
