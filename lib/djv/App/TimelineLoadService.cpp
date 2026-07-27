// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/TimelineLoadService.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace djv
{
    namespace app
    {
        struct TimelineLoadService::Private
        {
            struct Job
            {
                uint64_t id = 0;
                std::shared_ptr<ftk::Context> context;
                ftk::Path path;
                ftk::Path audioPath;
                tl::Options options;
                std::shared_ptr<tl::TimelineInitCancellation> cancellation =
                    std::make_shared<tl::TimelineInitCancellation>();
                std::promise<TimelineLoadResult> promise;
                std::shared_future<TimelineLoadResult> future =
                    promise.get_future().share();
            };

            explicit Private(const Loader& value) :
                loader(
                    value ?
                    value :
                    [](const auto& context,
                        const auto& path,
                        const auto& audioPath,
                        const auto& options,
                        const auto& cancellation)
                    {
                        return tl::Timeline::create(
                            context,
                            path,
                            audioPath,
                            options,
                            cancellation);
                    })
            {
                thread = std::thread([this] { run(); });
            }

            void run()
            {
                for (;;)
                {
                    std::shared_ptr<Job> job;
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        cv.wait(
                            lock,
                            [this] { return stopping || !queue.empty(); });
                        if (stopping && queue.empty())
                        {
                            break;
                        }
                        job = queue.front();
                        queue.pop_front();
                        current = job;
                    }

                    TimelineLoadResult result;
                    if (job->cancellation->isCancelled())
                    {
                        result.cancelled = true;
                        result.error = "Timeline loading cancelled";
                    }
                    else
                    {
                        try
                        {
                            result.timeline = loader(
                                job->context,
                                job->path,
                                job->audioPath,
                                job->options,
                                job->cancellation);
                            result.cancelled =
                                job->cancellation->isCancelled();
                            if (result.cancelled && result.error.empty())
                            {
                                result.timeline.reset();
                                result.error = "Timeline loading cancelled";
                            }
                        }
                        catch (const std::exception& e)
                        {
                            result.cancelled =
                                job->cancellation->isCancelled();
                            result.error = result.cancelled ?
                                "Timeline loading cancelled" :
                                e.what();
                        }
                        catch (...)
                        {
                            result.cancelled =
                                job->cancellation->isCancelled();
                            result.error = result.cancelled ?
                                "Timeline loading cancelled" :
                                "Unknown timeline loading error";
                        }
                    }
                    job->promise.set_value(std::move(result));
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        if (current == job)
                        {
                            current.reset();
                        }
                    }
                }
            }

            Loader loader;
            std::mutex mutex;
            std::condition_variable cv;
            std::deque<std::shared_ptr<Job> > queue;
            std::shared_ptr<Job> current;
            std::thread thread;
            uint64_t nextId = 1;
            bool stopping = false;
            std::mutex shutdownMutex;
        };

        TimelineLoadService::TimelineLoadService(const Loader& loader) :
            _p(new Private(loader))
        {}

        TimelineLoadService::~TimelineLoadService()
        {
            shutdown();
        }

        TimelineLoadRequest TimelineLoadService::request(
            const std::shared_ptr<ftk::Context>& context,
            const ftk::Path& path,
            const ftk::Path& audioPath,
            const tl::Options& options)
        {
            auto job = std::make_shared<Private::Job>();
            job->context = context;
            job->path = path;
            job->audioPath = audioPath;
            job->options = options;
            TimelineLoadRequest out;
            {
                std::lock_guard<std::mutex> lock(_p->mutex);
                if (_p->stopping)
                {
                    return out;
                }
                job->id = _p->nextId++;
                _p->queue.push_back(job);
                out.id = job->id;
                out.future = job->future;
            }
            _p->cv.notify_one();
            return out;
        }

        bool TimelineLoadService::cancel(uint64_t id)
        {
            std::lock_guard<std::mutex> lock(_p->mutex);
            if (_p->current && _p->current->id == id)
            {
                _p->current->cancellation->cancel();
                return true;
            }
            for (const auto& job : _p->queue)
            {
                if (job->id == id)
                {
                    job->cancellation->cancel();
                    return true;
                }
            }
            return false;
        }

        void TimelineLoadService::shutdown()
        {
            std::lock_guard<std::mutex> shutdownLock(_p->shutdownMutex);
            {
                std::lock_guard<std::mutex> lock(_p->mutex);
                if (_p->stopping && !_p->thread.joinable())
                {
                    return;
                }
                _p->stopping = true;
                if (_p->current)
                {
                    _p->current->cancellation->cancel();
                }
                for (const auto& job : _p->queue)
                {
                    job->cancellation->cancel();
                }
            }
            _p->cv.notify_all();
            if (_p->thread.joinable())
            {
                _p->thread.join();
            }
        }
    }
}
