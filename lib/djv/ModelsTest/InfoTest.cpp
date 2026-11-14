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
            FTK_CHECK(4 == sections.size());
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

            // In section order.
            std::vector<std::string> order;
            for (const auto& item : items)
            {
                if (order.empty() || order.back() != item.section)
                {
                    order.push_back(item.section);
                }
            }
            FTK_CHECK(sections == order);

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
