// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/ViewportModelTest.h>

#include <djv/ModelsTest/ModelsTestUtil.h>

#include <djv/Models/ViewportModel.h>

#include <ftk/GL/Texture.h>

#include <ftk/UI/Settings.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Observable.h>

#include <filesystem>

namespace djv
{
    namespace models_tests
    {
        ViewportModelTest::ViewportModelTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::ViewportModelTest")
        {}

        std::shared_ptr<ViewportModelTest> ViewportModelTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<ViewportModelTest>(new ViewportModelTest(context));
        }

        void ViewportModelTest::run()
        {
            _behavior();
            _persistence();
        }

        void ViewportModelTest::_behavior()
        {
            // In-memory settings keep this test isolated (no file I/O).
            auto settings = createTestSettings(_context);
            auto model = models::ViewportModel::create(_context, settings);

            // Observe the color buffer type. With ObserverAction::Trigger (the
            // default) the callback fires immediately with the current value.
            ftk::gl::TextureType colorBuffer = ftk::gl::TextureType::None;
            auto observer = ftk::Observer<ftk::gl::TextureType>::create(
                model->observeColorBuffer(),
                [&colorBuffer](const ftk::gl::TextureType& value) { colorBuffer = value; });

            // The observer should see the current (default) value. The default is
            // platform-dependent, so compare against the model rather than a
            // literal.
            FTK_ASSERT(model->getColorBuffer() == colorBuffer);

            // Set the color buffer; the observer should see the change.
            model->setColorBuffer(ftk::gl::TextureType::RGBA_U16);
            FTK_ASSERT(ftk::gl::TextureType::RGBA_U16 == model->getColorBuffer());
            FTK_ASSERT(ftk::gl::TextureType::RGBA_U16 == colorBuffer);
        }

        void ViewportModelTest::_persistence()
        {
            // A real (temporary) file is needed so the state survives across
            // model instances.
            const std::filesystem::path path =
                std::filesystem::temp_directory_path() / "djv-ViewportModelTest.json";
            std::filesystem::remove(path);

            // Change to a value that differs from either platform default, so a
            // successful reload proves the value was loaded rather than defaulted.
            {
                auto settings = ftk::Settings::create(_context, path, true);
                auto model = models::ViewportModel::create(_context, settings);
                model->setColorBuffer(ftk::gl::TextureType::RGBA_U16);
            }

            // Recreate from the same file (reset=false loads it) and verify.
            {
                auto settings = ftk::Settings::create(_context, path, false);
                auto model = models::ViewportModel::create(_context, settings);
                FTK_ASSERT(ftk::gl::TextureType::RGBA_U16 == model->getColorBuffer());
            }

            std::filesystem::remove(path);
        }
    }
}
