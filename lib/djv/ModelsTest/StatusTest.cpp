// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/StatusTest.h>

#include <djv/Models/Status.h>

#include <ftk/Core/Assert.h>

#include <algorithm>

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
            FTK_CHECK(12 == items.size());
            const auto values = models::getStatus(models::StatusOptions());
            for (const auto& item : items)
            {
                FTK_CHECK(values.count(item.id));
                FTK_CHECK(!item.label.empty());
            }
            FTK_CHECK(on(models::StatusOptions()).empty());

            // Every item is in a group, a group's items are together, and
            // each group has one letter of its own.
            const auto groups = models::getStatusGroups();
            FTK_CHECK(5 == groups.size());
            {
                std::vector<std::string> seen;
                for (const auto& item : items)
                {
                    const auto i = std::find_if(
                        groups.begin(),
                        groups.end(),
                        [&item](const models::StatusGroup& group)
                        {
                            return group.id == item.group;
                        });
                    FTK_CHECK(i != groups.end());
                    if (seen.empty() || seen.back() != item.group)
                    {
                        FTK_CHECK(std::find(seen.begin(), seen.end(), item.group) == seen.end());
                        seen.push_back(item.group);
                    }
                }
                FTK_CHECK(seen.size() == groups.size());
                std::vector<std::string> letters;
                for (size_t i = 0; i < groups.size(); ++i)
                {
                    FTK_CHECK(groups[i].id == seen[i]);
                    FTK_CHECK(1 == groups[i].letter.size());
                    FTK_CHECK(!groups[i].label.empty());
                    FTK_CHECK(std::find(letters.begin(), letters.end(), groups[i].letter) == letters.end());
                    letters.push_back(groups[i].letter);
                }
            }

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
                // A stored frame is on while it is stored, drawn or not:
                // with no compare mode it waits for the next one (#974).
                models::StatusOptions options;
                options.storedFrame = true;
                FTK_CHECK(only(options, "FrameStore"));
                options.compare = tl::Compare::Wipe;
                FTK_CHECK(only(options, "FrameStore"));
                options.storedFrame = false;
                FTK_CHECK(on(options).empty());
            }
            {
                // A comparison is on with B files to draw; a stored frame
                // stands in for them, and is what is on then.
                models::StatusOptions options;
                options.compare = tl::Compare::Wipe;
                FTK_CHECK(on(options).empty());
                options.bFiles = true;
                FTK_CHECK(only(options, "Compare"));
                options.storedFrame = true;
                FTK_CHECK(only(options, "FrameStore"));
                options.compare = tl::Compare::None;
                FTK_CHECK(only(options, "FrameStore"));
                options.storedFrame = false;
                FTK_CHECK(on(options).empty());
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
