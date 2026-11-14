// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/DetectCuts.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>

#include <djv/Models/MarkersModel.h>

#include <djv/UI/Viewport.h>

#include <tlRender/Timeline/DetectCuts.h>
#include <tlRender/Timeline/Player.h>

#include <ftk/UI/ProgressDialog.h>
#include <ftk/Core/Context.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/LogSystem.h>
#include <ftk/Core/String.h>
#include <ftk/Core/Timer.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

namespace djv
{
    namespace app
    {
        struct DetectCuts::Private
        {
            std::weak_ptr<ftk::Context> context;
            std::weak_ptr<App> app;

            //! The player's range searched, and where the player's timeline
            //! starts: the cuts come back in the detecting timeline's time,
            //! and are put back into the player's.
            OTIO_NS::TimeRange range;
            OTIO_NS::RationalTime playerStart;

            std::thread thread;
            std::atomic<bool> running = false;
            std::atomic<bool> cancel = false;
            std::atomic<bool> done = false;
            std::atomic<size_t> frame = 0;
            std::atomic<size_t> count = 0;
            std::mutex mutex;
            std::vector<OTIO_NS::RationalTime> cuts;
            OTIO_NS::RationalTime detectStart;
            std::string error;

            std::chrono::steady_clock::time_point startTime;
            std::string path;

            std::shared_ptr<ftk::ProgressDialog> dialog;
            std::shared_ptr<ftk::Timer> timer;
        };

        void DetectCuts::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            FTK_P();
            p.context = context;
            p.app = app;
            p.timer = ftk::Timer::create(context);
            p.timer->setRepeating(true);
        }

        DetectCuts::DetectCuts() :
            _p(new Private)
        {}

        DetectCuts::~DetectCuts()
        {
            FTK_P();
            p.cancel = true;
            if (p.thread.joinable())
            {
                p.thread.join();
            }
        }

        std::shared_ptr<DetectCuts> DetectCuts::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            auto out = std::shared_ptr<DetectCuts>(new DetectCuts);
            out->_init(context, app);
            return out;
        }

        void DetectCuts::start()
        {
            FTK_P();
            auto context = p.context.lock();
            auto app = p.app.lock();
            if (p.running || !context || !app)
            {
                return;
            }
            auto player = app->observePlayer()->get();
            if (!player)
            {
                return;
            }
            if (p.thread.joinable())
            {
                p.thread.join();
            }

            // Over the in/out range: all of it unless it has been narrowed,
            // which is how one reel of a long movie is done on its own.
            auto timeline = player->getTimeline();
            p.range = player->getInOutRange();
            p.playerStart = player->getTimeRange().start_time();
            p.running = true;
            p.cancel = false;
            p.done = false;
            p.frame = 0;
            p.count = static_cast<size_t>(p.range.duration().value());
            p.cuts.clear();
            p.error.clear();
            p.startTime = std::chrono::steady_clock::now();
            p.path = timeline->getPath().get();
            context->getLogSystem()->print(
                "djv::app::DetectCuts",
                ftk::Format("Reading {0} frames of {1}").arg(p.count.load()).arg(p.path));

            // A timeline of its own, opened on the thread: opening can take
            // seconds (a bundle, a probe), and the player's readers keep
            // serving playback. Pixels, not textures.
            tl::Options options = timeline->getOptions();
            options.ioOptions.erase("Texture");
            const ftk::Path path = timeline->getPath();
            const ftk::Path audioPath = timeline->getAudioPath();
            const OTIO_NS::TimeRange playerRange = p.range;
            const OTIO_NS::RationalTime playerStart = p.playerStart;
            std::weak_ptr<ftk::Context> contextWeak = context;
            Private* pp = _p.get();
            p.thread = std::thread(
                [pp, contextWeak, path, audioPath, options, playerRange, playerStart]
                {
                    try
                    {
                        auto context = contextWeak.lock();
                        if (!context)
                        {
                            throw std::runtime_error("No context");
                        }
                        auto detect = tl::Timeline::create(context, path, audioPath, options);
                        const OTIO_NS::RationalTime detectStart =
                            detect->getTimeRange().start_time();
                        const OTIO_NS::TimeRange range(
                            detectStart + (playerRange.start_time() - playerStart),
                            playerRange.duration());
                        auto cuts = tl::detectCuts(
                            detect,
                            range,
                            tl::DetectCutsOptions(),
                            [pp](size_t frame, size_t count)
                            {
                                pp->frame = frame;
                                pp->count = count;
                                return !pp->cancel;
                            });
                        std::lock_guard<std::mutex> lock(pp->mutex);
                        pp->cuts = std::move(cuts);
                        pp->detectStart = detectStart;
                    }
                    catch (const std::exception& e)
                    {
                        std::lock_guard<std::mutex> lock(pp->mutex);
                        pp->error = e.what();
                    }
                    pp->done = true;
                });

            if (auto window = app->getMainWindow())
            {
                p.dialog = ftk::ProgressDialog::create(
                    context,
                    "Detect Cuts",
                    "Reading the frames:");
                p.dialog->setRange(0.0, 1.0);
                std::weak_ptr<DetectCuts> weak(shared_from_this());
                p.dialog->setCloseCallback(
                    [weak]
                    {
                        // Closed by hand is cancelled; closed because the
                        // work is done, the timer has the results.
                        if (auto self = weak.lock())
                        {
                            if (!self->_p->done)
                            {
                                self->_p->cancel = true;
                            }
                            self->_p->dialog.reset();
                        }
                    });
                p.dialog->open(window);
            }
            std::weak_ptr<DetectCuts> weak(shared_from_this());
            p.timer->start(
                std::chrono::milliseconds(100),
                [weak]
                {
                    auto self = weak.lock();
                    if (!self)
                    {
                        return;
                    }
                    auto& p = *self->_p;
                    const size_t frame = p.frame;
                    const size_t count = p.count;
                    if (p.dialog)
                    {
                        p.dialog->setValue(count > 0 ? frame / static_cast<double>(count) : 0.0);
                        p.dialog->setMessage(ftk::Format("Frame: {0} / {1}").
                            arg(frame).arg(count));
                    }
                    if (p.done)
                    {
                        self->_finish();
                    }
                });
        }

        bool DetectCuts::isRunning() const
        {
            return _p->running;
        }

        void DetectCuts::_finish()
        {
            FTK_P();
            p.timer->stop();
            if (p.thread.joinable())
            {
                p.thread.join();
            }
            p.running = false;
            if (p.dialog)
            {
                p.dialog->close();
            }
            auto app = p.app.lock();
            if (!app)
            {
                return;
            }
            std::string message;
            if (!p.error.empty())
            {
                message = "Detect Cuts: " + p.error;
                if (auto context = p.context.lock())
                {
                    context->getLogSystem()->print(
                        "djv::app::DetectCuts",
                        message,
                        ftk::LogType::Error);
                }
            }
            else if (!p.cancel)
            {
                // In the player's time.
                std::vector<OTIO_NS::RationalTime> times;
                std::vector<std::string> frames;
                for (const auto& cut : p.cuts)
                {
                    times.push_back(p.playerStart + (cut - p.detectStart));
                    frames.push_back(std::to_string(
                        static_cast<int64_t>(times.back().value())));
                }
                const size_t added = app->getMarkersModel()->addCuts(times);
                if (auto context = p.context.lock())
                {
                    context->getLogSystem()->print(
                        "djv::app::DetectCuts",
                        ftk::Format("{0} cuts in {1} seconds: {2}").
                            arg(times.size()).
                            arg(std::chrono::duration<double>(
                                std::chrono::steady_clock::now() - p.startTime).count(), 1).
                            arg(ftk::join(frames, ' ')));
                }
                if (times.empty())
                {
                    message = "No cuts found";
                }
                else if (added == times.size())
                {
                    message = ftk::Format("{0} cuts marked").arg(added).str();
                }
                else
                {
                    message = ftk::Format("{0} cuts found, {1} not marked before").
                        arg(times.size()).arg(added).str();
                }
            }
            if (!message.empty())
            {
                if (auto window = app->getMainWindow())
                {
                    window->getViewport()->showHint(message);
                }
            }
        }
    }
}
