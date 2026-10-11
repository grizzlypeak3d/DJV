// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/InfoSearch.h>

#include <ftk/Core/Assert.h>
#include <ftk/TestLib/ITest.h>

#include <memory>
#include <string>
#include <vector>

namespace djv
{
    namespace tests
    {
        class InfoSearchAndTest : public ftk::test::ITest
        {
        protected:
            explicit InfoSearchAndTest(const std::shared_ptr<ftk::Context>& context) :
                ITest(context, "tests::InfoSearchAndTest")
            {}

        public:
            static std::shared_ptr<InfoSearchAndTest> create(
                const std::shared_ptr<ftk::Context>& context)
            {
                return std::shared_ptr<InfoSearchAndTest>(
                    new InfoSearchAndTest(context));
            }

            void run() override
            {
                using ui::detail::InfoSections;
                using ui::detail::filterInfoSections;

                const std::vector<std::string> order = {
                    "File", "Video", "Audio", "Metadata" };
                const InfoSections sections = {
                    { "File", {
                        { "Name", "clip.mov" },
                        { "Directory", "C:/shots" } } },
                    { "Video", {
                        { "Resolution", "1920 1080" },
                        { "Duration", "00:00:10:00" },
                        { "Pixel Aspect Ratio", "1.00" },
                        { "Pixel Type", "YUV 420P U8" } } },
                    { "Audio", {
                        { "Duration", "10.00 seconds" },
                        { "Channels", "2" } } },
                    { "Metadata", {
                        { "Camera Model", "Example" },
                        { "Description", "first\nsecond" } } }
                };

                const auto empty = filterInfoSections(sections, order, "");
                FTK_CHECK(empty.at("File").size() == 2);
                FTK_CHECK(empty.at("Video").size() == 4);
                FTK_CHECK(empty.at("Audio").size() == 2);
                FTK_CHECK(empty.at("Metadata").size() == 2);

                const auto whitespace = filterInfoSections(sections, order, "  \t\n ");
                FTK_CHECK(whitespace == empty);

                const auto single = filterInfoSections(sections, order, "cLiP");
                FTK_CHECK(single.at("File").size() == 1);
                FTK_CHECK(single.at("Video").empty());
                FTK_CHECK(single.at("Audio").empty());

                const auto crossRows = filterInfoSections(
                    sections, order, " nAmE \t DURATION   resolution ");
                FTK_CHECK(crossRows.at("File").size() == 1);
                FTK_CHECK(crossRows.at("Video").size() == 2);
                FTK_CHECK(crossRows.at("Audio").size() == 1);
                FTK_CHECK(crossRows.at("Metadata").empty());

                // The words in the original feature request map onto the
                // Information panel's newer Name and Resolution labels.
                const auto aliases = filterInfoSections(
                    sections, order, "filename duration dimensions");
                FTK_CHECK(aliases.at("File").size() == 1);
                FTK_CHECK(aliases.at("Video").size() == 2);
                FTK_CHECK(aliases.at("Audio").size() == 1);

                const auto duplicate = filterInfoSections(
                    sections, order, "DURATION duration");
                FTK_CHECK(duplicate.at("Video").size() == 1);
                FTK_CHECK(duplicate.at("Audio").size() == 1);

                const auto missing = filterInfoSections(
                    sections, order, "name nonexistent");
                for (const auto& name : order)
                {
                    FTK_CHECK(missing.at(name).empty());
                }

                const auto multiword = filterInfoSections(
                    sections, order, "pixel aspect ratio");
                FTK_CHECK(multiword.at("Video").size() == 1);
                if (!multiword.at("Video").empty())
                {
                    FTK_CHECK(multiword.at("Video").front().first ==
                        "Pixel Aspect Ratio");
                }

                // #316: a multiword field label combined with another term
                // must not independently match "ratio" in "Duration", nor
                // include unrelated fields such as "Pixel Type".
                const auto phraseAnd = filterInfoSections(
                    sections, order, "pixel aspect ratio channels");
                FTK_CHECK(phraseAnd.at("File").empty());
                FTK_CHECK(phraseAnd.at("Video").size() == 1);
                if (!phraseAnd.at("Video").empty())
                {
                    FTK_CHECK(phraseAnd.at("Video").front().first ==
                        "Pixel Aspect Ratio");
                }
                FTK_CHECK(phraseAnd.at("Audio").size() == 1);
                if (!phraseAnd.at("Audio").empty())
                {
                    FTK_CHECK(phraseAnd.at("Audio").front().first == "Channels");
                }
                FTK_CHECK(phraseAnd.at("Metadata").empty());

                const auto reversedPhraseAnd = filterInfoSections(
                    sections, order, "CHANNELS  PIXEL   ASPECT  RATIO");
                FTK_CHECK(reversedPhraseAnd == phraseAnd);

                const auto threePartQuery = filterInfoSections(
                    sections, order, "name pixel aspect ratio channels");
                FTK_CHECK(threePartQuery.at("File").size() == 1);
                FTK_CHECK(threePartQuery.at("Video").size() == 1);
                FTK_CHECK(threePartQuery.at("Audio").size() == 1);
                FTK_CHECK(threePartQuery.at("Metadata").empty());

                const auto phraseAndMissing = filterInfoSections(
                    sections, order, "pixel aspect ratio nonexistent123");
                for (const auto& name : order)
                {
                    FTK_CHECK(phraseAndMissing.at(name).empty());
                }

                const auto metadata = filterInfoSections(
                    sections, order, "camera SECOND");
                FTK_CHECK(metadata.at("Metadata").size() == 2);
                FTK_CHECK(metadata.at("File").empty());

                // A metadata key must not change how the search is parsed.
                // Both actual Name and Resolution rows should remain visible.
                InfoSections ambiguous = sections;
                ambiguous["Metadata"].push_back({
                    "Name Resolution Details", "extra" });
                const auto ambiguousResult = filterInfoSections(
                    ambiguous, order, "name resolution");
                FTK_CHECK(ambiguousResult.at("File").size() == 1);
                FTK_CHECK(ambiguousResult.at("Video").size() == 1);
                FTK_CHECK(ambiguousResult.at("Metadata").size() == 1);

                // An unusually named metadata key cannot absorb the two
                // intended search terms: Pixel Aspect Ratio and Channels.
                ambiguous["Metadata"].push_back({
                    "Pixel Aspect Ratio Channels", "extra" });
                const auto ambiguousPhrase = filterInfoSections(
                    ambiguous, order, "pixel aspect ratio channels");
                FTK_CHECK(ambiguousPhrase.at("Video").size() == 1);
                FTK_CHECK(ambiguousPhrase.at("Audio").size() == 1);

                // Regression: implied-AND word order should not change results.
                // "Source Format" and "Channels" exist, but "Source Channels"
                // does not exist in this fixture.
                InfoSections wordOrder = sections;
                wordOrder["Video"].push_back({ "Source Format", "YUV 420P" });

                const auto sourceFirst = filterInfoSections(
                    wordOrder, order, "source channels");
                const auto channelsFirst = filterInfoSections(
                    wordOrder, order, "channels source");

                FTK_CHECK(sourceFirst == channelsFirst);
                FTK_CHECK(sourceFirst.at("Video").size() == 1);
                FTK_CHECK(sourceFirst.at("Audio").size() == 1);

                // Word order must also be stable when the optional
                // Source Channels field is actually present.
                wordOrder["Audio"].push_back({ "Source Channels", "6" });
                const auto sourceFieldFirst = filterInfoSections(
                    wordOrder, order, "source channels");
                const auto channelsFieldFirst = filterInfoSections(
                    wordOrder, order, "channels source");
                FTK_CHECK(sourceFieldFirst == channelsFieldFirst);

                // A recognized multiword label must still match when its
                // query words are separated or appear in another order.
                const auto shuffledPhrase = filterInfoSections(
                    sections, order, "ratio channels pixel aspect");
                FTK_CHECK(shuffledPhrase == phraseAnd);

                // Filtering must use the current information, not results
                // cached for a prior media file.
                InfoSections changed = sections;
                changed["Video"].clear();
                const auto changedResult = filterInfoSections(
                    changed, order, "name resolution");
                for (const auto& name : order)
                {
                    FTK_CHECK(changedResult.at(name).empty());
                }

                // Adding an optional combined field must not hide existing
                // matches for the same implied-AND query.
                InfoSections optionalField = sections;
                optionalField["Video"].push_back({
                    "Source Format", "YUV 420P" });
                const auto beforeField = filterInfoSections(
                    optionalField, order, "source channels");
                FTK_CHECK(beforeField.at("Video").size() == 1);
                FTK_CHECK(beforeField.at("Audio").size() == 1);

                optionalField["Audio"].push_back({
                    "Source Channels", "6" });
                const auto afterField = filterInfoSections(
                    optionalField, order, "source channels");
                FTK_CHECK(afterField.at("Video") == beforeField.at("Video"));
                FTK_CHECK(afterField.at("Audio").size() ==
                    beforeField.at("Audio").size() + 1);
            }
        };
    }
}
