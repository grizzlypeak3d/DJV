// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/FolderScanner.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <string>

namespace djv
{
    namespace app
    {
        //! High-level outcome for an asynchronous folder scan request.
        enum class FolderScanServiceStatus
        {
            Accepted,
            Cancelled,
            Rejected,
            Failed
        };

        //! Typed service errors. The lower-level scan status and warnings are
        //! retained in FolderScanServiceResult::scanResult.
        enum class FolderScanServiceError
        {
            None,
            Cancelled,
            ShuttingDown,
            QueueFull,
            InvalidRequest,
            InvalidRoot,
            LimitReached,
            InternalError
        };

        //! Result returned by a folder scan request.
        //!
        //! Paths are exposed only for a completed scan. Cancellation and
        //! candidate/result limits are atomic and always return an empty path
        //! list, while preserving counters and warnings for diagnostics.
        struct FolderScanServiceResult
        {
            uint64_t requestId = 0;
            std::filesystem::path root;
            FolderScanServiceStatus status = FolderScanServiceStatus::Rejected;
            FolderScanServiceError error = FolderScanServiceError::InvalidRequest;
            std::string message;
            models::FolderScanResult scanResult;

            bool isSuccess() const;
        };

        //! Immediate handle for a folder scan request.
        struct FolderScanServiceRequest
        {
            uint64_t id = 0;
            std::shared_future<FolderScanServiceResult> future;

            explicit operator bool() const;
        };

        //! Truthful state for an active request. Completed requests are not
        //! retained by the service and therefore return std::nullopt.
        enum class FolderScanServiceRequestState
        {
            Queued,
            Running
        };

        //! Data-only worker gate used by deterministic stress tests. No user
        //! callback is ever invoked from the service worker.
        class FolderScanServiceStartGate
        {
        public:
            explicit FolderScanServiceStartGate(bool blocked = true);
            ~FolderScanServiceStartGate();

            FolderScanServiceStartGate(const FolderScanServiceStartGate&) = delete;
            FolderScanServiceStartGate& operator = (const FolderScanServiceStartGate&) = delete;

            void block();
            void release();
            bool isBlocked() const;
            bool waitUntilEntered(std::chrono::milliseconds) const;

            //! Worker-side bounded wait step. Public solely to keep the gate a
            //! passive synchronization primitive rather than a callback.
            bool waitForRelease(std::chrono::milliseconds);

        private:
            struct Private;
            std::unique_ptr<Private> _p;
        };

        //! Bounded configuration for FolderScanService.
        struct FolderScanServiceOptions
        {
            //! Maximum active requests, including the running request.
            size_t maxPendingRequests = 16;

            //! Per-request hard limits accepted by this service.
            size_t maxCandidatesPerRequest = 1000000;
            size_t maxDirectoryEntriesPerRequest = 250000;
            size_t maxDirectoriesPerRequest = 100000;
            size_t maxEntriesPerRequest = 1000000;
            size_t maxResultsPerRequest = 5000;
            size_t maxDepthPerRequest = 128;
            size_t maxWarningsPerRequest = 1000;
            size_t maxExtensionsPerRequest = 4096;
            size_t maxExtensionLength = 32;
            size_t maxSequenceDigits = 18;

            //! Optional passive start gate for deterministic tests.
            std::shared_ptr<FolderScanServiceStartGate> startGate;
        };

        //! Snapshot of service-level counters.
        struct FolderScanServiceStats
        {
            uint64_t requestsSubmitted = 0;
            uint64_t requestsCompleted = 0;
            uint64_t requestsCancelled = 0;
            uint64_t requestsRejected = 0;
            uint64_t requestsFailed = 0;
        };

        //! One-worker, bounded asynchronous wrapper around models::scanFolder().
        //!
        //! Cancellation is observed at directory and entry checkpoints. On
        //! Windows the scanner also cancels synchronous filesystem I/O on its
        //! owned worker thread, so a blocked remote enumeration can unwind.
        //! No partial paths are published and shutdown always joins the worker.
        class FolderScanService
        {
        public:
            explicit FolderScanService(
                const FolderScanServiceOptions& = FolderScanServiceOptions());
            ~FolderScanService();

            FolderScanService(const FolderScanService&) = delete;
            FolderScanService& operator = (const FolderScanService&) = delete;

            //! Validate and queue a scan. The filter is shared immutably for
            //! the duration of the request.
            FolderScanServiceRequest request(
                const std::filesystem::path& root,
                const std::shared_ptr<const models::CompiledFileFilter>& filter,
                const models::FolderScanOptions& = models::FolderScanOptions());

            //! Cancel a queued or running request. Its future is completed
            //! immediately and its queue capacity is released immediately.
            bool cancel(uint64_t requestId);

            //! Poll the state of an active request only.
            std::optional<FolderScanServiceRequestState> getState(
                uint64_t requestId) const;

            //! Refuse new work, cancel all active requests, and join the owned
            //! worker. Safe to call repeatedly.
            void shutdown();

            FolderScanServiceStats getStats() const;

        private:
            struct Private;
            std::unique_ptr<Private> _p;
        };
    }
}
