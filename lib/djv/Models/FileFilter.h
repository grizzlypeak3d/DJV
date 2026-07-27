// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/Core/Path.h>

#include <string>
#include <vector>

namespace djv
{
    namespace models
    {
        //! File filter target.
        enum class FileFilterTarget
        {
            Path,
            Name,
            Directory
        };

        //! File filter term.
        struct FileFilterTerm
        {
            bool include = true;
            FileFilterTarget target = FileFilterTarget::Path;
            std::string pattern;
        };

        //! Limits for a recursive folder scan.
        struct FileScanOptions
        {
            size_t maxDepth = 32;
            size_t maxEntries = 10000;
            size_t maxResults = 1000;
        };

        //! Result of a recursive folder scan.
        struct FileScanResult
        {
            std::vector<ftk::Path> paths;
            size_t entriesVisited = 0;
            bool truncated = false;
            std::string error;
        };

        //! Get default file filter presets.
        std::vector<std::string> getDefaultFileFilterPresets();

        //! Parse a file filter expression into terms.
        std::vector<FileFilterTerm> parseFileFilter(const std::string&);

        //! Match a file path against a filter expression.
        bool matchFileFilter(const ftk::Path&, const std::string&);

        //! Check whether a folder can be skipped by exclude terms.
        bool pruneDirectoryByFileFilter(const ftk::Path&, const std::string&);

        //! Scan a folder recursively with deterministic ordering and hard
        //! limits. Directory symlinks are not followed.
        FileScanResult scanFiles(
            const ftk::Path& folder,
            const std::string& filter,
            const std::vector<std::string>& extensions,
            const FileScanOptions& = FileScanOptions());
    }
}
