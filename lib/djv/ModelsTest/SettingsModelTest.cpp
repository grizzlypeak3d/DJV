// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/SettingsModelTest.h>

#include <djv/Models/ExportRender.h>
#include <djv/Models/SettingsModel.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Path.h>

#include <map>
#include <set>

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
            _stillFileName();
            _fileNameRules();
            _shortcuts();
            _defaultShortcuts();
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

        void SettingsModelTest::_stillFileName()
        {
            // The file's name and the time, what a file name cannot hold made
            // a dash: a timecode's colons, and a drop frame one's semicolon.
            FTK_CHECK("Train-Shaded_01-00-00-03.tif" == models::getStillFileName(
                ftk::Path("/media/Train-Shaded.otioz"), "01:00:00:03", ".tif"));
            FTK_CHECK("Train-Shaded_00-59-59-29.png" == models::getStillFileName(
                ftk::Path("Train-Shaded.mov"), "00:59:59;29", ".png"));
            // A sequence's name without its frame number or the separator
            // before it.
            {
                ftk::Path path("/shots/render.1001.exr");
                path.setFrames(ftk::RangeI64(1001, 1100));
                FTK_CHECK("render_1001.jpg" == models::getStillFileName(path, "1001", ".jpg"));
            }
            {
                ftk::Path path("shot_v002_0001.dpx");
                path.setFrames(ftk::RangeI64(1, 48));
                FTK_CHECK("shot_v002_24.dpx" == models::getStillFileName(path, "24", ".dpx"));
            }
            // A movie or a single image keeps the digits its name ends in.
            FTK_CHECK("IMG_1589_00-00-00-00.tif" == models::getStillFileName(
                ftk::Path("/Users/me/Desktop/IMG_1589.MOV"), "00:00:00:00", ".tif"));
            FTK_CHECK("shot_0001_1.png" == models::getStillFileName(
                ftk::Path("shot_0001.png"), "1", ".png"));
            // The time in the timeline's units, in a form that sorts and is
            // not taken for a frame number: frames padded to the last
            // frame's digits, seconds the same with an "s", timecode as
            // labeled.
            const OTIO_NS::TimeRange range(
                OTIO_NS::RationalTime(0.0, 24.0),
                OTIO_NS::RationalTime(2400.0, 24.0));
            const OTIO_NS::RationalTime time(42.0, 24.0);
            FTK_CHECK("0042" == models::getStillTimeLabel(
                time, range, tl::TimeUnits::Frames, "00:00:01:18"));
            FTK_CHECK("01.75s" == models::getStillTimeLabel(
                time, range, tl::TimeUnits::Seconds, "00:00:01:18"));
            FTK_CHECK("00:00:01:18" == models::getStillTimeLabel(
                time, range, tl::TimeUnits::Timecode, "00:00:01:18"));
            FTK_CHECK("IMG_1589_01.75s.tif" == models::getStillFileName(
                ftk::Path("IMG_1589.MOV"),
                models::getStillTimeLabel(time, range, tl::TimeUnits::Seconds, ""),
                ".tif"));
            // A short range pads to its own digits.
            FTK_CHECK("7" == models::getStillTimeLabel(
                OTIO_NS::RationalTime(7.0, 24.0),
                OTIO_NS::TimeRange(OTIO_NS::RationalTime(0.0, 24.0), OTIO_NS::RationalTime(10.0, 24.0)),
                tl::TimeUnits::Frames,
                ""));

            // Without a time, the name alone.
            FTK_CHECK("render.tif" == models::getStillFileName(
                ftk::Path("render.mov"), "", ".tif"));
        }

        void SettingsModelTest::_defaultShortcuts()
        {
            // The default keys the desktop and the iPad share: one entry a
            // name, every one with a key, and no key given to two actions,
            // which would leave one of them unreachable.
            const auto& defaults = models::getDefaultShortcuts();
            FTK_CHECK(!defaults.empty());
            std::set<std::string> names;
            std::map<std::pair<ftk::Key, int>, std::string> keys;
            for (const auto& shortcut : defaults)
            {
                FTK_CHECK(names.insert(shortcut.name).second);
                FTK_CHECK(shortcut.primary.key != ftk::Key::Unknown);
                for (const auto& key : { shortcut.primary, shortcut.secondary })
                {
                    if (key.key == ftk::Key::Unknown)
                        continue;
                    const auto inserted = keys.insert(
                        { { key.key, key.modifiers }, shortcut.name });
                    if (!inserted.second)
                    {
                        _print(ftk::Format("{0} and {1} have the same key").
                            arg(inserted.first->second).
                            arg(shortcut.name));
                    }
                    FTK_CHECK(inserted.second);
                }
            }
            {
                const auto shortcut = models::getDefaultShortcut("Playback/Toggle");
                FTK_CHECK("Playback/Toggle" == shortcut.name);
                FTK_CHECK(ftk::Key::Space == shortcut.primary.key);
                FTK_CHECK(0 == shortcut.primary.modifiers);
            }
            {
                // An action with no default keys has none.
                const auto shortcut = models::getDefaultShortcut("Review/Draw");
                FTK_CHECK("Review/Draw" == shortcut.name);
                FTK_CHECK(ftk::Key::Unknown == shortcut.primary.key);
            }
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
