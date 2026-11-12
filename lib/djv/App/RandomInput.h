// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>

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

        //! Random input for a while: clicks, drags, scrolls, keys, two
        //! finger gestures, menus, seeks and play and stop, aimed at the
        //! picture, the timeline, the menu bar and anywhere in the window,
        //! with playback running. What a tester does to an application in
        //! its first minute, done by the thousand, to shake out a crash
        //! that comes of two things happening at once. The sequence is set
        //! by the seed, so a run that failed can be run again.
        class DJV_APP_API_TYPE RandomInput : public std::enable_shared_from_this<RandomInput>
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                double seconds,
                unsigned seed);

            RandomInput();

        public:
            DJV_APP_API ~RandomInput();

            DJV_APP_API static std::shared_ptr<RandomInput> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                double seconds,
                unsigned seed);

            //! Start; the file on the command line is waited for.
            DJV_APP_API bool begin();

            //! Whether the time ran out with nothing logged as an error.
            DJV_APP_API bool succeeded() const;

        private:
            void _tick();
            void _note(const std::string&);

            FTK_PRIVATE();
        };
    }
}
