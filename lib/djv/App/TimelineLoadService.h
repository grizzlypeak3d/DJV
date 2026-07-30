// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <tlRender/Timeline/Timeline.h>

#include <ftk/Core/Path.h>

#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>

namespace ftk { class Context; }

namespace djv
{
    namespace app
    {
        struct TimelineLoadResult
        {
            std::shared_ptr<tl::Timeline> timeline;
            std::string error;
            bool cancelled = false;
        };

        struct TimelineLoadRequest
        {
            uint64_t id = 0;
            std::shared_future<TimelineLoadResult> future;

            explicit operator bool() const
            {
                return id && future.valid();
            }
        };

        //! Single-owner timeline initialization queue. Work never detaches:
        //! shutdown cancels the active tlRender reader and joins the worker.
        class TimelineLoadService
        {
        public:
            using Loader = std::function<std::shared_ptr<tl::Timeline>(
                const std::shared_ptr<ftk::Context>&,
                const ftk::Path&,
                const ftk::Path&,
                const tl::Options&,
                const std::shared_ptr<tl::TimelineInitCancellation>&)>;

            explicit TimelineLoadService(const Loader& = Loader());
            ~TimelineLoadService();

            TimelineLoadService(const TimelineLoadService&) = delete;
            TimelineLoadService& operator = (const TimelineLoadService&) = delete;

            TimelineLoadRequest request(
                const std::shared_ptr<ftk::Context>&,
                const ftk::Path&,
                const ftk::Path&,
                const tl::Options&);
            bool cancel(uint64_t);
            void shutdown();

        private:
            struct Private;
            std::unique_ptr<Private> _p;
        };
    }
}
