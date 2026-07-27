// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/PlaylistModel.h>
#include <djv/Models/FileFilter.h>
#include <djv/Models/FolderScanner.h>

#include <tlRender/Timeline/Init.h>

#include <ftk/Core/Context.h>

#include <opentimelineio/clip.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/gap.h>
#include <opentimelineio/stack.h>
#include <opentimelineio/track.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
    using TimelineRetainer =
        OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline>;

    void require(bool value, const std::string& message)
    {
        if (!value)
        {
            throw std::runtime_error(message);
        }
    }

    OTIO_NS::Clip* makeClip(
        const std::string& name,
        const std::string& path,
        double frames)
    {
        const OTIO_NS::TimeRange range(
            OTIO_NS::RationalTime(0.0, 24.0),
            OTIO_NS::RationalTime(frames, 24.0));
        auto out = new OTIO_NS::Clip;
        out->set_name(name);
        out->set_source_range(range);
        out->set_media_reference(new OTIO_NS::ExternalReference(path, range));
        return out;
    }

    void append(
        OTIO_NS::Composition* composition,
        OTIO_NS::Composable* child,
        const std::string& message)
    {
        OTIO_NS::ErrorStatus errorStatus;
        require(
            composition->append_child(child, &errorStatus) &&
                !OTIO_NS::is_error(errorStatus),
            message);
    }

    TimelineRetainer makePlaylist()
    {
        auto video = new OTIO_NS::Track(
            "Video", std::nullopt, OTIO_NS::Track::Kind::video);
        append(video, makeClip("A", "A.png", 1.0), "Cannot append A.");
        append(video, makeClip("B", "B.mov", 48.0), "Cannot append B.");
        append(video, makeClip("C", "C.jpg", 1.0), "Cannot append C.");

        auto audio = new OTIO_NS::Track(
            "Audio", std::nullopt, OTIO_NS::Track::Kind::audio);
        append(
            audio,
            new OTIO_NS::Gap(OTIO_NS::RationalTime(1.0, 24.0)),
            "Cannot append first audio gap.");
        append(audio, makeClip("B audio", "B.mov", 48.0), "Cannot append audio.");
        append(
            audio,
            new OTIO_NS::Gap(OTIO_NS::RationalTime(1.0, 24.0)),
            "Cannot append second audio gap.");

        auto stack = new OTIO_NS::Stack;
        append(stack, video, "Cannot append video track.");
        append(stack, audio, "Cannot append audio track.");

        TimelineRetainer out(new OTIO_NS::Timeline("Playlist"));
        out->set_tracks(stack);
        return out;
    }

    void testEditablePlaylist(const std::filesystem::path& directory)
    {
        auto model = djv::models::PlaylistModel::create();
        model->setTimeline(
            makePlaylist(),
            ftk::Path((directory / "input.otio").u8string()));

        require(model->isAvailable(), "The OTIO playlist should be available.");
        require(model->isEditable(), "The flat OTIO playlist should be editable.");
        require(model->getItems().size() == 3, "The playlist should have three items.");
        require(model->getItems()[1].hasAudio, "The movie item should keep its audio.");

        std::string error;
        require(model->move(0, 2, &error), error);
        require(model->getItems()[0].name == "B", "Move should put B first.");
        require(model->getItems()[2].name == "A", "Move should put A last.");
        require(model->getItems()[0].hasAudio, "Audio should move with its media item.");

        require(model->remove(1, &error), error);
        require(model->getItems().size() == 2, "Remove should keep two items.");
        require(model->getItems()[0].name == "B", "B should remain first.");
        require(model->getItems()[1].name == "A", "A should remain second.");

        const ftk::Path output((directory / "saved-playlist").u8string());
        require(model->save(output, &error), error);
        const std::filesystem::path saved = directory / "saved-playlist.otio";
        require(std::filesystem::exists(saved), "Save should add the .otio extension.");

        TimelineRetainer parsed(dynamic_cast<OTIO_NS::Timeline*>(
            OTIO_NS::Timeline::from_json_file(saved.u8string())));
        require(parsed.value, "The saved OTIO file should parse.");
        require(parsed->video_tracks().size() == 1, "Saved OTIO should have one video track.");
        require(
            parsed->video_tracks()[0]->children().size() == 2,
            "Saved OTIO should preserve the edited item count.");
    }

    void testReadOnlyContracts(const std::filesystem::path& directory)
    {
        auto model = djv::models::PlaylistModel::create();
        model->setTimeline(
            makePlaylist(),
            ftk::Path((directory / "packaged.otioz").u8string()));
        require(model->isAvailable(), "An .otioz timeline should be available.");
        require(!model->isEditable(), "An .otioz timeline should be read-only.");

        auto complex = makePlaylist();
        append(
            complex->tracks(),
            new OTIO_NS::Track("Extra", std::nullopt, OTIO_NS::Track::Kind::video),
            "Cannot append extra track.");
        model->setTimeline(
            complex,
            ftk::Path((directory / "complex.otio").u8string()));
        require(!model->isEditable(), "A multi-video-track OTIO should be read-only.");
    }

    void testNumberedImagesStayIndependent(
        const std::shared_ptr<ftk::Context>& context,
        const std::filesystem::path& directory)
    {
        const std::filesystem::path sampleData = DJV_TEST_SAMPLE_DATA;
        const ftk::Path first(
            (sampleData / "BART_2021-02-07.0000.jpg").u8string());
        const ftk::Path second(
            (sampleData / "BART_2021-02-07.0050.jpg").u8string());
        auto model = djv::models::PlaylistModel::create();
        std::string error;
        require(
            model->createFromMedia(
                context,
                first,
                first,
                ftk::Path((directory / "numbered-images.otio").u8string()),
                tl::Options(),
                &error),
            error);
        require(model->getItems().size() == 1, "The first numbered image must stay a still.");
        require(
            model->getItems()[0].duration.value() == 1.0,
            "A numbered still must be one frame, not an inferred sequence.");
        require(
            model->addMedia(context, second, second, tl::Options(), &error),
            error);
        require(model->getItems().size() == 2, "The second numbered image must be a separate item.");
        require(
            model->getItems()[1].duration.value() == 1.0,
            "The second numbered still must also stay one frame.");
    }

    void testFilteredFolderScan(const std::filesystem::path& directory)
    {
        const auto scanDirectory = directory / "folder-import";
        std::filesystem::create_directories(scanDirectory / "plates");
        std::filesystem::create_directories(scanDirectory / "cache");
        std::ofstream(scanDirectory / "plates" / "beauty.exr").put('\n');
        std::ofstream(scanDirectory / "plates" / "proxy.exr").put('\n');
        std::ofstream(scanDirectory / "plates" / "notes.txt").put('\n');
        std::ofstream(scanDirectory / "cache" / "beauty.exr").put('\n');

        const auto filter = djv::models::compileFileFilter(
            "name:beauty -dir:cache");
        require(static_cast<bool>(filter), "The folder filter must compile.");
        djv::models::FolderScanOptions options;
        options.fileExtensions = { ".exr" };
        options.collapseSequences = false;
        const auto result = djv::models::scanFolder(
            scanDirectory,
            *filter.filter,
            options);
        require(
            result.status == djv::models::FolderScanStatus::Completed,
            "The small folder scan must complete.");
        require(result.paths.size() == 1, "The filter should select one EXR.");
        require(
            result.paths[0].getFileName() == "beauty.exr",
            "The filtered path should be deterministic.");

        djv::models::FolderScanOptions limits = options;
        limits.maxResults = 1;
        const auto emptyFilter = djv::models::compileFileFilter(std::string());
        require(static_cast<bool>(emptyFilter), "The empty filter must compile.");
        const auto limited = djv::models::scanFolder(
            scanDirectory,
            *emptyFilter.filter,
            limits);
        require(
            limited.status == djv::models::FolderScanStatus::ResultLimitReached,
            "Reaching the result limit must be reported.");
        require(
            limited.paths.empty(),
            "A limited scan must not publish a partial playlist.");
    }
}

int main()
{
    try
    {
        const std::filesystem::path directory =
            std::filesystem::temp_directory_path() / "djv-playlist-model-test";
        std::filesystem::remove_all(directory);
        std::filesystem::create_directories(directory);
        auto context = ftk::Context::create();
        tl::init(context);
        testEditablePlaylist(directory);
        testReadOnlyContracts(directory);
        testNumberedImagesStayIndependent(context, directory);
        testFilteredFolderScan(directory);
        std::filesystem::remove_all(directory);
        std::cout << "DJV OTIO playlist model test passed." << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
