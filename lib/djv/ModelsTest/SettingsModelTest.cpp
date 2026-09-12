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
            _fileNameRules();
            _shortcuts();
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

        void SettingsModelTest::_fileNameRules()
        {
            const std::vector<std::string> exts = { ".exr", ".jpg", ".tif" };

            // Only a run of '#' is the frame slot. Digits are literal, or
            // "shot_v002" would be written as "shot_v023".
            FTK_CHECK(models::isFrameTemplate(ftk::Path("shot.####.tif")));
            FTK_CHECK(models::isFrameTemplate(ftk::Path("shot.#.tif")));
            FTK_CHECK(!models::isFrameTemplate(ftk::Path("shot_v002.tif")));
            FTK_CHECK(!models::isFrameTemplate(ftk::Path("shot.tif")));

            // An extension typed onto the name is taken off it, whatever
            // case it was typed in.
            {
                std::string name = "shot.exr";
                std::string ext = ".tif";
                FTK_CHECK(models::splitExt(name, ext, exts));
                FTK_CHECK("shot" == name);
                FTK_CHECK(".exr" == ext);
            }
            {
                std::string name = "shot.EXR";
                std::string ext = ".tif";
                FTK_CHECK(models::splitExt(name, ext, exts));
                FTK_CHECK("shot" == name);
                FTK_CHECK(".exr" == ext);
            }
            {
                // Not one of them: left as it was typed.
                std::string name = "shot.xyz";
                std::string ext = ".tif";
                FTK_CHECK(!models::splitExt(name, ext, exts));
                FTK_CHECK("shot.xyz" == name);
                FTK_CHECK(".tif" == ext);
            }

            // What the Export button reports, and refuses to write on.
            FTK_CHECK(!models::getFileNameError(
                "", ".tif", models::ExportFileType::Image, exts).empty());
            FTK_CHECK(!models::getFileNameError(
                "a/shot", ".tif", models::ExportFileType::Image, exts).empty());
            FTK_CHECK(!models::getFileNameError(
                "shot", ".xyz", models::ExportFileType::Image, exts).empty());
            // A sequence needs the frame slot; a movie has no frame number.
            FTK_CHECK(!models::getFileNameError(
                "shot", ".tif", models::ExportFileType::Seq, exts).empty());
            FTK_CHECK(models::getFileNameError(
                "shot.####", ".tif", models::ExportFileType::Seq, exts).empty());
            FTK_CHECK(models::getFileNameError(
                "shot", ".tif", models::ExportFileType::Movie, exts).empty());
            FTK_CHECK(!models::getFileNameError(
                "shot.####", ".tif", models::ExportFileType::Movie, exts).empty());
            // A version number is a name, not a frame slot.
            FTK_CHECK(models::getFileNameError(
                "shot_v002", ".tif", models::ExportFileType::Image, exts).empty());
        }

        void SettingsModelTest::_shortcuts()
        {
            const std::vector<models::Shortcut> defaults =
            {
                models::Shortcut(
                    "Review/Undo",
                    "Undo drawing",
                    ftk::KeyShortcut(ftk::Key::Z, static_cast<int>(ftk::KeyModifier::Control))),
                models::Shortcut("Review/Draw", "Draw strokes")
            };
            {
                // Shortcuts left at their defaults are not saved, so a default
                // changed later reaches everyone.
                FTK_CHECK(models::getChangedShortcuts(defaults, defaults).empty());
            }
            {
                auto value = defaults;
                value[1].primary = ftk::KeyShortcut(ftk::Key::D);
                const auto changed = models::getChangedShortcuts(value, defaults);
                FTK_CHECK(1 == changed.size());
                FTK_CHECK("Review/Draw" == changed[0].name);
            }
            {
                // A default cleared on purpose is a change, and is kept.
                auto value = defaults;
                value[0].primary = ftk::KeyShortcut();
                const auto changed = models::getChangedShortcuts(value, defaults);
                FTK_CHECK(1 == changed.size());
                FTK_CHECK("Review/Undo" == changed[0].name);
            }
            {
                // A shortcut with no default counts as changed.
                const auto changed = models::getChangedShortcuts(
                    { models::Shortcut("Other", "Other") },
                    defaults);
                FTK_CHECK(1 == changed.size());
            }
            {
                // Settings from before saved every shortcut: one saved without
                // a key is dropped, so its default applies.
                const std::vector<models::Shortcut> saved =
                {
                    models::Shortcut("Review/Undo", "Undo drawing"),
                    models::Shortcut("Review/Draw", "Draw strokes", ftk::KeyShortcut(ftk::Key::D)),
                    models::Shortcut("Other", "Other", ftk::KeyShortcut(), ftk::KeyShortcut(ftk::Key::X))
                };
                const auto bound = models::getBoundShortcuts(saved);
                FTK_CHECK(2 == bound.size());
                FTK_CHECK("Review/Draw" == bound[0].name);
                FTK_CHECK("Other" == bound[1].name);
            }
        }
    }
}
