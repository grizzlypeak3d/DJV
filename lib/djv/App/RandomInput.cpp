// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/RandomInput.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>
#include <djv/UI/Viewport.h>

#include <tlRender/UI/TimelineWidget.h>
#include <tlRender/Timeline/Player.h>

#include <ftk/UI/MenuBar.h>
#include <ftk/Core/Context.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/LogSystem.h>
#include <ftk/Core/Timer.h>

#include <chrono>
#include <iostream>
#include <random>

namespace djv
{
    namespace app
    {
        namespace
        {
            // Often enough that inputs land on top of each other's effects,
            // as fingers do; a tick is a few inputs.
            const std::chrono::milliseconds tickInterval(20);
            const int inputsPerTick = 3;
        }

        struct RandomInput::Private
        {
            std::weak_ptr<ftk::Context> context;
            std::weak_ptr<App> app;
            double seconds = 0.0;
            std::mt19937 random;

            std::shared_ptr<ftk::Timer> timer;
            std::chrono::steady_clock::time_point startTime;
            size_t inputs = 0;
            size_t errorsStart = 0;
            bool started = false;
            bool success = false;
        };

        void RandomInput::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            double seconds,
            unsigned seed)
        {
            FTK_P();
            p.context = context;
            p.app = app;
            p.seconds = seconds;
            p.random.seed(seed);
        }

        RandomInput::RandomInput() :
            _p(new Private)
        {}

        RandomInput::~RandomInput()
        {}

        std::shared_ptr<RandomInput> RandomInput::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            double seconds,
            unsigned seed)
        {
            auto out = std::shared_ptr<RandomInput>(new RandomInput);
            out->_init(context, app, seconds, seed);
            return out;
        }

        namespace
        {
            size_t errorCount(const std::shared_ptr<ftk::Context>& context)
            {
                size_t out = 0;
                for (const auto& item : context->getLogSystem()->observeLogItems()->get())
                {
                    if (ftk::LogType::Error == item.type)
                    {
                        ++out;
                    }
                }
                return out;
            }
        }

        bool RandomInput::begin()
        {
            FTK_P();
            auto context = p.context.lock();
            auto app = p.app.lock();
            if (!context || !app)
                return false;

            // No window on screen, as the benchmark.
            app->setOffscreen(true);
            p.errorsStart = errorCount(context);
            p.startTime = std::chrono::steady_clock::now();

            p.timer = ftk::Timer::create(context);
            p.timer->setRepeating(true);
            auto weak = std::weak_ptr<RandomInput>(shared_from_this());
            p.timer->start(tickInterval, [weak] {
                if (auto self = weak.lock())
                    self->_tick();
            });
            return true;
        }

        bool RandomInput::succeeded() const
        {
            return _p->success;
        }

        void RandomInput::_tick()
        {
            FTK_P();
            auto context = p.context.lock();
            auto app = p.app.lock();
            if (!context || !app || app->getWindows().empty())
                return;
            auto window = std::dynamic_pointer_cast<MainWindow>(
                app->getWindows().front());
            if (!window)
                return;
            auto player = app->observePlayer()->get();

            const auto now = std::chrono::steady_clock::now();
            // The file on the command line opens after the loop starts, so
            // the player is waited for, and the clock runs from its arrival.
            if (!p.started)
            {
                if (!player)
                {
                    if (std::chrono::duration<double>(now - p.startTime).count() > 30.0)
                    {
                        _note("no file to play");
                        p.timer->stop();
                        app->exit();
                    }
                    return;
                }
                p.started = true;
                p.startTime = now;
                player->setPlayback(tl::Playback::Forward);
            }
            const std::chrono::duration<double> elapsed = now - p.startTime;
            if (elapsed.count() >= p.seconds)
            {
                // Let go of anything held, and stop, so the run ends as a
                // session does.
                window->cancelPress();
                if (player)
                {
                    player->setPlayback(tl::Playback::Stop);
                }
                const size_t errors = errorCount(context) - p.errorsStart;
                p.success = 0 == errors;
                _note(ftk::Format("{0} inputs in {1} seconds, {2} errors logged").
                    arg(p.inputs).arg(elapsed.count(), 1).arg(errors));
                p.timer->stop();
                app->exit();
                return;
            }

            // Where to aim: the picture and the timeline most of the time,
            // the menu bar some, and anywhere at all now and then.
            const ftk::Box2I windowBox(ftk::V2I(0, 0), window->getGeometry().size());
            const ftk::Box2I viewportBox = window->getViewport()->getGeometry();
            const ftk::Box2I timelineBox = window->getTimelineWidget()->getGeometry();
            const ftk::Box2I menuBox = window->getMenuBar() ?
                window->getMenuBar()->getGeometry() :
                windowBox;
            const auto pointIn = [this](const ftk::Box2I& box)
            {
                std::uniform_int_distribution<int> x(box.min.x, std::max(box.min.x, box.max.x));
                std::uniform_int_distribution<int> y(box.min.y, std::max(box.min.y, box.max.y));
                return ftk::V2I(x(_p->random), y(_p->random));
            };
            const auto point = [&]
            {
                std::uniform_int_distribution<int> which(0, 9);
                switch (which(p.random))
                {
                case 0: case 1: case 2: case 3: return pointIn(viewportBox);
                case 4: case 5: case 6: return pointIn(timelineBox);
                case 7: return pointIn(menuBox);
                default: return pointIn(windowBox);
                }
            };
            const ftk::Key keys[] =
            {
                ftk::Key::Space, ftk::Key::Left, ftk::Key::Right, ftk::Key::Up,
                ftk::Key::Down, ftk::Key::Home, ftk::Key::End, ftk::Key::Escape,
                ftk::Key::Return, ftk::Key::Tab, ftk::Key::K, ftk::Key::J,
                ftk::Key::L, ftk::Key::I, ftk::Key::O, ftk::Key::F,
                ftk::Key::M, ftk::Key::Comma, ftk::Key::Period, ftk::Key::Equals,
                ftk::Key::Minus
            };

            for (int i = 0; i < inputsPerTick; ++i)
            {
                std::uniform_int_distribution<int> which(0, 11);
                switch (which(p.random))
                {
                case 0: case 1: case 2:
                    window->click(point());
                    break;
                case 3: case 4:
                {
                    const ftk::V2I a = point();
                    const ftk::V2I b = point();
                    window->drag({ a, ftk::V2I((a.x + b.x) / 2, (a.y + b.y) / 2), b });
                    break;
                }
                case 5:
                    window->press(point());
                    break;
                case 6:
                    window->release(point());
                    break;
                case 7:
                {
                    std::uniform_real_distribution<float> v(-5.F, 5.F);
                    window->scroll(point(), ftk::V2F(v(p.random), v(p.random)));
                    break;
                }
                case 8:
                {
                    std::uniform_int_distribution<size_t> k(0, sizeof(keys) / sizeof(keys[0]) - 1);
                    window->keyPress(keys[k(p.random)]);
                    break;
                }
                case 9:
                {
                    std::uniform_real_distribution<float> pan(-80.F, 80.F);
                    std::uniform_real_distribution<float> zoom(.5F, 2.F);
                    window->gesture(point(), ftk::V2F(pan(p.random), pan(p.random)), zoom(p.random));
                    break;
                }
                case 10:
                    window->ftk::IWindow::click(point(), ftk::MouseButton::Right);
                    break;
                default:
                    if (player)
                    {
                        const auto& range = player->getTimeRange();
                        std::uniform_real_distribution<double> t(0.0, range.duration().value());
                        player->seek(range.start_time() + OTIO_NS::RationalTime(t(p.random), range.duration().rate()));
                        std::uniform_int_distribution<int> playback(0, 2);
                        player->setPlayback(static_cast<tl::Playback>(playback(p.random)));
                    }
                    break;
                }
                ++p.inputs;
            }
        }

        void RandomInput::_note(const std::string& msg)
        {
            // The console and the log, as the benchmark: on a device with
            // no console the log is the only place this reaches.
            std::cerr << "djv random input: " << msg << std::endl;
            if (auto context = _p->context.lock())
            {
                context->log("djv::app::RandomInput", msg);
            }
        }
    }
}
