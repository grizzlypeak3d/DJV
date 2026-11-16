// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/ExportQueue.h>

#include <tlRender/Core/Audio.h>
#include <tlRender/IO/System.h>

#include <ftk/Core/Context.h>
#include <ftk/Core/Error.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>
#include <ftk/Core/Timer.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>

namespace djv
{
    namespace models
    {
        namespace
        {
            //! How many frames are asked for ahead of the one being
            //! rendered, so that reading overlaps rendering. Each holds its
            //! decoded frames until it is rendered.
            const size_t requestAhead = 3;

            //! How many rendered frames wait for the writer. A few smooth
            //! out an encoder that takes longer over some frames than
            //! others; each is a whole frame, 66 MB at 4K in 16 bits.
            const size_t writeAhead = 3;
        }

        FTK_ENUM_IMPL(
            ExportStatus,
            "Queued",
            "Running",
            "Finished",
            "Failed",
            "Cancelled");

        bool isDone(ExportStatus value)
        {
            return
                ExportStatus::Finished == value ||
                ExportStatus::Failed == value ||
                ExportStatus::Cancelled == value;
        }

        std::string getLabel(const ExportProgress& value)
        {
            std::string out;
            switch (value.status)
            {
            case ExportStatus::Queued:
                out = "Queued";
                break;
            case ExportStatus::Running:
                out = value.timeLeft.empty() ?
                    ftk::Format("Frame {0} / {1}").
                        arg(value.framesDone).
                        arg(value.frames).str() :
                    ftk::Format("Frame {0} / {1}, {2}").
                        arg(value.framesDone).
                        arg(value.frames).
                        arg(value.timeLeft).str();
                break;
            case ExportStatus::Finished:
                out = "Done";
                break;
            case ExportStatus::Failed:
                out = value.error.empty() ?
                    std::string("Failed") :
                    ftk::Format("Failed: {0}").arg(value.error).str();
                break;
            case ExportStatus::Cancelled:
                out = "Cancelled";
                break;
            default: break;
            }
            return out;
        }

        struct ExportJob::Private
        {
            std::weak_ptr<ftk::Context> context;
            ExportJobOptions options;
            std::shared_ptr<ftk::Observable<ExportProgress> > progress;

            ExportSources sources;
            std::shared_ptr<ExportRender> render;
            std::shared_ptr<tl::IWrite> writer;
            std::chrono::steady_clock::time_point startTime;

            //! The next frame to ask for, and the ones asked for and not
            //! yet rendered, in order.
            int64_t nextFrame = 0;
            std::deque<ExportFrameRequest> requests;

            struct WriteItem
            {
                std::shared_ptr<ftk::Image> image;
                OTIO_NS::RationalTime time;
            };
            struct Mutex
            {
                std::mutex mutex;
                std::condition_variable cv;
                std::deque<WriteItem> items;
                bool stop = false;
                bool done = false;
                bool finished = false;
                std::string error;
            };
            Mutex mutex;
            std::atomic<int64_t> framesWritten = 0;
            std::thread thread;

            //! The audio written so far, for a movie.
            double audioSeconds = 0.0;
            int64_t audioSamples = 0;
        };

        void ExportJob::_init(
            const std::shared_ptr<ftk::Context>& context,
            const ExportJobOptions& options)
        {
            FTK_P();
            p.context = context;
            p.options = options;
            ExportProgress progress;
            progress.frames = static_cast<int64_t>(options.range.duration().value());
            p.progress = ftk::Observable<ExportProgress>::create(progress);
        }

        ExportJob::ExportJob() :
            _p(new Private)
        {}

        ExportJob::~ExportJob()
        {
            _stop();
        }

        std::shared_ptr<ExportJob> ExportJob::create(
            const std::shared_ptr<ftk::Context>& context,
            const ExportJobOptions& options)
        {
            auto out = std::shared_ptr<ExportJob>(new ExportJob);
            out->_init(context, options);
            return out;
        }

        const ExportJobOptions& ExportJob::getOptions() const
        {
            return _p->options;
        }

        const ExportProgress& ExportJob::getProgress() const
        {
            return _p->progress->get();
        }

        std::shared_ptr<ftk::IObservable<ExportProgress> > ExportJob::observeProgress() const
        {
            return _p->progress;
        }

        void ExportJob::start()
        {
            FTK_P();
            auto context = p.context.lock();
            if (!context)
                return;
            ExportProgress progress = p.progress->get();
            progress.status = ExportStatus::Running;
            p.progress->setIfChanged(progress);
            try
            {
                p.sources = openExportSources(context, p.options.sources);
                if (!p.sources.timeline)
                {
                    throw std::runtime_error("No file to export");
                }
                p.render = ExportRender::create(context, p.options.render);
                auto plugin = context->getSystem<tl::WriteSystem>()->getPlugin(p.options.path);
                if (!plugin)
                {
                    throw std::runtime_error(
                        ftk::Format("Cannot open: \"{0}\"").arg(p.options.path.get()));
                }
                p.writer = plugin->write(p.options.path, p.options.outputInfo, p.options.ioOptions);
            }
            catch (const std::exception& e)
            {
                _release();
                progress.status = ExportStatus::Failed;
                progress.error = e.what();
                p.progress->setIfChanged(progress);
                throw;
            }
            p.nextFrame = p.options.range.start_time().value();
            p.startTime = std::chrono::steady_clock::now();
            p.thread = std::thread(
                [this]
                {
                    _writeThread();
                });
        }

        void ExportJob::tick(const std::function<void()>& makeCurrent)
        {
            FTK_P();
            ExportProgress progress = p.progress->get();
            if (progress.status != ExportStatus::Running)
                return;

            // Whether the writing is done.
            bool done = false;
            {
                std::unique_lock<std::mutex> lock(p.mutex.mutex);
                done = p.mutex.done;
                if (done)
                {
                    progress.error = p.mutex.error;
                    progress.status =
                        p.mutex.finished ? ExportStatus::Finished :
                        !p.mutex.error.empty() ? ExportStatus::Failed :
                        ExportStatus::Cancelled;
                }
            }
            if (done)
            {
                _stop();
                // The renderer's graphics objects go with the context they
                // were made in.
                if (makeCurrent)
                {
                    makeCurrent();
                }
                _release();
                progress.framesDone = p.framesWritten;
                progress.timeLeft.clear();
                p.progress->setIfChanged(progress);
                return;
            }

            // Ask for the frames ahead.
            const int64_t end = p.options.range.end_time_inclusive().value();
            const double rate = p.options.range.duration().rate();
            while (p.requests.size() < requestAhead && p.nextFrame <= end)
            {
                p.requests.push_back(p.render->request(
                    p.sources,
                    OTIO_NS::RationalTime(p.nextFrame, rate)));
                ++p.nextFrame;
            }

            // Render the ones that have arrived, in order, while the writer
            // has room for them.
            bool current = false;
            while (!p.requests.empty() && p.requests.front().isReady())
            {
                {
                    std::unique_lock<std::mutex> lock(p.mutex.mutex);
                    if (p.mutex.items.size() >= writeAhead)
                    {
                        break;
                    }
                }
                if (!current && makeCurrent)
                {
                    makeCurrent();
                    current = true;
                }
                ExportFrameRequest request = std::move(p.requests.front());
                p.requests.pop_front();
                Private::WriteItem item;
                try
                {
                    item.image = p.render->render(request);
                }
                catch (const std::exception& e)
                {
                    std::unique_lock<std::mutex> lock(p.mutex.mutex);
                    p.mutex.error = e.what();
                    p.mutex.stop = true;
                    p.mutex.cv.notify_all();
                    break;
                }
                // The sequence writers name each file from the time it is
                // written at, so those keep the frame numbers of the
                // timeline -- which is what the file name shown in the tool
                // promises. A movie has no frame numbers in its name and the
                // time becomes the presentation timestamp, so it starts at
                // zero.
                const int64_t start = p.options.range.start_time().value();
                const int64_t frame = static_cast<int64_t>(request.time.value());
                item.time = OTIO_NS::RationalTime(
                    ExportFileType::Movie == p.options.fileType ? frame - start : frame,
                    p.sources.speed);
                {
                    std::unique_lock<std::mutex> lock(p.mutex.mutex);
                    p.mutex.items.push_back(std::move(item));
                }
                p.mutex.cv.notify_all();
            }

            // The progress.
            progress.framesDone = p.framesWritten;
            progress.timeLeft = getExportTimeLeft(
                progress.framesDone,
                progress.frames,
                std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - p.startTime).count());
            p.progress->setIfChanged(progress);
        }

        void ExportJob::cancel()
        {
            FTK_P();
            ExportProgress progress = p.progress->get();
            if (isDone(progress.status))
                return;
            _stop();
            _release();
            progress.status = ExportStatus::Cancelled;
            progress.timeLeft.clear();
            p.progress->setIfChanged(progress);
        }

        void ExportJob::_stop()
        {
            FTK_P();
            {
                std::unique_lock<std::mutex> lock(p.mutex.mutex);
                p.mutex.stop = true;
            }
            p.mutex.cv.notify_all();
            if (p.thread.joinable())
            {
                p.thread.join();
            }
        }

        void ExportJob::_release()
        {
            FTK_P();
            // What is still being read is of no use now: let the timelines
            // drop it rather than decode it.
            for (const auto& request : p.requests)
            {
                for (size_t i = 0; i < request.requests.size(); ++i)
                {
                    const auto& timeline = 0 == i ?
                        p.sources.timeline :
                        (i - 1 < p.sources.compare.size() ? p.sources.compare[i - 1] : nullptr);
                    if (timeline)
                    {
                        timeline->cancelRequests({ request.requests[i].id });
                    }
                }
            }
            p.requests.clear();
            {
                std::unique_lock<std::mutex> lock(p.mutex.mutex);
                p.mutex.items.clear();
            }
            // The writer last: closing it finishes the file, or lets an
            // ffmpeg it started go.
            p.render.reset();
            p.sources = ExportSources();
            p.writer.reset();
        }

        void ExportJob::_writeThread()
        {
            FTK_P();
            const int64_t frames = static_cast<int64_t>(p.options.range.duration().value());
            while (true)
            {
                Private::WriteItem item;
                {
                    std::unique_lock<std::mutex> lock(p.mutex.mutex);
                    p.mutex.cv.wait(
                        lock,
                        [this]
                        {
                            return _p->mutex.stop || !_p->mutex.items.empty();
                        });
                    if (p.mutex.stop)
                    {
                        break;
                    }
                    item = std::move(p.mutex.items.front());
                    p.mutex.items.pop_front();
                }
                try
                {
                    p.writer->writeVideo(item.time, item.image);
                    item.image.reset();
                    const int64_t written = ++p.framesWritten;
                    _writeAudio(written);
                    if (written >= frames)
                    {
                        p.writer->finish();
                        std::unique_lock<std::mutex> lock(p.mutex.mutex);
                        p.mutex.finished = true;
                        break;
                    }
                }
                catch (const std::exception& e)
                {
                    std::unique_lock<std::mutex> lock(p.mutex.mutex);
                    p.mutex.error = e.what();
                    break;
                }
            }
            std::unique_lock<std::mutex> lock(p.mutex.mutex);
            p.mutex.done = true;
        }

        void ExportJob::_writeAudio(int64_t framesWritten)
        {
            FTK_P();
            if (!p.options.hasAudio)
                return;
            const double videoSeconds = OTIO_NS::RationalTime(
                framesWritten,
                p.sources.speed).rescaled_to(1.0).value();
            while (p.audioSeconds < std::min(videoSeconds, p.options.audioDurationSeconds))
            {
                // One second of audio from the timeline, the layers mixed
                // together.
                auto frame = p.sources.timeline->getAudio(
                    p.options.audioStartSeconds + p.audioSeconds).future.get();
                std::vector<std::shared_ptr<tl::Audio> > layers;
                for (const auto& layer : frame.layers)
                {
                    if (layer.audio)
                    {
                        layers.push_back(layer.audio);
                    }
                }
                auto audio = tl::mixAudio(layers, 1.F);
                if (audio && audio->isValid())
                {
                    // The last second trimmed to the in/out range.
                    const double remaining = p.options.audioDurationSeconds - p.audioSeconds;
                    if (remaining < 1.0)
                    {
                        const size_t sampleCount = std::min(
                            audio->getSampleCount(),
                            static_cast<size_t>(remaining * audio->getInfo().sampleRate + .5));
                        auto tmp = tl::Audio::create(audio->getInfo(), sampleCount);
                        std::memcpy(tmp->getData(), audio->getData(), tmp->getByteCount());
                        audio = tmp;
                    }
                    const OTIO_NS::TimeRange timeRange(
                        OTIO_NS::RationalTime(p.audioSamples, audio->getInfo().sampleRate),
                        OTIO_NS::RationalTime(audio->getSampleCount(), audio->getInfo().sampleRate));
                    p.writer->writeAudio(timeRange, audio);
                    p.audioSamples += audio->getSampleCount();
                }
                p.audioSeconds += 1.0;
            }
        }

        struct ExportQueue::Private
        {
            std::weak_ptr<ftk::Context> context;
            std::function<void()> makeCurrent;
            std::shared_ptr<ftk::ObservableList<std::shared_ptr<ExportJob> > > jobs;
            std::shared_ptr<ftk::Observable<bool> > busy;
            std::map<std::shared_ptr<ExportJob>, std::function<void(bool)> > callbacks;
            std::shared_ptr<ftk::Timer> timer;
        };

        void ExportQueue::_init(const std::shared_ptr<ftk::Context>& context)
        {
            FTK_P();
            p.context = context;
            p.jobs = ftk::ObservableList<std::shared_ptr<ExportJob> >::create();
            p.busy = ftk::Observable<bool>::create(false);
            p.timer = ftk::Timer::create(context);
            p.timer->setRepeating(true);
        }

        ExportQueue::ExportQueue() :
            _p(new Private)
        {}

        ExportQueue::~ExportQueue()
        {
            FTK_P();
            p.timer->stop();
            for (const auto& job : p.jobs->get())
            {
                job->cancel();
            }
        }

        std::shared_ptr<ExportQueue> ExportQueue::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            auto out = std::shared_ptr<ExportQueue>(new ExportQueue);
            out->_init(context);
            return out;
        }

        void ExportQueue::setMakeCurrent(const std::function<void()>& value)
        {
            _p->makeCurrent = value;
        }

        std::shared_ptr<ExportJob> ExportQueue::add(
            const ExportJobOptions& options,
            const std::function<void(bool)>& callback)
        {
            FTK_P();
            std::shared_ptr<ExportJob> out;
            if (auto context = p.context.lock())
            {
                out = ExportJob::create(context, options);
                if (callback)
                {
                    p.callbacks[out] = callback;
                }
                p.jobs->pushBack(out);
                _busyUpdate();
                if (!p.timer->isActive())
                {
                    std::weak_ptr<ExportQueue> weak(shared_from_this());
                    p.timer->start(
                        std::chrono::microseconds(500),
                        [weak]
                        {
                            if (auto queue = weak.lock())
                            {
                                queue->_tick();
                            }
                        });
                }
            }
            return out;
        }

        void ExportQueue::cancel(const std::shared_ptr<ExportJob>& job)
        {
            FTK_P();
            if (job && !isDone(job->getProgress().status))
            {
                // The renderer's graphics objects go with the context they
                // were made in.
                if (p.makeCurrent)
                {
                    p.makeCurrent();
                }
                job->cancel();
                _finish(job);
                _busyUpdate();
            }
        }

        void ExportQueue::cancelAll()
        {
            FTK_P();
            // A copy: a callback told of the cancel may add an export.
            const auto jobs = p.jobs->get();
            for (const auto& job : jobs)
            {
                cancel(job);
            }
        }

        void ExportQueue::remove(const std::shared_ptr<ExportJob>& job)
        {
            FTK_P();
            if (job && isDone(job->getProgress().status))
            {
                const auto& jobs = p.jobs->get();
                const auto i = std::find(jobs.begin(), jobs.end(), job);
                if (i != jobs.end())
                {
                    p.jobs->removeItem(i - jobs.begin());
                }
            }
        }

        void ExportQueue::clearDone()
        {
            FTK_P();
            std::vector<std::shared_ptr<ExportJob> > jobs;
            for (const auto& job : p.jobs->get())
            {
                if (!isDone(job->getProgress().status))
                {
                    jobs.push_back(job);
                }
            }
            p.jobs->setIfChanged(jobs);
        }

        std::shared_ptr<ftk::IObservableList<std::shared_ptr<ExportJob> > > ExportQueue::observeJobs() const
        {
            return _p->jobs;
        }

        std::shared_ptr<ftk::IObservable<bool> > ExportQueue::observeBusy() const
        {
            return _p->busy;
        }

        bool ExportQueue::isWriting(const ftk::Path& path) const
        {
            FTK_P();
            for (const auto& job : p.jobs->get())
            {
                if (!isDone(job->getProgress().status) &&
                    job->getOptions().path.get() == path.get())
                {
                    return true;
                }
            }
            return false;
        }

        void ExportQueue::_tick()
        {
            FTK_P();
            // The running export, or the next one to start. A copy of the
            // list: a callback told of an export that could not start may
            // add another.
            const auto jobs = p.jobs->get();
            std::shared_ptr<ExportJob> job;
            for (const auto& i : jobs)
            {
                const ExportStatus status = i->getProgress().status;
                if (ExportStatus::Running == status)
                {
                    job = i;
                    break;
                }
            }
            if (!job)
            {
                for (const auto& i : jobs)
                {
                    if (ExportStatus::Queued == i->getProgress().status)
                    {
                        if (p.makeCurrent)
                        {
                            p.makeCurrent();
                        }
                        try
                        {
                            i->start();
                            job = i;
                        }
                        catch (const std::exception&)
                        {
                            _finish(i);
                            continue;
                        }
                        break;
                    }
                }
            }
            if (job)
            {
                job->tick(p.makeCurrent);
                if (isDone(job->getProgress().status))
                {
                    _finish(job);
                }
            }
            _busyUpdate();
            if (!p.busy->get())
            {
                p.timer->stop();
            }
        }

        void ExportQueue::_finish(const std::shared_ptr<ExportJob>& job)
        {
            FTK_P();
            const ExportProgress& progress = job->getProgress();
            if (auto context = p.context.lock())
            {
                const std::string& path = job->getOptions().path.get();
                switch (progress.status)
                {
                case ExportStatus::Finished:
                    context->log(
                        "djv::models::ExportQueue",
                        ftk::Format("Exported: \"{0}\"").arg(path));
                    break;
                case ExportStatus::Failed:
                    context->log(
                        "djv::models::ExportQueue",
                        ftk::Format("Export failed: \"{0}\": {1}").arg(path).arg(progress.error),
                        ftk::LogType::Error);
                    break;
                case ExportStatus::Cancelled:
                    context->log(
                        "djv::models::ExportQueue",
                        ftk::Format("Export cancelled: \"{0}\"").arg(path));
                    break;
                default: break;
                }
            }
            const auto i = p.callbacks.find(job);
            if (i != p.callbacks.end())
            {
                // Taken out first: the callback may add an export, or quit.
                const auto callback = i->second;
                p.callbacks.erase(i);
                callback(ExportStatus::Finished == progress.status);
            }
        }

        void ExportQueue::_busyUpdate()
        {
            FTK_P();
            bool busy = false;
            for (const auto& job : p.jobs->get())
            {
                if (!isDone(job->getProgress().status))
                {
                    busy = true;
                    break;
                }
            }
            p.busy->setIfChanged(busy);
        }
    }
}
