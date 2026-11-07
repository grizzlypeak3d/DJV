// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/StatusTest.h>

#include <djv/Models/Status.h>

#include <ftk/Core/Assert.h>

namespace djv
{
    namespace models_tests
    {
        StatusTest::StatusTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::StatusTest")
        {}

        std::shared_ptr<StatusTest> StatusTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<StatusTest>(new StatusTest(context));
        }

        namespace
        {
            //! The ids that are on.
            std::vector<std::string> on(const models::StatusOptions& options)
            {
                std::vector<std::string> out;
                for (const auto& i : models::getStatus(options))
                {
                    if (i.second)
                    {
                        out.push_back(i.first);
                    }
                }
                return out;
            }
        }

        void StatusTest::run()
        {
            // Every item has a value, and nothing is on by default.
            const auto items = models::getStatusItems();
            FTK_CHECK(10 == items.size());
            const auto values = models::getStatus(models::StatusOptions());
            for (const auto& item : items)
            {
                FTK_CHECK(values.count(item.id));
                FTK_CHECK(!item.label.empty());
            }
            FTK_CHECK(on(models::StatusOptions()).empty());

            // Each option, on alone.
            const auto only = [](const models::StatusOptions& options, const std::string& id)
            {
                const auto ids = on(options);
                return 1 == ids.size() && id == ids.front();
            };
            {
                models::StatusOptions options;
                options.display.channels = ftk::ChannelDisplay::Red;
                FTK_CHECK(only(options, "Channels"));
            }
            {
                models::StatusOptions options;
                options.display.negative = true;
                FTK_CHECK(only(options, "Negative"));
            }
            {
                models::StatusOptions options;
                options.foreground.clippingWarning.enabled = true;
                FTK_CHECK(only(options, "ClippingWarning"));
            }
            {
                models::StatusOptions options;
                options.display.mirror.y = true;
                FTK_CHECK(only(options, "Mirror"));
            }
            {
                models::StatusOptions options;
                options.aspectRatio.index = 1;
                FTK_CHECK(only(options, "AspectRatio"));
            }
            {
                // OpenColorIO with a display and a view, as the renderer
                // has it; on alone it transforms nothing.
                models::StatusOptions options;
                options.ocio.enabled = true;
                FTK_CHECK(on(options).empty());
                options.ocio.display = "Display";
                options.ocio.view = "View";
                FTK_CHECK(only(options, "OCIO"));
            }
            {
                models::StatusOptions options;
                options.lut.enabled = true;
                FTK_CHECK(on(options).empty());
                options.lut.fileName = "look.cube";
                FTK_CHECK(only(options, "LUT"));
            }
            {
                // An adjustment at its defaults changes nothing.
                models::StatusOptions options;
                options.display.color.enabled = true;
                options.display.levels.enabled = true;
                options.display.exposure.enabled = true;
                FTK_CHECK(on(options).empty());
                options.display.exposure.exposure = 1.F;
                FTK_CHECK(only(options, "Color"));
            }
            {
                models::StatusOptions options;
                options.audioOffset = .5;
                FTK_CHECK(only(options, "AudioOffset"));
            }
            {
                models::StatusOptions options;
                options.audioChannel = 0;
                FTK_CHECK(only(options, "AudioChannel"));
            }
        }
    }
}
