// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/ToolsModelTest.h>

#include <djv/ModelsTest/ModelsTestUtil.h>

#include <djv/Models/ToolsModel.h>

#include <ftk/UI/Settings.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Observable.h>

#include <filesystem>

namespace djv
{
    namespace models_tests
    {
        ToolsModelTest::ToolsModelTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::ToolsModelTest")
        {}

        std::shared_ptr<ToolsModelTest> ToolsModelTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<ToolsModelTest>(new ToolsModelTest(context));
        }

        void ToolsModelTest::run()
        {
            _behavior();
            _persistence();
        }

        void ToolsModelTest::_behavior()
        {
            // In-memory settings keep this test isolated (no file I/O).
            auto settings = createTestSettings(_context);
            auto model = models::ToolsModel::create(settings);

            // Observe the active tool. With ObserverAction::Trigger (the default)
            // the callback fires immediately with the current value.
            std::string tool = "Files";
            auto observer = ftk::Observer<std::string>::create(
                model->observeActiveTool(),
                [&tool](const std::string& value) { tool = value; });

            // The default (with empty settings) is None.
            FTK_ASSERT(model->getActiveTool().empty());
            FTK_ASSERT(tool.empty());

            // Set the active tool; the observer should see each change.
            model->setActiveTool("Files");
            FTK_ASSERT("Files" == model->getActiveTool());
            FTK_ASSERT("Files" == tool);

            model->setActiveTool("Color");
            FTK_ASSERT("Color" == model->getActiveTool());
            FTK_ASSERT("Color" == tool);
        }

        void ToolsModelTest::_persistence()
        {
            // A real (temporary) file is needed so the state survives across
            // model instances.
            const std::filesystem::path path =
                std::filesystem::temp_directory_path() / "djv-ToolsModelTest.json";
            std::filesystem::remove(path);

            // Change to a non-default value; the model writes it into the settings
            // in its destructor, which then save to disk. Files (not the None
            // default) proves the value is actually restored, not defaulted.
            {
                auto settings = ftk::Settings::create(_context, path, true);
                auto model = models::ToolsModel::create(settings);
                model->setActiveTool("Files");
            }

            // Recreate from the same file (reset=false loads it) and verify.
            {
                auto settings = ftk::Settings::create(_context, path, false);
                auto model = models::ToolsModel::create(settings);
                FTK_ASSERT("Files" == model->getActiveTool());
            }

            std::filesystem::remove(path);
        }
    }
}
