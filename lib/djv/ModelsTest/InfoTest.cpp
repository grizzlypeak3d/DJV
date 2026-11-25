// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/InfoTest.h>

#include <djv/Models/Info.h>

#include <ftk/Core/Assert.h>

#include <algorithm>

namespace djv
{
    namespace models_tests
    {
        InfoTest::InfoTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::InfoTest")
        {}

        std::shared_ptr<InfoTest> InfoTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<InfoTest>(new InfoTest(context));
        }

        void InfoTest::run()
        {
            _rows();
            _header();
            _filter();
        }

        namespace
        {
            const models::InfoItem* find(
                const std::vector<models::InfoItem>& items,
                const std::string& section,
                const std::string& name)
            {
                const auto i = std::find_if(
                    items.begin(),
                    items.end(),
                    [&](const models::InfoItem& item)
                    {
                        return item.section == section && item.name == name;
                    });
                return i != items.end() ? &*i : nullptr;
            }
        }

        void InfoTest::_rows()
        {
            // Nothing open, nothing to say.
            FTK_CHECK(models::getInfo(ftk::Path(), tl::IOInfo()).empty());

            const std::vector<std::string> sections = models::getInfoSections();
            FTK_CHECK(5 == sections.size());
            FTK_CHECK("File" == sections.front());
            FTK_CHECK("Metadata" == sections.back());

            tl::IOInfo info;
            info.video.push_back(ftk::ImageInfo(1920, 1080, ftk::ImageType::RGBA_U8));
            info.videoTime = OTIO_NS::TimeRange(
                OTIO_NS::RationalTime(0.0, 24.0),
                OTIO_NS::RationalTime(48.0, 24.0));
            info.videoSource.codec = "ProRes";
            info.audio = tl::AudioInfo(2, tl::AudioType::F32, 48000);
            info.audioSource.channelCount = 6;
            info.audioSource.sampleRate = 48000;
            info.audioTime = OTIO_NS::TimeRange(
                OTIO_NS::RationalTime(0.0, 48000.0),
                OTIO_NS::RationalTime(96000.0, 48000.0));
            info.tags["Camera"] = "A";
            const auto items = models::getInfo(ftk::Path("/shots/a.mov"), info);

            // In section order; a movie has no header.
            std::vector<std::string> order;
            for (const auto& item : items)
            {
                if (order.empty() || order.back() != item.section)
                {
                    order.push_back(item.section);
                }
            }
            const std::vector<std::string> noHeader = { "File", "Video", "Audio", "Metadata" };
            FTK_CHECK(noHeader == order);

            FTK_CHECK(find(items, "File", "Name") && "a.mov" == find(items, "File", "Name")->value);
            FTK_CHECK(!find(items, "File", "Frames"));
            FTK_CHECK(!find(items, "File", "Missing"));

            // A sequence missing frames says how many and which.
            {
                ftk::Path seq("/renders/comp.1001.exr");
                seq.setSeq(ftk::toFrameSeq({ 1001, 1002, 1003, 1006, 1008 }));
                const auto seqItems = models::getInfo(seq, info);
                FTK_CHECK(find(seqItems, "File", "Frames") &&
                    "1001-1008 (3 missing)" == find(seqItems, "File", "Frames")->value);
                FTK_CHECK(find(seqItems, "File", "Missing") &&
                    "1004-1005,1007" == find(seqItems, "File", "Missing")->value);
            }
            FTK_CHECK(find(items, "Video", "Codec") && "ProRes" == find(items, "Video", "Codec")->value);
            FTK_CHECK(!find(items, "Video", "Source Format"));
            FTK_CHECK(find(items, "Video", "Resolution") && "1920 1080" == find(items, "Video", "Resolution")->value);
            FTK_CHECK(find(items, "Video", "Speed") && "24.00 FPS" == find(items, "Video", "Speed")->value);
            FTK_CHECK(find(items, "Audio", "Sample Rate") && "48.0kHz" == find(items, "Audio", "Sample Rate")->value);
            FTK_CHECK(find(items, "Audio", "Duration") && "2.00 seconds" == find(items, "Audio", "Duration")->value);
            // The source rows only where the file differs from what is read.
            FTK_CHECK(find(items, "Audio", "Source Channels") && "6" == find(items, "Audio", "Source Channels")->value);
            FTK_CHECK(!find(items, "Audio", "Source Sample Rate"));
            FTK_CHECK(find(items, "Metadata", "Camera") && "A" == find(items, "Metadata", "Camera")->value);
        }

        void InfoTest::_header()
        {
            tl::IOInfo info;
            info.video.push_back(ftk::ImageInfo(16, 16, ftk::ImageType::RGBA_F16));

            // One part: the header rows, out of the metadata.
            info.tags["Data Window"] = "2 3 9 12";
            info.tags["Display Window"] = "0 0 15 15";
            info.tags["Channels"] = "A B G R";
            info.tags["Compression"] = "PIZ";
            info.tags["Line Order"] = "Increasing Y";
            {
                const auto items = models::getInfo(ftk::Path("/renders/a.exr"), info);
                FTK_CHECK(find(items, "Header", "Data Window") &&
                    "2 3 9 12 (8 x 10)" == find(items, "Header", "Data Window")->value);
                FTK_CHECK(find(items, "Header", "Display Window") &&
                    "0 0 15 15 (16 x 16)" == find(items, "Header", "Display Window")->value);
                FTK_CHECK(find(items, "Header", "Compression"));
                FTK_CHECK(!find(items, "Metadata", "Data Window"));
                FTK_CHECK(!find(items, "Header", "Parts"));
                FTK_CHECK(find(items, "Metadata", "Line Order"));
                // After the file, before the video.
                std::vector<std::string> order;
                for (const auto& item : items)
                {
                    if (order.empty() || order.back() != item.section)
                    {
                        order.push_back(item.section);
                    }
                }
                const std::vector<std::string> result = { "File", "Header", "Video", "Metadata" };
                FTK_CHECK(result == order);
            }

            // Two parts, the frame from the second.
            info.tags["Name"] = "beauty";
            info.tags["Parts"] = "2";
            info.tags["Part 1/Name"] = "depth";
            info.tags["Part 1/Data Window"] = "0 0 15 15";
            info.tags["Part 1/Display Window"] = "0 0 15 15";
            info.tags["Part 1/Channels"] = "Z";
            info.tags["Part 1/Compression"] = "ZIP";
            info.tags["Part 1/renderer"] = "depth pass";
            {
                ftk::ImageTags frameTags = info.tags;
                frameTags["Part"] = "1";
                // The frame's own: it is not the file's first frame.
                frameTags["Data Window"] = "4 4 7 7";
                const auto items = models::getInfo(ftk::Path("/renders/a.exr"), info, 0, frameTags);
                FTK_CHECK(find(items, "Header", "Parts") && "2" == find(items, "Header", "Parts")->value);
                FTK_CHECK(find(items, "Header", "Part 0") && "beauty" == find(items, "Header", "Part 0")->value);
                FTK_CHECK(find(items, "Header", "Part 1") && "depth (shown)" == find(items, "Header", "Part 1")->value);
                FTK_CHECK(find(items, "Header", "  Data Window") &&
                    "4 4 7 7 (4 x 4)" == find(items, "Header", "  Data Window")->value);
                size_t channels = 0;
                for (const auto& item : items)
                {
                    if ("Header" == item.section && "  Channels" == item.name)
                    {
                        ++channels;
                    }
                }
                FTK_CHECK(2 == channels);
                // A part's other tags stay in the metadata.
                FTK_CHECK(find(items, "Metadata", "Part 1/renderer"));
                FTK_CHECK(!find(items, "Metadata", "Part 1/Channels"));
                FTK_CHECK(!find(items, "Metadata", "Part"));
                FTK_CHECK(!find(items, "Metadata", "Parts"));
                FTK_CHECK(!find(items, "Metadata", "Name"));
            }

            // The frame tags come from the A side's first layer.
            {
                FTK_CHECK(models::getFrameTags({}).empty());
                tl::VideoFrame frame;
                tl::VideoLayer layer;
                layer.image = ftk::Image::create(ftk::ImageInfo(1, 1, ftk::ImageType::L_U8));
                layer.image->setTags({ { "Part", "1" } });
                frame.layers.push_back(layer);
                const auto tags = models::getFrameTags({ frame });
                FTK_CHECK(1 == tags.size() && "1" == tags.at("Part"));
            }
        }

        void InfoTest::_filter()
        {
            const std::vector<models::InfoItem> items =
            {
                { "File", "Name", "a.mov" },
                { "Video", "Duration", "00:00:02:00" },
                { "Audio", "Codec", "AAC" }
            };
            FTK_CHECK(items == models::filterInfo(items, ""));
            FTK_CHECK(items == models::filterInfo(items, "  "));
            // Each word on its own, in the name or the value, case ignored.
            const auto a = models::filterInfo(items, "name DURATION");
            FTK_CHECK(2 == a.size());
            FTK_CHECK("Name" == a[0].name);
            FTK_CHECK("Duration" == a[1].name);
            const auto b = models::filterInfo(items, "aac");
            FTK_CHECK(1 == b.size() && "Codec" == b[0].name);
            FTK_CHECK(models::filterInfo(items, "exr").empty());
        }
    }
}
