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

            // Observe the open tools. With ObserverAction::Trigger (the
            // default) the callback fires immediately with the current value.
            std::vector<std::string> tools = { "Files" };
            auto observer = ftk::ListObserver<std::string>::create(
                model->observeOpenTools(),
                [&tools](const std::vector<std::string>& value) { tools = value; });

            // The default (with empty settings) is nothing open.
            FTK_CHECK(model->getOpenTools().empty());
            FTK_CHECK(tools.empty());

            // Open tools; the observer should see each change.
            model->setToolOpen("Files", true);
            FTK_CHECK(model->isToolOpen("Files"));
            FTK_CHECK(std::vector<std::string>({ "Files" }) == tools);

            // More than one can be open, and they come back in the order the
            // tools are listed rather than the order they were opened.
            model->setToolOpen("Color", true);
            FTK_CHECK(model->isToolOpen("Files"));
            FTK_CHECK(model->isToolOpen("Color"));
            FTK_CHECK(std::vector<std::string>({ "Files", "Color" }) == tools);

            // Opening one that is already open changes nothing.
            model->setToolOpen("Color", true);
            FTK_CHECK(std::vector<std::string>({ "Files", "Color" }) == tools);

            // Closing takes out only the one named.
            model->setToolOpen("Files", false);
            FTK_CHECK(!model->isToolOpen("Files"));
            FTK_CHECK(std::vector<std::string>({ "Color" }) == tools);

            model->closeTools();
            FTK_CHECK(model->getOpenTools().empty());
            FTK_CHECK(tools.empty());
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
                model->setToolOpen("Files", true);
                model->setToolOpen("Color Picker", true);
            }

            // Recreate from the same file (reset=false loads it) and verify.
            {
                auto settings = ftk::Settings::create(_context, path, false);
                auto model = models::ToolsModel::create(settings);
                FTK_CHECK(model->isToolOpen("Files"));
                FTK_CHECK(model->isToolOpen("Color Picker"));
            }

            std::filesystem::remove(path);
        }
    }
}
