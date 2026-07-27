// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/FileFilter.h>

#include <ftk/Core/String.h>

#include <algorithm>
#include <filesystem>
#include <regex>
#include <set>

namespace djv
{
    namespace models
    {
        namespace
        {
            bool regexSearch(const std::string& text, const std::string& pattern)
            {
                try
                {
                    return std::regex_search(
                        text,
                        std::regex(pattern, std::regex_constants::icase));
                }
                catch (const std::regex_error&)
                {
                    return ftk::contains(text, pattern, ftk::CaseCompare::Insensitive);
                }
            }

            std::string getFilterText(const ftk::Path& path, FileFilterTarget target)
            {
                switch (target)
                {
                case FileFilterTarget::Name:
                    return path.getFileName();
                case FileFilterTarget::Directory:
                    return path.getDir();
                case FileFilterTarget::Path:
                default:
                    return path.get();
                }
            }
        }

        std::vector<std::string> getDefaultFileFilterPresets()
        {
            return
            {
                "name:(exr|dpx|mov|mp4)$",
                "-dir:(cache|tmp|temp)",
                "-name:(proxy|thumb|thumbnail)",
                "dir:(shot010|shot020)",
                "name:beauty -name:proxy"
            };
        }

        std::vector<FileFilterTerm> parseFileFilter(const std::string& text)
        {
            std::vector<FileFilterTerm> out;
            for (auto token : ftk::split(text, { ' ', '\t' }))
            {
                if (token.empty())
                    continue;

                FileFilterTerm term;
                if ('-' == token.front() || '!' == token.front())
                {
                    term.include = false;
                    token.erase(token.begin());
                }
                else if ('+' == token.front())
                {
                    token.erase(token.begin());
                }
                if (token.empty())
                    continue;

                const size_t colon = token.find(':');
                if (colon != std::string::npos)
                {
                    const std::string key = ftk::toLower(token.substr(0, colon));
                    if ("name" == key || "file" == key)
                    {
                        term.target = FileFilterTarget::Name;
                        token = token.substr(colon + 1);
                    }
                    else if ("dir" == key || "folder" == key)
                    {
                        term.target = FileFilterTarget::Directory;
                        token = token.substr(colon + 1);
                    }
                    else if ("path" == key)
                    {
                        term.target = FileFilterTarget::Path;
                        token = token.substr(colon + 1);
                    }
                }
                if (!token.empty())
                {
                    term.pattern = token;
                    out.push_back(term);
                }
            }
            return out;
        }

        bool matchFileFilter(const ftk::Path& path, const std::string& text)
        {
            const auto terms = parseFileFilter(text);
            if (terms.empty())
                return true;

            for (const auto& term : terms)
            {
                const bool match = regexSearch(
                    getFilterText(path, term.target),
                    term.pattern);
                if (term.include && !match)
                {
                    return false;
                }
                if (!term.include && match)
                {
                    return false;
                }
            }
            return true;
        }

        bool pruneDirectoryByFileFilter(const ftk::Path& path, const std::string& text)
        {
            const auto terms = parseFileFilter(text);
            for (const auto& term : terms)
            {
                if (!term.include &&
                    (FileFilterTarget::Directory == term.target ||
                     FileFilterTarget::Path == term.target) &&
                    regexSearch(path.get(), term.pattern))
                {
                    return true;
                }
            }
            return false;
        }

        FileScanResult scanFiles(
            const ftk::Path& folder,
            const std::string& filter,
            const std::vector<std::string>& extensions,
            const FileScanOptions& options)
        {
            FileScanResult out;
            const std::filesystem::path root =
                std::filesystem::u8path(folder.get());
            std::error_code errorCode;
            if (!std::filesystem::is_directory(root, errorCode))
            {
                out.error = "The selected playlist import folder is not available.";
                return out;
            }

            std::set<std::string> normalizedExtensions;
            for (auto extension : extensions)
            {
                extension = ftk::toLower(extension);
                if (!extension.empty() && extension.front() != '.')
                {
                    extension.insert(extension.begin(), '.');
                }
                normalizedExtensions.insert(extension);
            }

            const auto directoryOptions =
                std::filesystem::directory_options::skip_permission_denied;
            std::filesystem::recursive_directory_iterator i(
                root,
                directoryOptions,
                errorCode);
            const std::filesystem::recursive_directory_iterator end;
            while (i != end)
            {
                if (errorCode)
                {
                    errorCode.clear();
                    i.increment(errorCode);
                    continue;
                }
                if (out.entriesVisited >= options.maxEntries ||
                    out.paths.size() >= options.maxResults)
                {
                    out.truncated = true;
                    break;
                }
                ++out.entriesVisited;

                const auto& entry = *i;
                const ftk::Path path(entry.path().u8string());
                if (entry.is_directory(errorCode))
                {
                    const bool isSymlink = entry.is_symlink(errorCode);
                    if (isSymlink ||
                        i.depth() >= static_cast<int>(options.maxDepth) ||
                        pruneDirectoryByFileFilter(path, filter))
                    {
                        i.disable_recursion_pending();
                    }
                }
                else if (entry.is_regular_file(errorCode))
                {
                    const std::string extension =
                        ftk::toLower(entry.path().extension().u8string());
                    if ((normalizedExtensions.empty() ||
                         normalizedExtensions.count(extension)) &&
                        matchFileFilter(path, filter))
                    {
                        out.paths.push_back(path);
                    }
                }
                errorCode.clear();
                i.increment(errorCode);
            }
            std::sort(
                out.paths.begin(),
                out.paths.end(),
                [](const ftk::Path& a, const ftk::Path& b)
                {
                    return a.get() < b.get();
                });
            return out;
        }
    }
}
