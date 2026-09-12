// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/SettingsModelTest.h>

#include <djv/Models/SettingsModel.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Path.h>

namespace djv
{
    namespace models_tests
    {
        SettingsModelTest::SettingsModelTest(
            const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::SettingsModelTest")
        {}

        std::shared_ptr<SettingsModelTest> SettingsModelTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<SettingsModelTest>(
                new SettingsModelTest(context));
        }

        void SettingsModelTest::run()
        {
            _exportNames();
        }

        void SettingsModelTest::_exportNames()
        {
            const std::vector<std::string> seqExts =
            {
                ".exr",
                ".jpg",
                ".png",
                ".tif"
            };
            struct Data
            {
                std::string path;
                std::string name;
                std::string seqName;
            };
            const std::vector<Data> data =
            {
                // A movie whose name ends in digits keeps them: only an
                // image sequence numbers its frames.
                { "Wide_169.mov", "Wide_169", "Wide_169.####" },
                { "shot_v003.mov", "shot_v003", "shot_v003.####" },
                // A sequence keeps the shape it had, separator and all.
                { "BART.0001.jpg", "BART", "BART.####" },
                { "render0001.exr", "render", "render####" },
                { "render-0001.exr", "render", "render-####" },
                // Unpadded: as many '#' as the frame has digits.
                { "render.1.exr", "render", "render.#" },
                // The sign belongs to the number rather than to the name.
                { "render-1.exr", "render", "render#" },
                // Nothing to number.
                { "file.txt", "file", "file.####" },
                // Digits, but not a sequence: part of the name.
                { "frame.0001.mov", "frame.0001", "frame.0001.####" }
            };
            for (const auto& i : data)
            {
                std::string name;
                std::string seqName;
                models::getExportNames(
                    ftk::Path(i.path), seqExts, name, seqName);
                FTK_CHECK(name == i.name);
                FTK_CHECK(seqName == i.seqName);
            }
        }
    }
}
