// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/RecentFilesModelTest.h>

#include <djv/ModelsTest/ModelsTestUtil.h>

#include <djv/Models/RecentFilesModel.h>

#include <ftk/UI/Settings.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Observable.h>

#include <filesystem>
#include <vector>

namespace djv
{
    namespace models_tests
    {
        RecentFilesModelTest::RecentFilesModelTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::RecentFilesModelTest")
        {}

        std::shared_ptr<RecentFilesModelTest> RecentFilesModelTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<RecentFilesModelTest>(new RecentFilesModelTest(context));
        }

        void RecentFilesModelTest::run()
        {
            _behavior();
            _persistence();
        }

        void RecentFilesModelTest::_behavior()
        {
            typedef std::vector<std::filesystem::path> Paths;

            // In-memory settings keep this test isolated (no file I/O).
            auto settings = createTestSettings(_context);
            auto model = models::RecentFilesModel::create(_context, settings);

            // Observe the recent list.
            Paths recent;
            auto observer = ftk::ListObserver<std::filesystem::path>::create(
                model->observeRecent(),
                [&recent](const Paths& value) { recent = value; });

            // addRecent() makes paths absolute, so use absolute paths (which need
            // not exist) to keep the comparisons independent of the working dir.
            const std::filesystem::path dir = std::filesystem::temp_directory_path();
            const std::filesystem::path a = std::filesystem::absolute(dir / "djv-recent-a.txt");
            const std::filesystem::path b = std::filesystem::absolute(dir / "djv-recent-b.txt");
            const std::filesystem::path c = std::filesystem::absolute(dir / "djv-recent-c.txt");

            // Empty by default; the default maximum is ten.
            FTK_ASSERT(model->getRecent().empty());
            FTK_ASSERT(10 == model->getRecentMax());

            // Adding files appends to the back (newest last); the observer fires.
            model->addRecent(a);
            model->addRecent(b);
            FTK_ASSERT(Paths({ a, b }) == model->getRecent());
            FTK_ASSERT(Paths({ a, b }) == recent);

            // Re-adding a file de-duplicates it and moves it to the back.
            model->addRecent(a);
            FTK_ASSERT(Paths({ b, a }) == model->getRecent());

            // Lowering the maximum trims the oldest (front) entries immediately.
            model->addRecent(c);            // { b, a, c }
            model->setRecentMax(2);         // trims to { a, c }
            FTK_ASSERT(2 == model->getRecentMax());
            FTK_ASSERT(Paths({ a, c }) == model->getRecent());

            // Adding past the maximum drops the oldest (front) entry.
            model->addRecent(b);            // { a, c, b } -> { c, b }
            FTK_ASSERT(Paths({ c, b }) == model->getRecent());
        }

        void RecentFilesModelTest::_persistence()
        {
            typedef std::vector<std::filesystem::path> Paths;

            // A real (temporary) file is needed so the state survives across model
            // instances.
            const std::filesystem::path path =
                std::filesystem::temp_directory_path() / "djv-RecentFilesModelTest.json";
            std::filesystem::remove(path);

            const std::filesystem::path dir = std::filesystem::temp_directory_path();
            const std::filesystem::path a = std::filesystem::absolute(dir / "djv-recent-a.txt");
            const std::filesystem::path b = std::filesystem::absolute(dir / "djv-recent-b.txt");

            // Populate the list and maximum; the model writes them into the
            // settings in its destructor, which then save to disk.
            {
                auto settings = ftk::Settings::create(_context, path, true);
                auto model = models::RecentFilesModel::create(_context, settings);
                model->addRecent(a);
                model->addRecent(b);
                model->setRecentMax(5);
            }

            // Recreate from the same file (reset=false loads it) and verify the
            // list and maximum were restored.
            {
                auto settings = ftk::Settings::create(_context, path, false);
                auto model = models::RecentFilesModel::create(_context, settings);
                FTK_ASSERT(Paths({ a, b }) == model->getRecent());
                FTK_ASSERT(5 == model->getRecentMax());
            }

            std::filesystem::remove(path);
        }
    }
}
