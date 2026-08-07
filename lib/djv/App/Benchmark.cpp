// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/Benchmark.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>
#include <djv/App/Viewport.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/Core/Context.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Timer.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

namespace djv
{
    namespace app
    {
        namespace
        {
            const std::chrono::milliseconds tickInterval(100);

            // Playback takes a moment to reach a steady rate: the cache is
            // filling and the first frames are still being decoded. Samples
            // taken during that would report a slow render path rather than a
            // cold one, so they are thrown away.
            const double warmupSeconds = 1.0;

            void note(const std::string& msg)
            {
                std::cerr << "djv benchmark: " << msg << std::endl;
            }
        }

        struct Benchmark::Private
        {
            std::weak_ptr<ftk::Context> context;
            std::weak_ptr<App> app;
            double seconds = 0.0;

            std::shared_ptr<ftk::Timer> timer;
            std::chrono::steady_clock::time_point startTime;
            bool warm = false;
            std::vector<double> fps;
            size_t droppedStart = 0;
            bool success = false;
        };

        void Benchmark::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            double seconds)
        {
            FTK_P();
            p.context = context;
            p.app = app;
            p.seconds = seconds;
        }

        Benchmark::Benchmark() :
            _p(new Private)
        {}

        Benchmark::~Benchmark()
        {}

        std::shared_ptr<Benchmark> Benchmark::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            double seconds)
        {
            auto out = std::shared_ptr<Benchmark>(new Benchmark);
            out->_init(context, app, seconds);
            return out;
        }

        bool Benchmark::begin()
        {
            FTK_P();
            auto context = p.context.lock();
            auto app = p.app.lock();
            if (!context || !app)
                return false;

            auto player = app->observePlayer()->get();
            if (!player)
            {
                note("no file to play");
                return false;
            }

            // No window on screen: nothing to interfere with, and nothing to
            // present to. An offscreen window skips the swap, which is what
            // keeps the measurement off the monitor's refresh rate.
            app->setOffscreen(true);

            player->setPlayback(tl::Playback::Forward);
            p.startTime = std::chrono::steady_clock::now();

            p.timer = ftk::Timer::create(context);
            p.timer->setRepeating(true);
            auto weak = std::weak_ptr<Benchmark>(shared_from_this());
            p.timer->start(tickInterval, [weak] {
                if (auto self = weak.lock())
                    self->_tick();
            });
            return true;
        }

        bool Benchmark::succeeded() const
        {
            return _p->success;
        }

        void Benchmark::_tick()
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app || app->getWindows().empty())
                return;
            auto window = std::dynamic_pointer_cast<MainWindow>(
                app->getWindows().front());
            if (!window)
                return;

            const auto now = std::chrono::steady_clock::now();
            const std::chrono::duration<double> elapsed = now - p.startTime;

            if (!p.warm)
            {
                if (elapsed.count() < warmupSeconds)
                    return;
                // Start the dropped frame count from here, so that frames lost
                // while the cache filled are not charged to the render path.
                p.warm = true;
                p.startTime = now;
                if (auto player = app->observePlayer()->get())
                {
                    p.droppedStart = player->getDroppedFrames();
                }
                return;
            }

            p.fps.push_back(window->getViewport()->observeFPS()->get());


            if (elapsed.count() >= p.seconds)
            {
                _report();
                p.timer->stop();
                app->exit();
            }
        }

        void Benchmark::_report()
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return;
            auto player = app->observePlayer()->get();
            if (!player || p.fps.empty())
            {
                note("no samples");
                return;
            }

            std::vector<double> sorted = p.fps;
            std::sort(sorted.begin(), sorted.end());
            double total = 0.0;
            for (double f : sorted)
            {
                total += f;
            }
            const double mean = total / static_cast<double>(sorted.size());
            // The slowest tenth says more about whether playback stutters than
            // the single worst sample, which is usually one scheduling hiccup.
            const size_t lowCount = std::max<size_t>(1, sorted.size() / 10);
            double lowTotal = 0.0;
            for (size_t i = 0; i < lowCount; ++i)
            {
                lowTotal += sorted[i];
            }
            const double low = lowTotal / static_cast<double>(lowCount);

            const size_t dropped = player->getDroppedFrames() - p.droppedStart;
            note(ftk::Format("{0} samples over {1}s").
                arg(sorted.size()).
                arg(p.seconds, 1).str());
            note(ftk::Format("playback {0} FPS of {1} requested (slowest tenth {2})").
                arg(mean, 2).
                arg(player->getSpeed(), 2).
                arg(low, 2).str());
            note(ftk::Format("dropped frames {0}").arg(dropped).str());
            p.success = true;
        }
    }
}
