// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/WipeTest.h>

#include <djv/Models/Wipe.h>

#include <cmath>

namespace djv
{
    namespace models_tests
    {
        namespace
        {
            bool near(const ftk::V2F& a, const ftk::V2F& b, float e = .01F)
            {
                return std::abs(a.x - b.x) < e && std::abs(a.y - b.y) < e;
            }
        }

        WipeTest::WipeTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::WipeTest")
        {}

        std::shared_ptr<WipeTest> WipeTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<WipeTest>(new WipeTest(context));
        }

        void WipeTest::run()
        {
            _widget();
            _handles();
            _drag();
        }

        void WipeTest::_widget()
        {
            // Upright through the middle of a 200x100 picture, shown at
            // (10, 20) at twice its size: the turning handle above the
            // center.
            const ftk::Box2I box(0, 0, 200, 100);
            tl::CompareOptions options;
            options.compare = tl::Compare::Wipe;
            auto widget = models::getWipeWidget(options, box, ftk::V2F(10.F, 20.F), 2.F, 30.F);
            FTK_CHECK(near(widget.center, ftk::V2F(210.F, 120.F)));
            FTK_CHECK(near(widget.rotate, ftk::V2F(210.F, 90.F)));

            // Turned a quarter: the handle to the right.
            options.wipeRotation = 90.F;
            widget = models::getWipeWidget(options, box, ftk::V2F(), 1.F, 30.F);
            FTK_CHECK(near(widget.center, ftk::V2F(100.F, 50.F)));
            FTK_CHECK(near(widget.rotate, ftk::V2F(130.F, 50.F)));
        }

        void WipeTest::_handles()
        {
            const ftk::Box2I box(0, 0, 200, 100);
            tl::CompareOptions options;
            options.compare = tl::Compare::Wipe;
            const auto widget = models::getWipeWidget(options, box, ftk::V2F(), 1.F, 30.F);
            FTK_CHECK(models::WipeHandle::Rotate == models::getWipeHandle(widget, ftk::V2F(102.F, 21.F), 5.F));
            FTK_CHECK(models::WipeHandle::Center == models::getWipeHandle(widget, ftk::V2F(99.F, 52.F), 5.F));
            // On the wipe, away from the handles: nothing.
            FTK_CHECK(models::WipeHandle::None == models::getWipeHandle(widget, ftk::V2F(100.F, 90.F), 5.F));
            FTK_CHECK(models::WipeHandle::None == models::getWipeHandle(widget, ftk::V2F(150.F, 90.F), 5.F));
        }

        void WipeTest::_drag()
        {
            // The center at a point, back through the view.
            const ftk::Box2I box(0, 0, 200, 100);
            FTK_CHECK(near(
                models::getWipeCenter(ftk::V2F(110.F, 70.F), box, ftk::V2F(10.F, 20.F), 2.F),
                ftk::V2F(.25F, .25F)));
            FTK_CHECK(near(
                models::getWipeCenter(ftk::V2F(1.F, 1.F), box, ftk::V2F(), 0.F),
                ftk::V2F(.5F, .5F)));

            // The rotation with the handle above, to the right, below and
            // to the left of the center; and the handle where it was
            // turned to.
            const ftk::V2F c(50.F, 50.F);
            FTK_CHECK(std::abs(models::getWipeRotation(c, ftk::V2F(50.F, 10.F)) - 0.F) < .01F);
            FTK_CHECK(std::abs(models::getWipeRotation(c, ftk::V2F(90.F, 50.F)) - 90.F) < .01F);
            FTK_CHECK(std::abs(models::getWipeRotation(c, ftk::V2F(50.F, 90.F)) - 180.F) < .01F);
            FTK_CHECK(std::abs(models::getWipeRotation(c, ftk::V2F(10.F, 50.F)) - 270.F) < .01F);
            FTK_CHECK(0.F == models::getWipeRotation(c, c));
            tl::CompareOptions options;
            options.wipeRotation = models::getWipeRotation(c, ftk::V2F(80.F, 20.F));
            const auto widget = models::getWipeWidget(options, ftk::Box2I(0, 0, 100, 100), ftk::V2F(), 1.F, 30.F);
            const float d = 30.F / std::sqrt(2.F);
            FTK_CHECK(near(widget.rotate, ftk::V2F(50.F + d, 50.F - d)));
        }
    }
}
