// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/FolderScanService.h>

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace djv
{
    namespace app
    {
        bool FolderScanServiceResult::isSuccess() const
        {
            return FolderScanServiceStatus::Accepted == status &&
                FolderScanServiceError::None == error &&
                models::FolderScanStatus::Completed == scanResult.status;
        }

        FolderScanServiceRequest::operator bool() const
        {
            return 0 != id && future.valid();
        }

        struct FolderScanServiceStartGate::Private
        {
            mutable std::mutex mutex;
            mutable std::condition_variable cv;
            bool blocked = true;
            bool entered = false;
        };

        FolderScanServiceStartGate::FolderScanServiceStartGate(bool blocked) :
            _p(new Private)
        {
            _p->blocked = blocked;
        }

        FolderScanServiceStartGate::~FolderScanServiceStartGate() = default;

        void FolderScanServiceStartGate::block()
        {
            std::lock_guard<std::mutex> lock(_p->mutex);
            _p->blocked = true;
            _p->entered = false;
        }

        void FolderScanServiceStartGate::release()
        {
            {
                std::lock_guard<std::mutex> lock(_p->mutex);
                _p->blocked = false;
            }
            _p->cv.notify_all();
        }

        bool FolderScanServiceStartGate::isBlocked() const
        {
            std::lock_guard<std::mutex> lock(_p->mutex);
            return _p->blocked;
        }

        bool FolderScanServiceStartGate::waitUntilEntered(
            std::chrono::milliseconds timeout) const
        {
            std::unique_lock<std::mutex> lock(_p->mutex);
            return _p->cv.wait_for(
                lock,
                timeout,
                [this] { return _p->entered; });
        }

        bool FolderScanServiceStartGate::waitForRelease(
            std::chrono::milliseconds timeout)
        {
            std::unique_lock<std::mutex> lock(_p->mutex);
            _p->entered = true;
            _p->cv.notify_all();
            return _p->cv.wait_for(
                lock,
                timeout,
                [this] { return !_p->blocked; });
        }

        namespace
        {
            FolderScanServiceResult makeImmediateResult(
                uint64_t requestId,
                const std::filesystem::path& root,
                FolderScanServiceStatus status,
                FolderScanServiceError error,
                const std::string& message)
            {
                FolderScanServiceResult out;
                out.requestId = requestId;
                out.root = root;
                out.status = status;
                out.error = error;
                out.message = message;
                return out;
            }

            bool validateExtensionList(
                const std::vector<std::string>& values,
                const FolderScanServiceOptions& limits)
            {
                if (values.size() > limits.maxExtensionsPerRequest)
                {
                    return false;
                }
                return std::all_of(
                    values.begin(),
                    values.end(),
                    [&limits](const std::string& value)
                    {
                        return value.size() <= limits.maxExtensionLength;
                    });
            }

            bool validateRequest(
                const std::filesystem::path& root,
                const std::shared_ptr<const models::CompiledFileFilter>& filter,
                const models::FolderScanOptions& options,
                const FolderScanServiceOptions& limits,
                std::string& message)
            {
                if (root.empty() || !root.is_absolute())
                {
                    message = "The folder scan root must be an absolute path.";
                    return false;
                }
                if (!filter)
                {
                    message = "The folder scan requires a compiled immutable filter.";
                    return false;
                }
                if (options.maxCandidates > limits.maxCandidatesPerRequest ||
                    options.maxDirectoryEntries > limits.maxDirectoryEntriesPerRequest ||
                    0 == options.maxDirectories ||
                    options.maxDirectories > limits.maxDirectoriesPerRequest ||
                    0 == options.maxEntries ||
                    options.maxEntries > limits.maxEntriesPerRequest ||
                    0 == options.maxResults ||
                    options.maxResults > limits.maxResultsPerRequest ||
                    options.maxDepth > limits.maxDepthPerRequest ||
                    options.maxWarnings > limits.maxWarningsPerRequest ||
                    0 == options.sequenceMaxDigits ||
                    options.sequenceMaxDigits > limits.maxSequenceDigits ||
                    !validateExtensionList(options.fileExtensions, limits) ||
                    !validateExtensionList(options.sequenceExtensions, limits))
                {
                    message = "The folder scan options exceed the service's bounded limits.";
                    return false;
                }
                return true;
            }

            FolderScanServiceResult classifyScanResult(
                uint64_t requestId,
                const std::filesystem::path& root,
                models::FolderScanResult scanResult)
            {
                FolderScanServiceResult out;
                out.requestId = requestId;
                out.root = root;
                out.scanResult = std::move(scanResult);
                switch (out.scanResult.status)
                {
                case models::FolderScanStatus::Completed:
                    out.status = FolderScanServiceStatus::Accepted;
                    out.error = FolderScanServiceError::None;
                    out.message = "Folder scan completed.";
                    break;
                case models::FolderScanStatus::Cancelled:
                    out.scanResult.paths.clear();
                    out.status = FolderScanServiceStatus::Cancelled;
                    out.error = FolderScanServiceError::Cancelled;
                    out.message = "Folder scan was cancelled.";
                    break;
                case models::FolderScanStatus::CandidateLimitReached:
                case models::FolderScanStatus::TraversalLimitReached:
                case models::FolderScanStatus::ResultLimitReached:
                    out.scanResult.paths.clear();
                    out.status = FolderScanServiceStatus::Failed;
                    out.error = FolderScanServiceError::LimitReached;
                    out.message = "Folder scan stopped at a configured limit; no partial paths were published.";
                    break;
                case models::FolderScanStatus::InvalidRoot:
                    out.scanResult.paths.clear();
                    out.status = FolderScanServiceStatus::Failed;
                    out.error = FolderScanServiceError::InvalidRoot;
                    out.message = "The folder scan root is not an accessible directory.";
                    break;
                default:
                    out.scanResult.paths.clear();
                    out.status = FolderScanServiceStatus::Failed;
                    out.error = FolderScanServiceError::InternalError;
                    out.message = "Folder scan returned an unknown status.";
                    break;
                }
                return out;
            }
        }

        struct FolderScanService::Private
        {
            struct RequestState
            {
                uint64_t id = 0;
                std::filesystem::path root;
                std::shared_ptr<const models::CompiledFileFilter> filter;
                models::FolderScanOptions options;
                models::FolderScanCancellationSource cancellation;
                std::promise<FolderScanServiceResult> promise;
                std::shared_future<FolderScanServiceResult> future;
                bool running = false;
                bool done = false;
            };

            explicit Private(const FolderScanServiceOptions& value) :
                options(value)
            {
                if (0 == options.maxPendingRequests ||
                    0 == options.maxCandidatesPerRequest ||
                    0 == options.maxDirectoryEntriesPerRequest ||
                    0 == options.maxDirectoriesPerRequest ||
                    0 == options.maxEntriesPerRequest ||
                    0 == options.maxResultsPerRequest ||
                    0 == options.maxExtensionsPerRequest ||
                    0 == options.maxExtensionLength ||
                    0 == options.maxSequenceDigits)
                {
                    throw std::invalid_argument(
                        "FolderScanService requires non-zero hard limits.");
                }
                worker = std::thread([this] { run(); });
            }

            FolderScanServiceResult cancelledResult(
                const std::shared_ptr<RequestState>& request,
                FolderScanServiceError error) const
            {
                FolderScanServiceResult out = makeImmediateResult(
                    request->id,
                    request->root,
                    FolderScanServiceStatus::Cancelled,
                    error,
                    FolderScanServiceError::ShuttingDown == error ?
                        "Folder scan stopped because the service is shutting down." :
                        "Folder scan request was cancelled.");
                out.scanResult.status = models::FolderScanStatus::Cancelled;
                return out;
            }

            void accountResult(const FolderScanServiceResult& result)
            {
                ++stats.requestsCompleted;
                switch (result.status)
                {
                case FolderScanServiceStatus::Cancelled:
                    ++stats.requestsCancelled;
                    break;
                case FolderScanServiceStatus::Rejected:
                    ++stats.requestsRejected;
                    break;
                case FolderScanServiceStatus::Failed:
                    ++stats.requestsFailed;
                    break;
                default:
                    break;
                }
            }

            void complete(
                const std::shared_ptr<RequestState>& request,
                FolderScanServiceResult result)
            {
                bool publish = false;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    if (!request->done)
                    {
                        request->done = true;
                        requests.erase(request->id);
                        accountResult(result);
                        publish = true;
                    }
                }
                if (publish)
                {
                    request->promise.set_value(std::move(result));
                }
            }

            FolderScanServiceResult execute(
                const std::shared_ptr<RequestState>& request)
            {
                const auto token = request->cancellation.getToken();
                if (options.startGate)
                {
                    while (!options.startGate->waitForRelease(
                        std::chrono::milliseconds(5)))
                    {
                        if (token.isCancellationRequested())
                        {
                            return cancelledResult(
                                request,
                                FolderScanServiceError::Cancelled);
                        }
                    }
                }
                if (token.isCancellationRequested())
                {
                    return cancelledResult(request, FolderScanServiceError::Cancelled);
                }

                try
                {
                    return classifyScanResult(
                        request->id,
                        request->root,
                        models::scanFolder(
                            request->root,
                            *request->filter,
                            request->options,
                            token));
                }
                catch (const std::exception& e)
                {
                    return makeImmediateResult(
                        request->id,
                        request->root,
                        FolderScanServiceStatus::Failed,
                        FolderScanServiceError::InternalError,
                        std::string("Unexpected folder scan failure: ") + e.what());
                }
                catch (...)
                {
                    return makeImmediateResult(
                        request->id,
                        request->root,
                        FolderScanServiceStatus::Failed,
                        FolderScanServiceError::InternalError,
                        "Unexpected folder scan failure.");
                }
            }

            void run()
            {
                while (true)
                {
                    std::shared_ptr<RequestState> request;
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        cv.wait(lock, [this] { return stopping || !queue.empty(); });
                        if (queue.empty())
                        {
                            if (stopping)
                            {
                                return;
                            }
                            continue;
                        }
                        request = queue.front();
                        queue.pop_front();
                        if (request->done)
                        {
                            continue;
                        }
                        request->running = true;
                    }

                    complete(request, execute(request));
                }
            }

            FolderScanServiceOptions options;
            mutable std::mutex mutex;
            std::mutex shutdownMutex;
            std::condition_variable cv;
            std::deque<std::shared_ptr<RequestState> > queue;
            std::unordered_map<uint64_t, std::shared_ptr<RequestState> > requests;
            uint64_t nextRequestId = 1;
            bool accepting = true;
            bool stopping = false;
            std::thread worker;
            FolderScanServiceStats stats;
        };

        FolderScanService::FolderScanService(
            const FolderScanServiceOptions& options) :
            _p(new Private(options))
        {}

        FolderScanService::~FolderScanService()
        {
            shutdown();
        }

        FolderScanServiceRequest FolderScanService::request(
            const std::filesystem::path& root,
            const std::shared_ptr<const models::CompiledFileFilter>& filter,
            const models::FolderScanOptions& options)
        {
            const auto request = std::make_shared<Private::RequestState>();
            request->root = root;
            request->filter = filter;
            request->options = options;
            request->future = request->promise.get_future().share();

            FolderScanServiceResult immediate;
            bool hasImmediate = false;
            {
                std::lock_guard<std::mutex> lock(_p->mutex);
                request->id = _p->nextRequestId++;
                ++_p->stats.requestsSubmitted;

                if (!_p->accepting)
                {
                    immediate = makeImmediateResult(
                        request->id,
                        root,
                        FolderScanServiceStatus::Rejected,
                        FolderScanServiceError::ShuttingDown,
                        "Folder scan service is not accepting requests.");
                    hasImmediate = true;
                }
                else
                {
                    std::string validationMessage;
                    if (!validateRequest(
                        root, filter, options, _p->options, validationMessage))
                    {
                        immediate = makeImmediateResult(
                            request->id,
                            root,
                            FolderScanServiceStatus::Rejected,
                            FolderScanServiceError::InvalidRequest,
                            validationMessage);
                        hasImmediate = true;
                    }
                }
                if (!hasImmediate &&
                    _p->requests.size() >= _p->options.maxPendingRequests)
                {
                    immediate = makeImmediateResult(
                        request->id,
                        root,
                        FolderScanServiceStatus::Rejected,
                        FolderScanServiceError::QueueFull,
                        "Folder scan request queue is full.");
                    hasImmediate = true;
                }

                if (hasImmediate)
                {
                    request->done = true;
                    _p->accountResult(immediate);
                }
                else
                {
                    _p->requests[request->id] = request;
                    _p->queue.push_back(request);
                }
            }

            if (hasImmediate)
            {
                request->promise.set_value(std::move(immediate));
            }
            else
            {
                _p->cv.notify_one();
            }
            return { request->id, request->future };
        }

        bool FolderScanService::cancel(uint64_t requestId)
        {
            std::shared_ptr<Private::RequestState> request;
            FolderScanServiceResult result;
            {
                std::lock_guard<std::mutex> lock(_p->mutex);
                const auto i = _p->requests.find(requestId);
                if (i == _p->requests.end() || !i->second || i->second->done)
                {
                    return false;
                }
                request = i->second;
                request->cancellation.cancel();
                request->done = true;
                _p->requests.erase(i);
                if (!request->running)
                {
                    const auto queueIt = std::find(
                        _p->queue.begin(), _p->queue.end(), request);
                    if (queueIt != _p->queue.end())
                    {
                        _p->queue.erase(queueIt);
                    }
                }
                result = _p->cancelledResult(
                    request, FolderScanServiceError::Cancelled);
                _p->accountResult(result);
            }
            request->promise.set_value(std::move(result));
            _p->cv.notify_all();
            return true;
        }

        std::optional<FolderScanServiceRequestState> FolderScanService::getState(
            uint64_t requestId) const
        {
            std::lock_guard<std::mutex> lock(_p->mutex);
            const auto i = _p->requests.find(requestId);
            if (i == _p->requests.end() || !i->second || i->second->done)
            {
                return std::nullopt;
            }
            return i->second->running ?
                FolderScanServiceRequestState::Running :
                FolderScanServiceRequestState::Queued;
        }

        void FolderScanService::shutdown()
        {
            std::lock_guard<std::mutex> shutdownLock(_p->shutdownMutex);
            std::vector<std::pair<
                std::shared_ptr<Private::RequestState>,
                FolderScanServiceResult> > completions;
            {
                std::lock_guard<std::mutex> lock(_p->mutex);
                if (_p->accepting || !_p->stopping)
                {
                    _p->accepting = false;
                    _p->stopping = true;
                    completions.reserve(_p->requests.size());
                    for (const auto& i : _p->requests)
                    {
                        const auto& request = i.second;
                        if (!request || request->done)
                        {
                            continue;
                        }
                        request->cancellation.cancel();
                        request->done = true;
                        FolderScanServiceResult result = _p->cancelledResult(
                            request, FolderScanServiceError::ShuttingDown);
                        _p->accountResult(result);
                        completions.emplace_back(request, std::move(result));
                    }
                    _p->requests.clear();
                    _p->queue.clear();
                }
            }
            for (auto& completion : completions)
            {
                completion.first->promise.set_value(std::move(completion.second));
            }
            _p->cv.notify_all();
            if (_p->worker.joinable())
            {
                // The worker never invokes user code or any service method, so
                // ownership cannot return to shutdown() on the worker thread.
                if (_p->worker.get_id() == std::this_thread::get_id())
                {
                    std::terminate();
                }
                _p->worker.join();
            }
        }

        FolderScanServiceStats FolderScanService::getStats() const
        {
            std::lock_guard<std::mutex> lock(_p->mutex);
            return _p->stats;
        }
    }
}
