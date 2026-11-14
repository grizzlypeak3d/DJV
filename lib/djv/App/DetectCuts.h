// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>

#include <ftk/Core/Util.h>

#include <memory>

namespace ftk
{
    class Context;
}

namespace djv
{
    namespace app
    {
        class App;

        //! Timeline > Detect Cuts: finds the cuts in the current file, over
        //! the in/out range, and marks each one. The frames are read on a
        //! thread of its own through a timeline of its own, so playback and
        //! the player's cache are left alone, with a progress dialog to
        //! cancel it from. What is marked is the models' (see
        //! models::MarkersModel::addCuts()).
        class DJV_APP_API_TYPE DetectCuts : public std::enable_shared_from_this<DetectCuts>
        {
            FTK_NON_COPYABLE(DetectCuts);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            DetectCuts();

        public:
            DJV_APP_API ~DetectCuts();

            //! Create a new object.
            DJV_APP_API static std::shared_ptr<DetectCuts> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            //! Start finding the cuts in the current file. Nothing while it
            //! is already running, or with no file open.
            DJV_APP_API void start();

            //! Get whether it is running.
            DJV_APP_API bool isRunning() const;

        private:
            void _finish();

            FTK_PRIVATE();
        };
    }
}
