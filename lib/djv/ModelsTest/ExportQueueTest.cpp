// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/ExportQueueTest.h>

#include <djv/Models/ExportQueue.h>

#include <ftk/Core/Assert.h>

namespace djv
{
    namespace models_tests
    {
        ExportQueueTest::ExportQueueTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::ExportQueueTest")
        {}

        std::shared_ptr<ExportQueueTest> ExportQueueTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<ExportQueueTest>(new ExportQueueTest(context));
        }

        void ExportQueueTest::run()
        {
            _labels();
            _order();
            _queue();
        }

        void ExportQueueTest::_labels()
        {
            using namespace models;
            FTK_CHECK(!isDone(ExportStatus::Queued));
            FTK_CHECK(!isDone(ExportStatus::Running));
            FTK_CHECK(isDone(ExportStatus::Finished));
            FTK_CHECK(isDone(ExportStatus::Failed));
            FTK_CHECK(isDone(ExportStatus::Cancelled));

            ExportProgress progress;
            FTK_CHECK("Queued" == getLabel(progress));
            progress.status = ExportStatus::Running;
            progress.framesDone = 12;
            progress.frames = 48;
            FTK_CHECK("Frame 12 / 48" == getLabel(progress));
            progress.timeLeft = "about a minute left";
            FTK_CHECK("Frame 12 / 48, about a minute left" == getLabel(progress));
            progress.status = ExportStatus::Finished;
            FTK_CHECK("Done" == getLabel(progress));
            // How long it took, to compare one export with another (#976).
            progress.seconds = 134.2;
            FTK_CHECK("Done in 2 min 14 s" == getLabel(progress));
            FTK_CHECK("8.3 s" == getExportElapsed(8.27));
            FTK_CHECK("0.0 s" == getExportElapsed(-1.0));
            FTK_CHECK("1 min 00 s" == getExportElapsed(59.96));
            FTK_CHECK("1 h 02 min 05 s" == getExportElapsed(3725.0));
            progress.status = ExportStatus::Failed;
            FTK_CHECK("Failed" == getLabel(progress));
            progress.error = "Disk full";
            FTK_CHECK("Failed: Disk full" == getLabel(progress));
            progress.status = ExportStatus::Cancelled;
            FTK_CHECK("Cancelled" == getLabel(progress));
        }

        void ExportQueueTest::_order()
        {
            using namespace models;
            FTK_CHECK(getListOrder({}).empty());
            // Two done, one running, two waiting, as the queue runs them.
            const std::vector<size_t> order = getListOrder({
                ExportStatus::Finished,
                ExportStatus::Failed,
                ExportStatus::Running,
                ExportStatus::Queued,
                ExportStatus::Queued });
            FTK_CHECK(std::vector<size_t>({ 2, 3, 4, 1, 0 }) == order);
            // A queued export cancelled before it ran goes with the
            // stopped ones, by when it was asked for.
            FTK_CHECK(std::vector<size_t>({ 1, 2, 0 }) == getListOrder({
                ExportStatus::Finished,
                ExportStatus::Running,
                ExportStatus::Cancelled }));
        }

        void ExportQueueTest::_queue()
        {
            // The queue's timer is not ticked here, so nothing starts: what
            // is tested is the bookkeeping, which is what the Export tool
            // and the quit question go by.
            using namespace models;
            auto context = _context;
            auto queue = ExportQueue::create(context);
            FTK_CHECK(queue->observeJobs()->isEmpty());
            FTK_CHECK(!queue->observeBusy()->get());

            ExportJobOptions options;
            options.name = "a.mov";
            options.path = ftk::Path("/tmp/a.mov");
            options.range = OTIO_NS::TimeRange(
                OTIO_NS::RationalTime(0.0, 24.0),
                OTIO_NS::RationalTime(48.0, 24.0));
            int calls = 0;
            bool written = true;
            auto a = queue->add(
                options,
                [&calls, &written](bool value)
                {
                    ++calls;
                    written = value;
                });
            FTK_CHECK(a);
            FTK_CHECK(ExportStatus::Queued == a->getProgress().status);
            FTK_CHECK(48 == a->getProgress().frames);
            FTK_CHECK(queue->observeBusy()->get());
            FTK_CHECK(queue->isWriting(ftk::Path("/tmp/a.mov")));
            FTK_CHECK(!queue->isWriting(ftk::Path("/tmp/b.mov")));

            // An export that has not stopped is not removed.
            queue->remove(a);
            FTK_CHECK(1 == queue->observeJobs()->getSize());

            // Cancelling tells the callback, once, that nothing was written.
            queue->cancel(a);
            FTK_CHECK(ExportStatus::Cancelled == a->getProgress().status);
            FTK_CHECK(1 == calls);
            FTK_CHECK(!written);
            FTK_CHECK(!queue->observeBusy()->get());
            FTK_CHECK(!queue->isWriting(ftk::Path("/tmp/a.mov")));
            queue->cancel(a);
            FTK_CHECK(1 == calls);

            // A stopped export stays on the list until it is removed.
            FTK_CHECK(1 == queue->observeJobs()->getSize());
            queue->remove(a);
            FTK_CHECK(queue->observeJobs()->isEmpty());

            // Clearing leaves the ones that have not stopped.
            auto b = queue->add(options);
            options.name = "c.mov";
            options.path = ftk::Path("/tmp/c.mov");
            auto c = queue->add(options);
            queue->cancel(b);
            queue->clearDone();
            FTK_CHECK(1 == queue->observeJobs()->getSize());
            FTK_CHECK(c == queue->observeJobs()->getItem(0));
            FTK_CHECK(queue->observeBusy()->get());

            // Cancelling them all, as quitting does.
            queue->cancelAll();
            FTK_CHECK(ExportStatus::Cancelled == c->getProgress().status);
            FTK_CHECK(!queue->observeBusy()->get());
        }
    }
}
