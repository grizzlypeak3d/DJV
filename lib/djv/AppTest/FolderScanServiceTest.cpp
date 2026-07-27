// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/AppTest/FolderScanServiceTest.h>

#include <djv/App/FolderScanService.h>
#include <djv/Models/FileFilter.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif
namespace djv
{
    namespace app_tests
    {
        namespace
        {
            void require(bool value, const char* expression, int line)
            {
                if (!value)
                {
                    std::ostringstream stream;
                    stream << "Requirement failed at line " << line << ": " << expression;
                    throw std::runtime_error(stream.str());
                }
            }

#define DJV_FOLDER_SCAN_REQUIRE(EXPRESSION) \
            require(!!(EXPRESSION), #EXPRESSION, __LINE__)

            class TempDirectory
            {
            public:
                explicit TempDirectory(const std::string& suffix)
                {
                    static std::atomic<uint64_t> nextId(1);
                    const auto clock = std::chrono::steady_clock::now().time_since_epoch().count();
                    _path = std::filesystem::temp_directory_path() /
                        ("djv-folder-scan-service-" + suffix + "-" +
                            std::to_string(clock) + "-" +
                            std::to_string(nextId.fetch_add(1)));
                    std::filesystem::create_directories(_path);
                    _path = std::filesystem::absolute(_path);
                }

                ~TempDirectory()
                {
                    std::error_code error;
                    std::filesystem::remove_all(_path, error);
                }

                const std::filesystem::path& path() const
                {
                    return _path;
                }

                void touch(const std::filesystem::path& relative) const
                {
                    const auto path = _path / relative;
                    std::filesystem::create_directories(path.parent_path());
                    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
                    stream << "x";
                    DJV_FOLDER_SCAN_REQUIRE(stream.good());
                }

            private:
                std::filesystem::path _path;
            };

            std::shared_ptr<const models::CompiledFileFilter> emptyFilter()
            {
                const auto result = models::compileFileFilter("");
                DJV_FOLDER_SCAN_REQUIRE(result);
                DJV_FOLDER_SCAN_REQUIRE(result.filter);
                return result.filter;
            }

            app::FolderScanServiceResult waitFor(
                const app::FolderScanServiceRequest& request,
                std::chrono::milliseconds timeout = std::chrono::seconds(5))
            {
                DJV_FOLDER_SCAN_REQUIRE(request);
                DJV_FOLDER_SCAN_REQUIRE(
                    std::future_status::ready == request.future.wait_for(timeout));
                return request.future.get();
            }

            void orderAndState()
            {
                TempDirectory firstRoot("order-a");
                TempDirectory secondRoot("order-b");
                firstRoot.touch("a.exr");
                secondRoot.touch("b.exr");
                const auto filter = emptyFilter();
                const auto gate = std::make_shared<app::FolderScanServiceStartGate>();
                app::FolderScanServiceOptions serviceOptions;
                serviceOptions.maxPendingRequests = 2;
                serviceOptions.startGate = gate;
                app::FolderScanService service(serviceOptions);

                const auto first = service.request(firstRoot.path(), filter);
                DJV_FOLDER_SCAN_REQUIRE(
                    gate->waitUntilEntered(std::chrono::seconds(2)));
                const auto second = service.request(secondRoot.path(), filter);
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceRequestState::Running ==
                        service.getState(first.id).value());
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceRequestState::Queued ==
                        service.getState(second.id).value());
                DJV_FOLDER_SCAN_REQUIRE(
                    std::future_status::timeout ==
                        second.future.wait_for(std::chrono::milliseconds(20)));

                gate->release();
                const auto firstResult = waitFor(first);
                const auto secondResult = waitFor(second);
                DJV_FOLDER_SCAN_REQUIRE(firstResult.isSuccess());
                DJV_FOLDER_SCAN_REQUIRE(secondResult.isSuccess());
                DJV_FOLDER_SCAN_REQUIRE(1 == firstResult.scanResult.paths.size());
                DJV_FOLDER_SCAN_REQUIRE(1 == secondResult.scanResult.paths.size());
                DJV_FOLDER_SCAN_REQUIRE(
                    0 != firstResult.scanResult.contentSignature);
                DJV_FOLDER_SCAN_REQUIRE(
                    0 != secondResult.scanResult.contentSignature);
                DJV_FOLDER_SCAN_REQUIRE(!service.getState(first.id));
                DJV_FOLDER_SCAN_REQUIRE(!service.getState(second.id));
            }

            void cancellationAndImmediateRerequest()
            {
                TempDirectory root("cancel");
                root.touch("a.exr");
                const auto filter = emptyFilter();
                const auto gate = std::make_shared<app::FolderScanServiceStartGate>();
                app::FolderScanServiceOptions options;
                options.maxPendingRequests = 1;
                options.startGate = gate;
                app::FolderScanService service(options);

                const auto first = service.request(root.path(), filter);
                DJV_FOLDER_SCAN_REQUIRE(
                    gate->waitUntilEntered(std::chrono::seconds(2)));
                DJV_FOLDER_SCAN_REQUIRE(service.cancel(first.id));
                DJV_FOLDER_SCAN_REQUIRE(!service.cancel(first.id));
                const auto cancelled = waitFor(first);
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceStatus::Cancelled == cancelled.status);
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceError::Cancelled == cancelled.error);
                DJV_FOLDER_SCAN_REQUIRE(cancelled.scanResult.paths.empty());

                // The cancelled running request no longer occupies bounded
                // queue capacity, even though its scanner checkpoint may still
                // be unwinding on the single worker.
                const auto second = service.request(root.path(), filter);
                DJV_FOLDER_SCAN_REQUIRE(second);
                DJV_FOLDER_SCAN_REQUIRE(
                    std::future_status::timeout ==
                        second.future.wait_for(std::chrono::milliseconds(20)));
                gate->release();
                DJV_FOLDER_SCAN_REQUIRE(waitFor(second).isSuccess());
            }

            void queueSaturation()
            {
                TempDirectory root("queue");
                root.touch("a.exr");
                const auto filter = emptyFilter();
                const auto gate = std::make_shared<app::FolderScanServiceStartGate>();
                app::FolderScanServiceOptions options;
                options.maxPendingRequests = 2;
                options.startGate = gate;
                app::FolderScanService service(options);

                const auto first = service.request(root.path(), filter);
                DJV_FOLDER_SCAN_REQUIRE(
                    gate->waitUntilEntered(std::chrono::seconds(2)));
                const auto second = service.request(root.path(), filter);
                const auto third = service.request(root.path(), filter);
                const auto rejected = waitFor(third);
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceStatus::Rejected == rejected.status);
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceError::QueueFull == rejected.error);
                gate->release();
                DJV_FOLDER_SCAN_REQUIRE(waitFor(first).isSuccess());
                DJV_FOLDER_SCAN_REQUIRE(waitFor(second).isSuccess());

                const auto stats = service.getStats();
                DJV_FOLDER_SCAN_REQUIRE(3 == stats.requestsSubmitted);
                DJV_FOLDER_SCAN_REQUIRE(3 == stats.requestsCompleted);
                DJV_FOLDER_SCAN_REQUIRE(1 == stats.requestsRejected);
            }

            void queuedCancellation()
            {
                TempDirectory root("queued-cancel");
                root.touch("a.exr");
                const auto filter = emptyFilter();
                const auto gate = std::make_shared<app::FolderScanServiceStartGate>();
                app::FolderScanServiceOptions options;
                options.maxPendingRequests = 2;
                options.startGate = gate;
                app::FolderScanService service(options);

                const auto running = service.request(root.path(), filter);
                DJV_FOLDER_SCAN_REQUIRE(
                    gate->waitUntilEntered(std::chrono::seconds(2)));
                const auto queued = service.request(root.path(), filter);
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceRequestState::Queued ==
                        service.getState(queued.id).value());
                DJV_FOLDER_SCAN_REQUIRE(service.cancel(queued.id));
                const auto cancelled = waitFor(queued);
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceStatus::Cancelled == cancelled.status);
                DJV_FOLDER_SCAN_REQUIRE(cancelled.scanResult.paths.empty());
                DJV_FOLDER_SCAN_REQUIRE(!service.getState(queued.id));

                gate->release();
                DJV_FOLDER_SCAN_REQUIRE(waitFor(running).isSuccess());
            }

            void shutdownAndDestruction()
            {
                TempDirectory root("shutdown");
                root.touch("a.exr");
                const auto filter = emptyFilter();
                const auto gate = std::make_shared<app::FolderScanServiceStartGate>();
                app::FolderScanServiceOptions options;
                options.startGate = gate;

                std::shared_future<app::FolderScanServiceResult> destroyedFuture;
                const auto start = std::chrono::steady_clock::now();
                {
                    app::FolderScanService service(options);
                    const auto request = service.request(root.path(), filter);
                    destroyedFuture = request.future;
                    DJV_FOLDER_SCAN_REQUIRE(
                        gate->waitUntilEntered(std::chrono::seconds(2)));
                }
                const auto elapsed = std::chrono::steady_clock::now() - start;
                DJV_FOLDER_SCAN_REQUIRE(elapsed < std::chrono::seconds(2));
                DJV_FOLDER_SCAN_REQUIRE(
                    std::future_status::ready ==
                        destroyedFuture.wait_for(std::chrono::seconds(1)));
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceError::ShuttingDown ==
                        destroyedFuture.get().error);

                gate->release();
                app::FolderScanService service;
                service.shutdown();
                service.shutdown();
                const auto rejected = waitFor(service.request(root.path(), filter));
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceStatus::Rejected == rejected.status);
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceError::ShuttingDown == rejected.error);
            }

            void validationAndAtomicLimits()
            {
                TempDirectory root("limits");
                root.touch("a.exr");
                root.touch("b.exr");
                root.touch("c.exr");
                const auto filter = emptyFilter();
                app::FolderScanService service;

                const auto relative = waitFor(service.request("relative", filter));
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceError::InvalidRequest == relative.error);
                const auto noFilter = waitFor(service.request(root.path(), nullptr));
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceError::InvalidRequest == noFilter.error);

                models::FolderScanOptions unbounded;
                unbounded.maxResults = 0;
                const auto unboundedResult = waitFor(
                    service.request(root.path(), filter, unbounded));
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceError::InvalidRequest == unboundedResult.error);

                models::FolderScanOptions resultLimit;
                resultLimit.maxResults = 2;
                const auto limited = waitFor(
                    service.request(root.path(), filter, resultLimit));
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceStatus::Failed == limited.status);
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceError::LimitReached == limited.error);
                DJV_FOLDER_SCAN_REQUIRE(
                    models::FolderScanStatus::ResultLimitReached ==
                        limited.scanResult.status);
                DJV_FOLDER_SCAN_REQUIRE(limited.scanResult.paths.empty());

                models::FolderScanOptions candidateLimit;
                candidateLimit.maxCandidates = 1;
                const auto candidateLimited = waitFor(
                    service.request(root.path(), filter, candidateLimit));
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceError::LimitReached == candidateLimited.error);
                DJV_FOLDER_SCAN_REQUIRE(candidateLimited.scanResult.paths.empty());

                std::filesystem::create_directories(root.path() / "empty-a");
                std::filesystem::create_directories(root.path() / "empty-b");
                models::FolderScanOptions traversalLimit;
                traversalLimit.maxDirectories = 2;
                const auto traversalLimited = waitFor(
                    service.request(root.path(), filter, traversalLimit));
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceError::LimitReached ==
                        traversalLimited.error);
                DJV_FOLDER_SCAN_REQUIRE(
                    models::FolderScanStatus::TraversalLimitReached ==
                        traversalLimited.scanResult.status);
                DJV_FOLDER_SCAN_REQUIRE(traversalLimited.scanResult.paths.empty());

                models::FolderScanOptions entryLimit;
                entryLimit.maxEntries = 2;
                const auto entryLimited = waitFor(
                    service.request(root.path(), filter, entryLimit));
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceError::LimitReached ==
                        entryLimited.error);
                DJV_FOLDER_SCAN_REQUIRE(2 == entryLimited.scanResult.visitedEntries);
                DJV_FOLDER_SCAN_REQUIRE(entryLimited.scanResult.paths.empty());

                models::FolderScanOptions complete;
                complete.maxResults = 3;
                const auto accepted = waitFor(
                    service.request(root.path(), filter, complete));
                DJV_FOLDER_SCAN_REQUIRE(accepted.isSuccess());
                DJV_FOLDER_SCAN_REQUIRE(3 == accepted.scanResult.paths.size());

                const auto missing = waitFor(service.request(
                    root.path() / "missing", filter));
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceStatus::Failed == missing.status);
                DJV_FOLDER_SCAN_REQUIRE(
                    app::FolderScanServiceError::InvalidRoot == missing.error);
            }

            void repeatedRuns()
            {
                const auto filter = emptyFilter();
                for (size_t iteration = 0; iteration < 20; ++iteration)
                {
                    TempDirectory root("repeat-" + std::to_string(iteration));
                    for (size_t i = 0; i < 32; ++i)
                    {
                        root.touch(
                            std::filesystem::path("shot") /
                            ("frame-" + std::to_string(i) + ".exr"));
                    }
                    app::FolderScanServiceOptions options;
                    options.maxPendingRequests = 4;
                    app::FolderScanService service(options);
                    std::vector<app::FolderScanServiceRequest> requests;
                    for (size_t i = 0; i < 4; ++i)
                    {
                        requests.push_back(service.request(root.path(), filter));
                    }
                    for (const auto& request : requests)
                    {
                        const auto result = waitFor(request);
                        DJV_FOLDER_SCAN_REQUIRE(result.isSuccess());
                        DJV_FOLDER_SCAN_REQUIRE(32 == result.scanResult.paths.size());
                    }
                }
            }

            void concurrentSubmissionStress()
            {
                const auto filter = emptyFilter();
                for (size_t iteration = 0; iteration < 10; ++iteration)
                {
                    TempDirectory root("concurrent-" + std::to_string(iteration));
                    for (size_t i = 0; i < 16; ++i)
                    {
                        root.touch("frame-" + std::to_string(i) + ".exr");
                    }

                    app::FolderScanServiceOptions options;
                    options.maxPendingRequests = 8;
                    app::FolderScanService service(options);
                    std::vector<app::FolderScanServiceRequest> requests(8);
                    std::vector<std::thread> submitters;
                    for (size_t i = 0; i < requests.size(); ++i)
                    {
                        submitters.emplace_back(
                            [&service, &requests, &root, &filter, i]
                            {
                                requests[i] = service.request(root.path(), filter);
                            });
                    }
                    for (auto& submitter : submitters)
                    {
                        submitter.join();
                    }

                    std::vector<uint64_t> ids;
                    for (const auto& request : requests)
                    {
                        const auto result = waitFor(request);
                        DJV_FOLDER_SCAN_REQUIRE(result.isSuccess());
                        DJV_FOLDER_SCAN_REQUIRE(16 == result.scanResult.paths.size());
                        ids.push_back(result.requestId);
                    }
                    std::sort(ids.begin(), ids.end());
                    DJV_FOLDER_SCAN_REQUIRE(
                        ids.end() == std::unique(ids.begin(), ids.end()));
                }
            }

            void largeDirectoryBenchmark()
            {
                TempDirectory root("benchmark-10k");
                for (size_t i = 0; i < 10000; ++i)
                {
                    std::ostringstream name;
                    name << "frame-" << std::setfill('0') << std::setw(5)
                         << i << ".exr";
                    root.touch(std::filesystem::path("shots") / name.str());
                }

                const auto compiled = models::compileFileFilter("ext:^exr$");
                DJV_FOLDER_SCAN_REQUIRE(compiled);
                app::FolderScanServiceOptions serviceOptions;
                serviceOptions.maxResultsPerRequest = 10000;
                app::FolderScanService service(serviceOptions);
                models::FolderScanOptions scanOptions;
                scanOptions.maxCandidates = 10001;
                scanOptions.maxResults = 10000;
                scanOptions.fileExtensions = { ".exr" };

                const auto submitStart = std::chrono::steady_clock::now();
                const auto request = service.request(
                    root.path(),
                    compiled.filter,
                    scanOptions);
                const auto submitElapsed =
                    std::chrono::steady_clock::now() - submitStart;
                DJV_FOLDER_SCAN_REQUIRE(
                    submitElapsed < std::chrono::milliseconds(50));

                const auto scanStart = std::chrono::steady_clock::now();
                const auto result = waitFor(request, std::chrono::seconds(30));
                const auto scanElapsed = std::chrono::duration_cast<
                    std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - scanStart);
                DJV_FOLDER_SCAN_REQUIRE(result.isSuccess());
                DJV_FOLDER_SCAN_REQUIRE(10000 == result.scanResult.paths.size());
                DJV_FOLDER_SCAN_REQUIRE(std::is_sorted(
                    result.scanResult.paths.begin(),
                    result.scanResult.paths.end(),
                    [](const ftk::Path& a, const ftk::Path& b)
                    {
                        return a.get() < b.get();
                    }));
                DJV_FOLDER_SCAN_REQUIRE(scanElapsed < std::chrono::seconds(10));
                std::cout << "FolderScanService 10k benchmark: "
                          << scanElapsed.count() << " ms\n";
            }

#if defined(_WIN32)
            void synchronousIoCancellation()
            {
                HANDLE readPipe = nullptr;
                HANDLE writePipe = nullptr;
                SECURITY_ATTRIBUTES attributes = {};
                attributes.nLength = sizeof(attributes);
                attributes.bInheritHandle = FALSE;
                DJV_FOLDER_SCAN_REQUIRE(CreatePipe(
                    &readPipe, &writePipe, &attributes, 0));

                models::FolderScanCancellationSource cancellation;
                const auto token = cancellation.getToken();
                std::atomic_bool entered(false);
                std::atomic<DWORD> error(ERROR_SUCCESS);
                std::thread reader([&]
                {
                    token.bindToCurrentThread();
                    entered.store(true, std::memory_order_release);
                    char byte = 0;
                    DWORD read = 0;
                    const BOOL ok = ReadFile(readPipe, &byte, 1, &read, nullptr);
                    if (!ok)
                    {
                        error.store(GetLastError(), std::memory_order_release);
                    }
                    token.unbindFromCurrentThread();
                });
                const auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::seconds(2);
                while (!entered.load(std::memory_order_acquire) &&
                    std::chrono::steady_clock::now() < deadline)
                {
                    std::this_thread::yield();
                }
                DJV_FOLDER_SCAN_REQUIRE(entered.load(std::memory_order_acquire));
                cancellation.cancel();
                reader.join();
                CloseHandle(readPipe);
                CloseHandle(writePipe);
                DJV_FOLDER_SCAN_REQUIRE(
                    ERROR_OPERATION_ABORTED == error.load(std::memory_order_acquire));
            }
#endif
        }

        int runFolderScanServiceTests()
        {
            try
            {
                orderAndState();
                cancellationAndImmediateRerequest();
                queueSaturation();
                queuedCancellation();
                shutdownAndDestruction();
                validationAndAtomicLimits();
                repeatedRuns();
                concurrentSubmissionStress();
                largeDirectoryBenchmark();
#if defined(_WIN32)
                synchronousIoCancellation();
#endif
                std::cout << "FolderScanServiceTest: PASS\n";
                return 0;
            }
            catch (const std::exception& e)
            {
                std::cerr << "FolderScanServiceTest: FAIL: " << e.what() << '\n';
                return 1;
            }
        }
    }
}
#if defined(DJV_FOLDER_SCAN_SERVICE_TEST_MAIN)
int main()
{
    return djv::app_tests::runFolderScanServiceTests();
}
#endif
