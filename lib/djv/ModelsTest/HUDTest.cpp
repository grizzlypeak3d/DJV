// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/HUDTest.h>

#include <djv/Models/HUD.h>

#include <ftk/Core/Assert.h>

namespace djv
{
    namespace models_tests
    {
        HUDTest::HUDTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::HUDTest")
        {}

        std::shared_ptr<HUDTest> HUDTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<HUDTest>(new HUDTest(context));
        }

        void HUDTest::run()
        {
            _lines();
        }

        void HUDTest::_lines()
        {
            // Nothing open.
            models::HUDData data;
            FTK_CHECK("(No file)" == models::getHUDText(models::HUDItem::FileName, data));
            FTK_CHECK(models::getHUDText(models::HUDItem::Info, data).empty());
            FTK_CHECK(models::getHUDText(models::HUDItem::Cache, data).empty());
            FTK_CHECK(models::getHUDText(models::HUDItem::Render, data).empty());
            FTK_CHECK("LUT: -" == models::getHUDText(models::HUDItem::LUT, data));
            FTK_CHECK(models::getHUDText(models::HUDItem::ColorPicker, data).empty());

            // Audio alone: no frames per second to speak of.
            data.path = ftk::Path("/shots/a.wav");
            data.ioInfo.audio = tl::AudioInfo(2, tl::AudioType::F32, 48000);
            data.timeLabel = "00:00:01:00";
            FTK_CHECK("a.wav" == models::getHUDText(models::HUDItem::FileName, data));
            FTK_CHECK("Time: 00:00:01:00" == models::getHUDText(models::HUDItem::Time, data));

            // With video.
            data.path = ftk::Path("/shots/a.mov");
            data.ioInfo.video.push_back(ftk::ImageInfo(1920, 1080, ftk::ImageType::RGBA_U8));
            data.fps = 24.0;
            data.droppedFrames = 2;
            const std::string time = models::getHUDText(models::HUDItem::Time, data);
            FTK_CHECK(time.find("24.00 FPS") != std::string::npos);
            FTK_CHECK(time.find("2 dropped") != std::string::npos);
            data.missing = true;
            data.heldFrom = 10;
            FTK_CHECK(models::getHUDText(models::HUDItem::Time, data).find("held from 10") != std::string::npos);
            FTK_CHECK(models::getHUDText(models::HUDItem::Info, data).find("V: ") == 0);
            FTK_CHECK(models::getHUDText(models::HUDItem::Info, data).find(", A: ") != std::string::npos);
            FTK_CHECK(models::getHUDText(models::HUDItem::Render, data).find("Render: 1920x1080") == 0);
            data.cacheInfo.videoPercentage = 50.F;
            FTK_CHECK(models::getHUDText(models::HUDItem::Cache, data).find("50% V") != std::string::npos);
            data.lutOptions.enabled = true;
            data.lutOptions.fileName = "/luts/show.cube";
            FTK_CHECK("LUT: show.cube" == models::getHUDText(models::HUDItem::LUT, data));
        }
    }
}
