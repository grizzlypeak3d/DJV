// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/TimelineLoadService.h>

#include <ftk/Core/Assert.h>

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

int main()
{
    try
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool entered = false;
        djv::app::TimelineLoadService service(
            [&](const auto&,
                const auto&,
                const auto&,
                const auto&,
                const auto& cancellation)
            {
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    entered = true;
                }
                cv.notify_one();
                while (!cancellation->isCancelled())
                {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(2));
                }
                throw std::runtime_error(
                    "Synthetic blocked initialization cancelled");
                return std::shared_ptr<tl::Timeline>();
            });

        const auto request = service.request(
            nullptr,
            ftk::Path("blocked.mov"),
            ftk::Path(),
            tl::Options());
        FTK_ASSERT(request);
        {
            std::unique_lock<std::mutex> lock(mutex);
            FTK_ASSERT(
                cv.wait_for(
                    lock,
                    std::chrono::seconds(2),
                    [&] { return entered; }));
        }

        const auto start = std::chrono::steady_clock::now();
        service.shutdown();
        const auto elapsed = std::chrono::steady_clock::now() - start;
        FTK_ASSERT(elapsed < std::chrono::milliseconds(500));
        FTK_ASSERT(
            std::future_status::ready ==
            request.future.wait_for(std::chrono::milliseconds(0)));
        const auto result = request.future.get();
        FTK_ASSERT(result.cancelled);
        FTK_ASSERT(!result.timeline);

        // Shutdown is idempotent and rejects new work.
        service.shutdown();
        FTK_ASSERT(!service.request(
            nullptr,
            ftk::Path("late.mov"),
            ftk::Path(),
            tl::Options()));

        std::cout << "TimelineLoadServiceTest: PASS\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr <<
            "TimelineLoadServiceTest: FAIL: " <<
            e.what() <<
            '\n';
        return 1;
    }
}
