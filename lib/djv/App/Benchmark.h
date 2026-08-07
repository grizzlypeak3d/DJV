// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/Core/Util.h>

#include <memory>
#include <string>

namespace ftk
{
    class Context;
}

namespace djv
{
    namespace app
    {
        class App;

        //! Headless playback measurement.
        //!
        //! Plays for a while with no window on screen and reports what the
        //! viewport actually achieved, so that a change to the render path can
        //! be measured rather than guessed at. Like Capture, it runs from a
        //! timer inside the normal event loop, which is what sizes the window
        //! and produces a buffer to draw into.
        //!
        //! What is measured is the rate frames reach the viewport and how many
        //! were dropped -- whether playback keeps up. It is not a measure of
        //! what drawing costs: the only per-frame render timing ftk keeps is a
        //! DiagSystem sampler, which ticks once every three seconds and is
        //! rounded to a millisecond, so it cannot tell two filters apart.
        class Benchmark : public std::enable_shared_from_this<Benchmark>
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                double seconds);

            Benchmark();

        public:
            ~Benchmark();

            static std::shared_ptr<Benchmark> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                double seconds);

            //! Start playback and arm the sampling timer. Returns false if
            //! there is nothing to play. After this returns true, the caller
            //! runs the event loop.
            bool begin();

            //! Whether the run produced a measurement.
            bool succeeded() const;

        private:
            void _tick();
            void _report();

            FTK_PRIVATE();
        };
    }
}
