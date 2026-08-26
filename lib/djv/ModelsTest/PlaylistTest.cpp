// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/PlaylistTest.h>

#include <djv/Models/Playlist.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Format.h>

#include <opentimelineio/clip.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/gap.h>
#include <opentimelineio/marker.h>
#include <opentimelineio/stack.h>
#include <opentimelineio/track.h>
#include <opentimelineio/transition.h>

#include <filesystem>

namespace djv
{
    namespace models_tests
    {
        namespace
        {
            std::string normalize(const std::string& fileName)
            {
                return std::filesystem::u8path(fileName).
                    lexically_normal().generic_u8string();
            }
        }

        PlaylistTest::PlaylistTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::PlaylistTest")
        {}

        std::shared_ptr<PlaylistTest> PlaylistTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<PlaylistTest>(new PlaylistTest(context));
        }

        void PlaylistTest::run()
        {
            _roundTrip();
            _foreign();
            _version();
        }

        void PlaylistTest::_roundTrip()
        {
            // None of the media needs to exist: the playlist stores paths and
            // opens none of them.
            const std::string dir =
                std::filesystem::temp_directory_path().u8string();
            const std::string mediaDir = (std::filesystem::u8path(dir) /
                "djv-PlaylistTest-media").u8string();

            models::Playlist playlist;

            auto movie = std::make_shared<models::FilesModelItem>();
            movie->path = ftk::Path((std::filesystem::u8path(mediaDir) /
                "movie.mov").u8string());
            movie->audioPath = ftk::Path((std::filesystem::u8path(mediaDir) /
                "audio.wav").u8string());
            movie->videoLayer = 2;
            movie->speed = 23.976;
            movie->currentTime = OTIO_NS::RationalTime(15.0, 24.0);
            movie->inOutRange = OTIO_NS::TimeRange(
                OTIO_NS::RationalTime(10.0, 24.0),
                OTIO_NS::RationalTime(20.0, 24.0));
            playlist.items.push_back(movie);

            auto seq = std::make_shared<models::FilesModelItem>();
            seq->path = ftk::Path((std::filesystem::u8path(mediaDir) /
                "render.0001.exr").u8string());
            seq->path.setFrames(ftk::RangeI64(1, 48));
            seq->framesStated = true;
            playlist.items.push_back(seq);

            playlist.aIndex = 1;
            playlist.bIndexes.push_back(0);
            playlist.compareOptions.compare = tl::Compare::Wipe;
            playlist.compareOptions.wipeCenter = ftk::V2F(.25F, .75F);
            playlist.compareTime = tl::CompareTime::Absolute;

            const std::string fileName = (std::filesystem::u8path(dir) /
                "djv-PlaylistTest.otio").u8string();
            models::playlistSave(fileName, playlist, 24.0);
            std::vector<std::string> report;
            const models::Playlist result =
                models::playlistOpen(fileName, report);

            // A playlist DJV wrote reads back without anything to report.
            for (const auto& line : report)
            {
                _print(line);
            }
            FTK_CHECK(report.empty());

            FTK_CHECK(2 == result.items.size());
            FTK_CHECK(
                normalize(movie->path.getFileName(true)) ==
                normalize(result.items[0]->path.getFileName(true)));
            FTK_CHECK(
                normalize(movie->audioPath.getFileName(true)) ==
                normalize(result.items[0]->audioPath.getFileName(true)));
            FTK_CHECK(2 == result.items[0]->videoLayer);
            FTK_CHECK(23.976 == result.items[0]->speed);
            FTK_CHECK(movie->currentTime == result.items[0]->currentTime);
            FTK_CHECK(movie->inOutRange == result.items[0]->inOutRange);
            FTK_CHECK(!result.items[0]->framesStated);

            FTK_CHECK(result.items[1]->path.isSeq());
            FTK_CHECK(
                ftk::RangeI64(1, 48) == result.items[1]->path.getFrames());
            FTK_CHECK(result.items[1]->framesStated);
            FTK_CHECK(-1.0 == result.items[1]->speed);
            FTK_CHECK(!result.items[1]->currentTime.has_value());

            FTK_CHECK(1 == result.aIndex);
            FTK_CHECK(std::vector<int>{ 0 } == result.bIndexes);
            FTK_CHECK(tl::Compare::Wipe == result.compareOptions.compare);
            FTK_CHECK(ftk::V2F(.25F, .75F) == result.compareOptions.wipeCenter);
            FTK_CHECK(tl::CompareTime::Absolute == result.compareTime);
        }

        void PlaylistTest::_foreign()
        {
            // A timeline another application wrote: two video tracks, a
            // transition, markers, an audio track, and its own clip metadata.
            // Everything the file list cannot carry is reported, and the
            // metadata survives a pass through DJV.
            const OTIO_NS::TimeRange range(
                OTIO_NS::RationalTime(0.0, 24.0),
                OTIO_NS::RationalTime(24.0, 24.0));

            auto clipA = new OTIO_NS::Clip(
                "a", new OTIO_NS::ExternalReference("a.mov"), range);
            auto clipB = new OTIO_NS::Clip(
                "b", new OTIO_NS::ExternalReference("b.mov"), range);
            clipB->markers().push_back(new OTIO_NS::Marker("note"));
            auto trackA = new OTIO_NS::Track(
                "A", std::nullopt, OTIO_NS::Track::Kind::video);
            trackA->append_child(clipA);
            trackA->append_child(clipB);
            // On the top track, so it survives the flattening: flattening
            // keeps what is on top wherever tracks overlap.
            OTIO_NS::AnyDictionary foreignMetadata;
            OTIO_NS::AnyDictionary shot;
            shot["shot"] = std::string("sh010");
            foreignMetadata["studio"] = shot;
            // A range that does not begin at zero, the way a clip cut from
            // the middle of a movie does not.
            const OTIO_NS::TimeRange rangeC(
                OTIO_NS::RationalTime(10.0, 24.0),
                OTIO_NS::RationalTime(24.0, 24.0));
            auto clipC = new OTIO_NS::Clip(
                "c",
                new OTIO_NS::ExternalReference("c.mov"),
                rangeC,
                foreignMetadata);
            auto trackB = new OTIO_NS::Track(
                "B", std::nullopt, OTIO_NS::Track::Kind::video);
            trackB->append_child(clipC);
            auto audioTrack = new OTIO_NS::Track(
                "Audio", std::nullopt, OTIO_NS::Track::Kind::audio);
            auto stack = new OTIO_NS::Stack;
            stack->append_child(trackA);
            stack->append_child(trackB);
            stack->append_child(audioTrack);
            OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline> timeline(
                new OTIO_NS::Timeline);
            timeline->set_tracks(stack);

            std::vector<std::string> report;
            const models::Playlist result =
                models::playlistFromOTIO(timeline, std::string(), report);
            for (const auto& line : report)
            {
                _print(line);
            }
            FTK_CHECK(!result.items.empty());
            FTK_CHECK(!report.empty());
            bool flattened = false;
            bool audioIgnored = false;
            for (const auto& line : report)
            {
                if (line.find("Flatten") != std::string::npos)
                {
                    flattened = true;
                }
                if (line.find("audio") != std::string::npos)
                {
                    audioIgnored = true;
                }
            }
            FTK_CHECK(flattened);
            FTK_CHECK(audioIgnored);

            // The foreign metadata rides the item and goes back out on the
            // clip when the playlist is saved from DJV.
            const auto& first = result.items.front();
            FTK_CHECK(first->metadata.find("studio") != first->metadata.end());

            // With no saved position, the item starts at its in point;
            // starting at zero would sit before it, where there is nothing
            // to show.
            FTK_CHECK(first->currentTime == rangeC.start_time());
            auto saved = models::playlistToOTIO(result, std::string(), 24.0);
            std::vector<OTIO_NS::SerializableObject::Retainer<OTIO_NS::Clip> > clips;
            for (const auto& child :
                OTIO_NS::dynamic_retainer_cast<OTIO_NS::Track>(
                    saved->tracks()->children().front())->children())
            {
                if (auto clip = OTIO_NS::dynamic_retainer_cast<OTIO_NS::Clip>(child))
                {
                    clips.push_back(clip);
                }
            }
            FTK_CHECK(!clips.empty());
            const auto& metadata = clips.front()->metadata();
            FTK_CHECK(metadata.find("studio") != metadata.end());
            FTK_CHECK(metadata.find("djv") != metadata.end());
        }

        void PlaylistTest::_version()
        {
            // A playlist from a newer DJV is refused rather than read in
            // part: saving what this version understood would throw the rest
            // away.
            models::Playlist playlist;
            auto item = std::make_shared<models::FilesModelItem>();
            item->path = ftk::Path("movie.mov");
            playlist.items.push_back(item);
            auto timeline = models::playlistToOTIO(playlist, std::string(), 24.0);
            auto djv = std::any_cast<OTIO_NS::AnyDictionary>(
                timeline->metadata()["djv"]);
            djv["playlist"] = int64_t(2);
            timeline->metadata()["djv"] = djv;

            bool caught = false;
            try
            {
                std::vector<std::string> report;
                models::playlistFromOTIO(timeline, std::string(), report);
            }
            catch (const std::exception& e)
            {
                _print(e.what());
                caught = true;
            }
            FTK_CHECK(caught);
        }
    }
}
