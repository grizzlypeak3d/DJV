// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/ExportRender.h>
#include <djv/Models/SettingsModel.h>

#include <tlRender/IO/IO.h>

#include <ftk/Core/ObservableList.h>
#include <ftk/Core/Observable.h>

namespace djv
{
    namespace models
    {
        //! \name Export Queue
        //! Exports run one after another, each on timelines of its own, so
        //! that the file being exported can be closed, and its cache with
        //! it, and the work carry on while they run.
        ///@{

        //! What an export is doing.
        enum class DJV_MODELS_API_TYPE ExportStatus
        {
            Queued,
            Running,
            Finished,
            Failed,
            Cancelled,

            Count,
            First = Queued
        };
        FTK_ENUM(DJV_MODELS_API, ExportStatus);

        //! Everything an export is made from, taken when it is asked for:
        //! changing the settings, the color, or the file afterwards changes
        //! nothing about it.
        struct DJV_MODELS_API_TYPE ExportJobOptions
        {
            //! What the queue calls it: the output's file name.
            std::string name;
            //! The file it is made from, for saying so.
            std::string sourceName;

            ExportFileType fileType = ExportFileType::Movie;
            //! The frames, in the A file's timeline.
            OTIO_NS::TimeRange range;
            ftk::Path path;
            tl::IOInfo outputInfo;
            tl::IOOptions ioOptions;

            //! The player's sources; the export opens its own when it starts.
            ExportSources sources;
            ExportRenderOptions render;

            //! The audio, for a movie: where in the timeline it starts and
            //! how long it lasts, in seconds.
            bool hasAudio = false;
            double audioStartSeconds = 0.0;
            double audioDurationSeconds = 0.0;
        };

        //! How far an export has got.
        struct DJV_MODELS_API_TYPE ExportProgress
        {
            //! What the export is doing.
            ExportStatus status = ExportStatus::Queued;
            //! The number of frames written so far.
            int64_t framesDone = 0;
            //! The number of frames to write.
            int64_t frames = 0;
            //! "about 2 minutes left", once there is enough to go on; see
            //! getExportTimeLeft().
            std::string timeLeft;
            //! How long it ran, once it has stopped.
            double seconds = 0.0;
            //! Why the export failed.
            std::string error;

            bool operator == (const ExportProgress&) const = default;
        };

        //! Whether an export has stopped, one way or another.
        DJV_MODELS_API bool isDone(ExportStatus);

        //! The order to list exports in, given their statuses in the order
        //! they run: the running one first, so that "Am I done?" is
        //! answered at the top; then the ones waiting, in the order they
        //! will run; then the ones that have stopped, newest first. Returns
        //! indexes into the statuses. (Newest first throughout, as a list
        //! of downloads has it, put the running export under every one
        //! waiting: downloads all run at once, exports one at a time.)
        DJV_MODELS_API std::vector<size_t> getListOrder(const std::vector<ExportStatus>&);

        //! What an export's progress says in a line: "Frame 12 / 48, about a
        //! minute left", "Queued", "Failed: ...".
        DJV_MODELS_API std::string getLabel(const ExportProgress&);

        //! The time the finished exports took, added up: "3 done, 6 min 02 s
        //! in all". Empty with fewer than two, whose own rows say it. Failed
        //! and cancelled exports are not counted (#976).
        DJV_MODELS_API std::string getTotalLabel(const std::vector<ExportProgress>&);

        //! One export. The frames are asked for and rendered on the thread
        //! the queue runs on, which has the graphics context, a few ahead
        //! so that reading, rendering and writing overlap; they are written
        //! on a thread of the export's own, so that an encoder slower than
        //! the frames arrive does not hold up the application.
        class DJV_MODELS_API_TYPE ExportJob : public std::enable_shared_from_this<ExportJob>
        {
            FTK_NON_COPYABLE(ExportJob);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const ExportJobOptions&);

            ExportJob();

        public:
            DJV_MODELS_API ~ExportJob();

            DJV_MODELS_API static std::shared_ptr<ExportJob> create(
                const std::shared_ptr<ftk::Context>&,
                const ExportJobOptions&);

            //! Get the options the export was made from.
            DJV_MODELS_API const ExportJobOptions& getOptions() const;

            //! Get how far the export has got.
            DJV_MODELS_API const ExportProgress& getProgress() const;
            //! Observe how far the export has got.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<ExportProgress> > observeProgress() const;

            //! Open the timelines, the renderer, and the writer, and start
            //! writing. The graphics context must be current. Throws when
            //! it cannot start; the export has then failed.
            DJV_MODELS_API void start();

            //! Ask for frames, render the ones that have arrived, and see
            //! whether the writing is done. The callback makes the graphics
            //! context current before anything is rendered.
            DJV_MODELS_API void tick(const std::function<void()>& makeCurrent);

            //! Stop, leaving what has been written.
            DJV_MODELS_API void cancel();

        private:
            void _stop();
            void _release();
            void _writeThread();
            void _writeAudio(int64_t framesWritten);

            FTK_PRIVATE();
        };

        //! The exports, run one at a time in the order they were asked for.
        //! One at a time is the point: each encoder has memory of its own,
        //! gigabytes for a long GOP codec at 4K.
        class DJV_MODELS_API_TYPE ExportQueue : public std::enable_shared_from_this<ExportQueue>
        {
            FTK_NON_COPYABLE(ExportQueue);

        protected:
            void _init(const std::shared_ptr<ftk::Context>&);

            ExportQueue();

        public:
            DJV_MODELS_API ~ExportQueue();

            DJV_MODELS_API static std::shared_ptr<ExportQueue> create(
                const std::shared_ptr<ftk::Context>&);

            //! Set what makes the graphics context the exports render with
            //! current: the main window's.
            DJV_MODELS_API void setMakeCurrent(const std::function<void()>&);

            //! Add an export. The callback is told once whether it was
            //! written, including when it could not start or was cancelled.
            DJV_MODELS_API std::shared_ptr<ExportJob> add(
                const ExportJobOptions&,
                const std::function<void(bool)>& = nullptr);

            //! Cancel an export, queued or running.
            DJV_MODELS_API void cancel(const std::shared_ptr<ExportJob>&);

            //! Cancel every export.
            DJV_MODELS_API void cancelAll();

            //! Remove an export that has stopped from the list.
            DJV_MODELS_API void remove(const std::shared_ptr<ExportJob>&);

            //! Remove every export that has stopped.
            DJV_MODELS_API void clearDone();

            //! Observe the exports, in the order they run.
            DJV_MODELS_API std::shared_ptr<ftk::IObservableList<std::shared_ptr<ExportJob> > > observeJobs() const;

            //! Observe whether any export is queued or running.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<bool> > observeBusy() const;

            //! Whether an export that has not stopped writes to a path.
            DJV_MODELS_API bool isWriting(const ftk::Path&) const;

        private:
            void _tick();
            void _finish(const std::shared_ptr<ExportJob>&);
            void _busyUpdate();

            FTK_PRIVATE();
        };

        ///@}
    }
}
