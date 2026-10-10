// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/Core/String.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace djv
{
    namespace ui
    {
        namespace detail
        {
            //! Existing case-insensitive single-query Information row filter.
            inline bool matchesInfoRow(
                const std::string& name,
                const std::string& value,
                const std::string& search)
            {
                return search.empty() ||
                    ftk::contains(name, search, ftk::CaseCompare::Insensitive) ||
                    ftk::contains(value, search, ftk::CaseCompare::Insensitive);
            }

            using InfoRows = std::vector<std::pair<std::string, std::string> >;
            using InfoSections = std::map<std::string, InfoRows>;

            //! Match a term in a field name/value or in the names used by
            //! #316's original example (Filename/Dimensions).
            inline bool matchesInfoTerm(
                const std::string& name,
                const std::string& value,
                const std::string& term)
            {
                return matchesInfoRow(name, value, term) ||
                    (name == "Name" && ftk::contains(
                        "Filename", term, ftk::CaseCompare::Insensitive)) ||
                    (name == "Resolution" && ftk::contains(
                        "Dimensions", term, ftk::CaseCompare::Insensitive));
            }

            //! Provisional panel-wide implied AND: every term must be found
            //! somewhere, then show the rows matching at least one term.
            //! Keep this separate so the maintainer can settle #316's
            //! cross-row behavior without changing the widget layout.
            inline InfoSections filterInfoSections(
                const InfoSections& sections,
                const std::vector<std::string>& sectionNames,
                const std::string& search)
            {
                InfoSections kept;
                for (const auto& name : sectionNames)
                {
                    kept[name];
                }

                std::istringstream input(search);
                std::vector<std::string> words;
                for (std::string word; input >> word;)
                {
                    words.push_back(word);
                }
                if (words.empty())
                {
                    for (const auto& name : sectionNames)
                    {
                        kept[name] = sections.at(name);
                    }
                    return kept;
                }

                // Use built-in multiword labels for phrase grouping. Basing
                // query parsing on metadata from the current media file
                // would change the meaning of the same search across files.
                static const std::vector<std::string> phraseLabels = {
                    "Source Format", "Pixel Type", "Pixel Aspect Ratio",
                    "Start Time", "Sample Rate", "Source Channels",
                    "Source Type", "Source Sample Rate" };
                std::vector<std::string> terms;
                for (size_t i = 0; i < words.size();)
                {
                    std::string candidate = words[i];
                    std::string longestLabel;
                    size_t longestSize = 0;
                    // Built-in labels contain at most three words.
                    for (size_t j = i + 1;
                        j < words.size() && j - i < 3; ++j)
                    {
                        candidate += " " + words[j];
                        bool matchesLabel = false;
                        for (const auto& label : phraseLabels)
                        {
                            if (ftk::contains(
                                label, candidate,
                                ftk::CaseCompare::Insensitive))
                            {
                                matchesLabel = true;
                                break;
                            }
                        }
                        if (matchesLabel)
                        {
                            longestLabel = candidate;
                            longestSize = j - i + 1;
                        }
                    }
                    if (longestSize > 0)
                    {
                        terms.push_back(longestLabel);
                        i += longestSize;
                    }
                    else
                    {
                        terms.push_back(words[i]);
                        ++i;
                    }
                }

                for (const auto& term : terms)
                {
                    bool found = false;
                    for (const auto& name : sectionNames)
                    {
                        for (const auto& row : sections.at(name))
                        {
                            if (matchesInfoTerm(row.first, row.second, term))
                            {
                                found = true;
                                break;
                            }
                        }
                        if (found)
                        {
                            break;
                        }
                    }
                    if (!found)
                    {
                        return kept;
                    }
                }

                for (const auto& name : sectionNames)
                {
                    for (const auto& row : sections.at(name))
                    {
                        if (std::any_of(terms.begin(), terms.end(),
                            [&row](const std::string& term)
                            {
                                return matchesInfoTerm(
                                    row.first, row.second, term);
                            }))
                        {
                            kept[name].push_back(row);
                        }
                    }
                }
                return kept;
            }
        }
    }
}
