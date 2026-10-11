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

                // Parse recognized, unambiguous field labels independently
                // of the media's currently available rows. Terms like
                // "source channels" must remain separate: "Source Channels"
                // can coexist with "Source Format" and "Channels".
                // The exact phrase syntax is pending maintainer feedback.
                static const std::vector<std::string> phraseLabels = {
                    "Pixel Aspect Ratio", "Source Format",
                    "Start Time", "Sample Rate" };
                std::vector<std::string> remaining = words;
                std::vector<std::string> terms;
                for (const auto& label : phraseLabels)
                {
                    // Consume a full field label as an unordered collection
                    // of words. If any word is missing, keep all tokens
                    // available for another label or independent matching.
                    std::istringstream labelInput(label);
                    std::vector<std::string> unmatched = remaining;
                    bool complete = true;
                    for (std::string labelWord; labelInput >> labelWord;)
                    {
                        const auto i = std::find_if(
                            unmatched.begin(), unmatched.end(),
                            [&labelWord](const std::string& word)
                            {
                                return word.size() == labelWord.size() &&
                                    ftk::contains(
                                        labelWord, word,
                                        ftk::CaseCompare::Insensitive);
                            });
                        if (i == unmatched.end())
                        {
                            complete = false;
                            break;
                        }
                        unmatched.erase(i);
                    }
                    if (complete)
                    {
                        terms.push_back(label);
                        remaining.swap(unmatched);
                    }
                }
                terms.insert(terms.end(), remaining.begin(), remaining.end());

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
