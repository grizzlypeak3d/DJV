// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/DisplaysModelTest.h>

#include <djv/ModelsTest/ModelsTestUtil.h>

#include <djv/Models/ColorModel.h>
#include <djv/Models/DisplaysModel.h>
#include <djv/Models/SettingsKeys.h>

#include <ftk/UI/Settings.h>
#include <ftk/Core/Assert.h>

#include <algorithm>

namespace djv
{
    namespace models_tests
    {
        DisplaysModelTest::DisplaysModelTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::DisplaysModelTest")
        {}

        std::shared_ptr<DisplaysModelTest> DisplaysModelTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<DisplaysModelTest>(new DisplaysModelTest(context));
        }

        void DisplaysModelTest::run()
        {
            _functions();
            _model();
        }

        void DisplaysModelTest::_functions()
        {
            const std::vector<std::string> displays = { "sRGB", "Rec.2100-PQ" };
            const std::vector<std::string> views = { "ACES 2.0", "Un-tone-mapped" };

            // Nothing chosen follows the viewport; a display the
            // configuration has keeps its view when the display has it,
            // and takes the first otherwise; one it does not have follows
            // the viewport.
            FTK_CHECK(models::DisplayColor() ==
                models::validateDisplayColor({}, displays, views));
            FTK_CHECK((models::DisplayColor{ "Rec.2100-PQ", "Un-tone-mapped" }) ==
                models::validateDisplayColor({ "Rec.2100-PQ", "Un-tone-mapped" }, displays, views));
            FTK_CHECK((models::DisplayColor{ "Rec.2100-PQ", "ACES 2.0" }) ==
                models::validateDisplayColor({ "Rec.2100-PQ", "Gone" }, displays, views));
            FTK_CHECK(models::DisplayColor() ==
                models::validateDisplayColor({ "Gone", "ACES 2.0" }, displays, views));

            // Applied, a display turns color management on and leaves the
            // input to resolve; none leaves the options as they were.
            tl::OCIOOptions off;
            off.display = "sRGB";
            off.view = "ACES 2.0";
            FTK_CHECK(off == models::applyDisplayColor(off, {}));
            const auto on = models::applyDisplayColor(off, { "Rec.2100-PQ", "Un-tone-mapped" });
            FTK_CHECK(on.enabled);
            FTK_CHECK("Rec.2100-PQ" == on.display);
            FTK_CHECK("Un-tone-mapped" == on.view);
            FTK_CHECK(off.input == on.input);

            // The settings round trip.
            models::SecondaryDisplay secondary;
            secondary.color = { "Rec.2100-PQ", "Un-tone-mapped" };
            secondary.monitor = "LG TV SSCR2";
            secondary.open = true;
            nlohmann::json json;
            models::to_json(json, secondary);
            models::SecondaryDisplay read;
            models::from_json(json, read);
            FTK_CHECK(secondary == read);
        }

        void DisplaysModelTest::_model()
        {
            auto settings = createTestSettings(_context);
            auto colorModel = models::ColorModel::create(_context, settings);
            {
                auto model = models::DisplaysModel::create(_context, settings, colorModel);
                FTK_CHECK(models::SecondaryDisplay() == model->getSecondary());
                FTK_CHECK(models::DisplayColor() == model->observeSecondaryColor()->get());
                FTK_CHECK(!model->observeSecondaryHDRTransfer()->get().has_value());

#if defined(TLRENDER_OCIO)
                // The configuration's displays, and a display chosen from
                // them is what the window is given, with a view it has.
                const auto lists = model->observeSecondaryLists()->get();
                FTK_CHECK(!lists.displays.empty());
                FTK_CHECK(!lists.views.empty());
                if (!lists.displays.empty())
                {
                    const std::string display = lists.displays.back();
                    model->setSecondaryColor({ display, "No such view" });
                    const auto color = model->observeSecondaryColor()->get();
                    FTK_CHECK(display == color.display);
                    const auto views = model->observeSecondaryLists()->get().views;
                    FTK_CHECK(std::find(views.begin(), views.end(), color.view) != views.end());
                }

                // A PQ display is HDR, and an SDR one is not; the built in
                // configuration has both.
                model->setSecondaryColor({ "Rec.2100-PQ - Display", "" });
                FTK_CHECK(tl::HDR_EOTF::ST2084 == model->observeSecondaryHDRTransfer()->get());
                model->setSecondaryColor({ "sRGB - Display", "" });
                FTK_CHECK(tl::HDR_EOTF::SDR == model->observeSecondaryHDRTransfer()->get());
#endif // TLRENDER_OCIO

                // A display the configuration does not have follows the
                // viewport, though it is kept as chosen.
                model->setSecondaryColor({ "No such display", "" });
                FTK_CHECK("No such display" == model->getSecondary().color.display);
                FTK_CHECK(models::DisplayColor() == model->observeSecondaryColor()->get());
                FTK_CHECK(!model->observeSecondaryHDRTransfer()->get().has_value());

                model->setSecondaryMonitor("LG TV SSCR2");
                model->setSecondaryOpen(true);
                FTK_CHECK(model->getSecondary().open);
                model->save();
            }
            {
                // Remembered.
                auto model = models::DisplaysModel::create(_context, settings, colorModel);
                FTK_CHECK("LG TV SSCR2" == model->getSecondary().monitor);
                FTK_CHECK("No such display" == model->getSecondary().color.display);
                FTK_CHECK(model->getSecondary().open);
            }
        }
    }
}
