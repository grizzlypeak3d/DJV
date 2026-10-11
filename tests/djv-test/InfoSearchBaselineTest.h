// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/InfoSearch.h>

#include <ftk/Core/Assert.h>
#include <ftk/TestLib/ITest.h>

#include <memory>

namespace djv
{
    namespace tests
    {
        //! Baseline for the existing single-query Information row filter.
        //! Cross-row implied AND and field aliases in #316 are not decided.
        class InfoSearchBaselineTest : public ftk::test::ITest
        {
        protected:
            explicit InfoSearchBaselineTest(const std::shared_ptr<ftk::Context>& context) :
                ITest(context, "tests::InfoSearchBaselineTest")
            {}

        public:
            static std::shared_ptr<InfoSearchBaselineTest> create(
                const std::shared_ptr<ftk::Context>& context)
            {
                return std::shared_ptr<InfoSearchBaselineTest>(
                    new InfoSearchBaselineTest(context));
            }

            void run() override
            {
                using ui::detail::matchesInfoRow;

                // Empty search retains all rows; no implicit trimming.
                FTK_CHECK(matchesInfoRow("Name", "clip.mov", ""));
                FTK_CHECK(!matchesInfoRow("Name", "clip.mov", " "));

                // Existing single-term, case-insensitive key/value matching.
                FTK_CHECK(matchesInfoRow("Name", "clip.mov", "nAmE"));
                FTK_CHECK(matchesInfoRow("Name", "clip.mov", "CLIP"));
                FTK_CHECK(!matchesInfoRow("Name", "clip.mov", "duration"));
                FTK_CHECK(matchesInfoRow("Resolution", "1920 1080", "1920"));
                FTK_CHECK(matchesInfoRow("Pixel Aspect Ratio", "1.00", "aspect ratio"));
                FTK_CHECK(!matchesInfoRow("Codec", "ProRes", "resolution"));

                // Both Video and Audio can independently contain Duration.
                FTK_CHECK(matchesInfoRow("Duration", "00:00:10:00", "duration"));
                FTK_CHECK(matchesInfoRow("Duration", "10.00 seconds", "DURATION"));

                // Metadata may have arbitrary names and multiline values.
                FTK_CHECK(matchesInfoRow("Camera Model", "Example", "camera"));
                FTK_CHECK(matchesInfoRow("Description", "first\nsecond", "second"));
                FTK_CHECK(!matchesInfoRow("Description", "first\nsecond", "third"));
            }
        };
    }
}
