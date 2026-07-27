// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/FileFilter.h>

#include <ftk/Core/Path.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace djv
{
    namespace models
    {
        //! Cancellation token shared safely between a scanner and its owner.
        class FolderScanCancellationToken
        {
        public:
            FolderScanCancellationToken() = default;

            bool isCancellationRequested() const;

            //! Bind/unbind the current synchronous I/O thread so cancellation
            //! can interrupt a blocked Windows filesystem request.
            void bindToCurrentThread() const;
            void unbindFromCurrentThread() const;

        private:
            struct State;
            explicit FolderScanCancellationToken(
                const std::shared_ptr<State>&);

            std::shared_ptr<State> _state;

            friend class FolderScanCancellationSource;
        };

        //! Owner of a folder scan cancellation token.
        class FolderScanCancellationSource
        {
        public:
            FolderScanCancellationSource();

            FolderScanCancellationToken getToken() const;
            void cancel() const;

        private:
            std::shared_ptr<FolderScanCancellationToken::State> _state;
        };

        enum class FolderScanStatus
        {
            Completed,
            Cancelled,
            CandidateLimitReached,
            TraversalLimitReached,
            ResultLimitReached,
            InvalidRoot
        };

        enum class FolderScanWarningCode
        {
            PermissionDenied,
            IOError,
            LinkSkipped,
            CandidateTooLong,
            CandidateLimitReached,
            TraversalLimitReached,
            ResultLimitReached,
            DepthLimitReached
        };

        struct FolderScanWarning
        {
            FolderScanWarningCode code = FolderScanWarningCode::IOError;
            std::filesystem::path path;
            std::error_code error;
            std::string message;
        };

        struct FolderScanOptions
        {
            bool recursive = true;
            bool followDirectoryLinks = false;
            bool collapseSequences = false;
            size_t sequenceMaxDigits = 9;
            size_t maxCandidates = 1000000;
            size_t maxDirectoryEntries = 250000;
            //! Global traversal limits across the complete request.
            size_t maxDirectories = 100000;
            size_t maxEntries = 1000000;
            //! Maximum final paths after optional sequence collapsing. Zero is unlimited.
            size_t maxResults = 5000;
            size_t maxDepth = 128;
            size_t maxWarnings = 1000;

            //! Empty accepts all extensions. Entries may include a leading dot.
            std::vector<std::string> fileExtensions;

            //! Extensions eligible for sequence collapsing. Only used when
            //! collapseSequences is true. Entries may include a leading dot.
            std::vector<std::string> sequenceExtensions;
        };

        struct FolderScanResult
        {
            FolderScanStatus status = FolderScanStatus::Completed;
            std::vector<ftk::Path> paths;
            std::vector<FolderScanWarning> warnings;
            size_t visitedDirectories = 0;
            size_t visitedEntries = 0;
            size_t visitedFiles = 0;
            size_t suppressedWarnings = 0;
        };

        //! Scan a directory synchronously. The caller may run this function on
        //! a worker thread and cancel it through the supplied token.
        FolderScanResult scanFolder(
            const std::filesystem::path& root,
            const CompiledFileFilter& filter,
            const FolderScanOptions& = FolderScanOptions(),
            const FolderScanCancellationToken& = FolderScanCancellationToken());
    }
}
