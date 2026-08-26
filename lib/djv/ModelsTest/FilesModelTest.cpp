// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/FilesModelTest.h>

#include <djv/ModelsTest/ModelsTestUtil.h>

#include <djv/Models/FilesModel.h>

#include <tlRender/Timeline/CompareOptions.h>

#include <ftk/UI/Settings.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Math.h>
#include <ftk/Core/Observable.h>
#include <ftk/Core/ObservableList.h>
#include <ftk/Core/Path.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace djv
{
    namespace models_tests
    {
        namespace
        {
            std::shared_ptr<models::FilesModelItem> makeItem(const std::string& name)
            {
                auto out = std::make_shared<models::FilesModelItem>();
                out->path = ftk::Path(name);
                return out;
            }
        }

        FilesModelTest::FilesModelTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::FilesModelTest")
        {}

        std::shared_ptr<FilesModelTest> FilesModelTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<FilesModelTest>(new FilesModelTest(context));
        }

        void FilesModelTest::run()
        {
            _files();
            _navigation();
            _compare();
            _tileCompare();
            _reviewRestore();
            _frames();
            _persistence();
        }

        void FilesModelTest::_files()
        {
            typedef std::vector<std::shared_ptr<models::FilesModelItem> > Items;

            auto settings = createTestSettings(_context);
            auto model = models::FilesModel::create(settings);

            // Observe the files list.
            size_t count = 0;
            auto filesObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                model->observeFiles(),
                [&count](const Items& value) { count = value.size(); });

            // Empty by default.
            FTK_ASSERT(model->getFiles().empty());
            FTK_ASSERT(!model->getA());

            // Adding a file appends it and makes it the "A" file; the observer
            // fires.
            auto item0 = makeItem("file0.exr");
            model->add(item0);
            FTK_ASSERT(1 == model->getFiles().size());
            FTK_ASSERT(item0 == model->getA());
            FTK_ASSERT(0 == model->getAIndex());
            FTK_ASSERT(1 == count);

            auto item1 = makeItem("file1.exr");
            auto item2 = makeItem("file2.exr");
            model->add(item1);
            model->add(item2);
            FTK_ASSERT(3 == model->getFiles().size());
            FTK_ASSERT(item2 == model->getA());
            FTK_ASSERT(2 == model->getAIndex());

            // Closing a file removes it and re-clamps the "A" index.
            model->close(1);
            FTK_ASSERT(2 == model->getFiles().size());

            // Closing all empties the list and clears the "A" file.
            model->closeAll();
            FTK_ASSERT(model->getFiles().empty());
            FTK_ASSERT(!model->getA());
        }

        void FilesModelTest::_navigation()
        {
            auto settings = createTestSettings(_context);
            auto model = models::FilesModel::create(settings);
            model->add(makeItem("file0.exr"));
            model->add(makeItem("file1.exr"));
            model->add(makeItem("file2.exr"));

            // "A" navigation. next()/prev() wrap around the ends.
            model->setA(0);
            FTK_ASSERT(0 == model->getAIndex());
            model->next();
            FTK_ASSERT(1 == model->getAIndex());
            model->next();
            model->next();                     // wraps from the last to the first
            FTK_ASSERT(0 == model->getAIndex());
            model->prev();                     // wraps from the first to the last
            FTK_ASSERT(2 == model->getAIndex());
            model->first();
            FTK_ASSERT(0 == model->getAIndex());
            model->last();
            FTK_ASSERT(2 == model->getAIndex());

            // "B" files (compare). In the default single-buffer compare mode,
            // selecting a "B" file replaces any previous one; toggling the same
            // index again removes it. (Multiple simultaneous "B" files are only
            // allowed in tile mode, whose transition behavior is left to a
            // dedicated compare-options test.)
            model->setA(0);
            model->toggleB(1);
            FTK_ASSERT(std::vector<int>({ 1 }) == model->getBIndexes());
            model->toggleB(2);                 // replaces (single-buffer mode)
            FTK_ASSERT(std::vector<int>({ 2 }) == model->getBIndexes());
            model->toggleB(2);                 // toggle off
            FTK_ASSERT(model->getBIndexes().empty());

            // clearB() removes all "B" files.
            model->toggleB(1);
            FTK_ASSERT(!model->getBIndexes().empty());
            model->clearB();
            FTK_ASSERT(model->getBIndexes().empty());
        }

        void FilesModelTest::_compare()
        {
            auto settings = createTestSettings(_context);
            auto model = models::FilesModel::create(settings);

            // Compare time (an enum; the default is Relative).
            tl::CompareTime compareTime = tl::CompareTime::First;
            auto compareTimeObserver = ftk::Observer<tl::CompareTime>::create(
                model->observeCompareTime(),
                [&compareTime](const tl::CompareTime& value) { compareTime = value; });
            FTK_ASSERT(tl::CompareTime::Relative == model->getCompareTime());
            model->setCompareTime(tl::CompareTime::Absolute);
            FTK_ASSERT(tl::CompareTime::Absolute == model->getCompareTime());
            FTK_ASSERT(tl::CompareTime::Absolute == compareTime);

            // Compare options.
            bool observed = false;
            auto compareOptionsObserver = ftk::Observer<tl::CompareOptions>::create(
                model->observeCompareOptions(),
                [&observed](const tl::CompareOptions&) { observed = true; });
            observed = false;
            tl::CompareOptions options;
            options.overlay = 0.75F;
            model->setCompareOptions(options);
            FTK_ASSERT(ftk::fuzzyCompare(0.75F, model->getCompareOptions().overlay));
            FTK_ASSERT(observed);
        }

        void FilesModelTest::_tileCompare()
        {
            auto settings = createTestSettings(_context);
            auto model = models::FilesModel::create(settings);
            model->add(makeItem("file0.exr"));
            model->add(makeItem("file1.exr"));
            model->add(makeItem("file2.exr"));
            model->add(makeItem("file3.exr"));
            model->setA(0);

            // Tile mode is the only mode that keeps several "B" files, so a
            // review can hold more than two sources there.
            tl::CompareOptions options;
            options.compare = tl::Compare::Tile;
            model->setCompareOptions(options);
            model->clearB();
            model->setB(1, true);
            model->setB(2, true);
            model->setB(3, true);
            FTK_ASSERT(std::vector<int>({ 1, 2, 3 }) == model->getBIndexes());

            // "A" stays the master source: it is the first active file, ahead of
            // every "B", whatever the mode.
            FTK_ASSERT(4 == model->getActive().size());
            FTK_ASSERT(model->getA() == model->getActive()[0]);

            // Switching to a single-buffer mode truncates "B" down to one file;
            // "A" is never dropped.
            options.compare = tl::Compare::Horizontal;
            model->setCompareOptions(options);
            FTK_ASSERT(1 == model->getBIndexes().size());
            FTK_ASSERT(0 == model->getAIndex());
            FTK_ASSERT(model->getA() == model->getActive()[0]);
        }

        void FilesModelTest::_reviewRestore()
        {
            // Replay the order used when a review is opened (see
            // App::_applyReview):
            // add every file, set the compare options, clear "B", rebuild "B",
            // then set "A" last. This must hold for every compare mode.
            const std::vector<tl::Compare> modes =
            {
                tl::Compare::A,
                tl::Compare::B,
                tl::Compare::Wipe,
                tl::Compare::Overlay,
                tl::Compare::Difference,
                tl::Compare::Horizontal,
                tl::Compare::Vertical,
                tl::Compare::Tile
            };
            for (const auto mode : modes)
            {
                auto settings = createTestSettings(_context);
                auto model = models::FilesModel::create(settings);
                for (int i = 0; i < 4; ++i)
                {
                    model->add(makeItem("file" + std::to_string(i) + ".exr"));
                }

                // A review saved with "A" = file0 and "B" = the other three.
                tl::CompareOptions options;
                options.compare = mode;
                model->setCompareOptions(options);
                model->clearB();
                model->setB(1, true);
                model->setB(2, true);
                model->setB(3, true);
                model->setA(0);

                // "A" is restored as saved in every mode, and remains the first
                // active file -- the master the player and the timeline follow.
                FTK_ASSERT(0 == model->getAIndex());
                FTK_ASSERT(!model->getActive().empty());
                FTK_ASSERT(model->getA() == model->getActive()[0]);

                // Only tile mode keeps all three "B" files; the others collapse
                // to the last one selected.
                const size_t expectedB = tl::Compare::Tile == mode ? 3 : 1;
                FTK_ASSERT(expectedB == model->getBIndexes().size());

                // FTK_ASSERT is compiled out in Release builds, so report the
                // observed state explicitly to keep this meaningful there too.
                const bool aOk =
                    0 == model->getAIndex() &&
                    !model->getActive().empty() &&
                    model->getA() == model->getActive()[0];
                const bool bOk = expectedB == model->getBIndexes().size();
                _print(ftk::Format("  {0}: A index {1}, B count {2} (expected {3}) -> {4}").
                    arg(tl::getLabel(mode)).
                    arg(model->getAIndex()).
                    arg(model->getBIndexes().size()).
                    arg(expectedB).
                    arg(aOk && bOk ? "ok" : "FAILED"));
                if (!aOk || !bOk)
                {
                    _error(ftk::Format("Review restore failed for compare mode {0}").
                        arg(tl::getLabel(mode)));
                }
            }
        }
        void FilesModelTest::_frames()
        {
            auto settings = createTestSettings(_context);
            auto model = models::FilesModel::create(settings);

            auto item = makeItem("/tmp/render.0050.exr");
            item->path.setFrames(ftk::RangeI64(50, 52));
            item->currentTime = OTIO_NS::RationalTime(51.0, 24.0);
            item->inOutRange = OTIO_NS::TimeRange(
                OTIO_NS::RationalTime(50.0, 24.0),
                OTIO_NS::RationalTime(3.0, 24.0));
            model->add(item);

            std::shared_ptr<models::FilesModelItem> reloaded;
            auto observer = ftk::Observer<std::shared_ptr<models::FilesModelItem> >::create(
                model->observeReload(),
                [&reloaded](const std::shared_ptr<models::FilesModelItem>& value)
                {
                    reloaded = value;
                },
                ftk::ObserverAction::Suppress);

            // Stating a wider range reopens the file, and drops the in/out
            // range, which was in the old range's terms.
            model->setFrames(item, ftk::RangeI64(1, 100));
            FTK_ASSERT(reloaded == item);
            FTK_ASSERT(item->path.getFrames().has_value());
            FTK_ASSERT(ftk::RangeI64(1, 100) == item->path.getFrames().value());
            FTK_ASSERT(!item->inOutRange.has_value());

            // The same range again is not a reopen.
            reloaded.reset();
            model->setFrames(item, ftk::RangeI64(1, 100));
            FTK_ASSERT(!reloaded);

            // A range stated against what the file was opened as, not
            // against the frame parsed out of its name. Opening one file of a
            // sequence leaves the path knowing only that frame, so narrowing
            // the range down to it is still a change.
            {
                auto opened = makeItem("/tmp/shot.0001.exr");
                FTK_ASSERT(ftk::RangeI64(1, 1) == opened->path.getFrames().value());
                opened->timeRange = OTIO_NS::TimeRange(
                    OTIO_NS::RationalTime(1.0, 24.0),
                    OTIO_NS::RationalTime(5.0, 24.0));
                model->add(opened);

                reloaded.reset();
                model->setFrames(opened, ftk::RangeI64(1, 1));
                FTK_ASSERT(reloaded == opened);
                FTK_ASSERT(ftk::RangeI64(1, 1) == opened->path.getFrames().value());

                // And the range it was opened as is not a change.
                opened->timeRange = OTIO_NS::TimeRange(
                    OTIO_NS::RationalTime(1.0, 24.0),
                    OTIO_NS::RationalTime(1.0, 24.0));
                reloaded.reset();
                model->setFrames(opened, ftk::RangeI64(1, 1));
                FTK_ASSERT(!reloaded);
            }

            // A file the model does not hold is ignored. Parsing the number
            // out of its name already gave it a range of that one frame, so
            // what is checked is that the range did not move.
            auto other = makeItem("/tmp/other.0001.exr");
            FTK_ASSERT(ftk::RangeI64(1, 1) == other->path.getFrames().value());
            model->setFrames(other, ftk::RangeI64(1, 10));
            FTK_ASSERT(!reloaded);
            FTK_ASSERT(ftk::RangeI64(1, 1) == other->path.getFrames().value());
        }

        void FilesModelTest::_persistence()
        {
            // FilesModel persists the compare options and compare time (not the
            // open files, which are session state). A temporary file lets the
            // state survive across model instances.
            const std::filesystem::path path =
                std::filesystem::temp_directory_path() / "djv-FilesModelTest.json";
            std::filesystem::remove(path);

            {
                auto settings = ftk::Settings::create(_context, path, true);
                auto model = models::FilesModel::create(settings);
                model->setCompareTime(tl::CompareTime::Absolute);
                tl::CompareOptions options;
                options.overlay = 0.75F;
                model->setCompareOptions(options);
            }

            {
                auto settings = ftk::Settings::create(_context, path, false);
                auto model = models::FilesModel::create(settings);
                FTK_ASSERT(tl::CompareTime::Absolute == model->getCompareTime());
                FTK_ASSERT(ftk::fuzzyCompare(0.75F, model->getCompareOptions().overlay));
            }

            std::filesystem::remove(path);
        }
    }
}
