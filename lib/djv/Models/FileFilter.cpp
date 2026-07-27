// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/FileFilter.h>

#include <algorithm>
#include <cctype>
#include <locale>
#include <regex>
#include <stdexcept>
#include <utility>

namespace djv
{
    namespace models
    {
        namespace
        {
            std::string toLowerASCII(const std::string& value)
            {
                std::string out = value;
                std::transform(
                    out.begin(),
                    out.end(),
                    out.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return out;
            }

            bool isSpace(char value)
            {
                return 0 != std::isspace(static_cast<unsigned char>(value));
            }

            std::optional<FileFilterTarget> parseTarget(const std::string& value)
            {
                const std::string target = toLowerASCII(value);
                if ("path" == target)
                {
                    return FileFilterTarget::Path;
                }
                if ("name" == target || "file" == target)
                {
                    return FileFilterTarget::Name;
                }
                if ("ext" == target || "extension" == target)
                {
                    return FileFilterTarget::Extension;
                }
                if ("dir" == target || "folder" == target)
                {
                    return FileFilterTarget::Directory;
                }
                return std::nullopt;
            }

            std::optional<std::string> validateRegexSafety(const std::string& pattern)
            {
                bool escaped = false;
                bool characterClass = false;
                size_t unboundedQuantifiers = 0;
                size_t quantifiers = 0;
                size_t alternations = 0;
                for (size_t i = 0; i < pattern.size(); ++i)
                {
                    const char value = pattern[i];
                    if (escaped)
                    {
                        if (value >= '1' && value <= '9')
                        {
                            return "Backreferences are not supported";
                        }
                        escaped = false;
                        continue;
                    }
                    if ('\\' == value)
                    {
                        escaped = true;
                        continue;
                    }
                    if (characterClass)
                    {
                        characterClass = ']' != value;
                        continue;
                    }
                    if ('[' == value)
                    {
                        characterClass = true;
                        continue;
                    }
                    if ('(' == value && i + 1 < pattern.size() && '?' == pattern[i + 1])
                    {
                        return "Lookaround and special groups are not supported";
                    }
                    if ('|' == value && ++alternations > 16)
                    {
                        return "Too many alternatives";
                    }
                    if (')' == value && i + 1 < pattern.size())
                    {
                        const char next = pattern[i + 1];
                        if ('*' == next || '+' == next || '?' == next || '{' == next)
                        {
                            return "Quantified groups are not supported";
                        }
                    }
                    if ('*' == value || '+' == value)
                    {
                        if (++quantifiers > 16)
                        {
                            return "Too many quantifiers";
                        }
                        ++unboundedQuantifiers;
                        if (unboundedQuantifiers > 2)
                        {
                            return "Too many unbounded quantifiers";
                        }
                        if (i > 0 &&
                            ('*' == pattern[i - 1] || '+' == pattern[i - 1] ||
                                '?' == pattern[i - 1] || '}' == pattern[i - 1]))
                        {
                            return "Nested quantifiers are not supported";
                        }
                    }
                    if ('?' == value)
                    {
                        if (++quantifiers > 16)
                        {
                            return "Too many quantifiers";
                        }
                        if (i > 0 &&
                            ('*' == pattern[i - 1] || '+' == pattern[i - 1] ||
                                '?' == pattern[i - 1] || '}' == pattern[i - 1]))
                        {
                            return "Nested quantifiers are not supported";
                        }
                    }
                    if ('{' == value)
                    {
                        if (++quantifiers > 16)
                        {
                            return "Too many quantifiers";
                        }
                        const size_t close = pattern.find('}', i + 1);
                        if (std::string::npos == close)
                        {
                            continue;
                        }
                        const std::string repetition = pattern.substr(i + 1, close - i - 1);
                        const size_t comma = repetition.find(',');
                        const std::string maximum = std::string::npos == comma ?
                            repetition : repetition.substr(comma + 1);
                        if (std::string::npos != comma && maximum.empty())
                        {
                            ++unboundedQuantifiers;
                            if (unboundedQuantifiers > 2)
                            {
                                return "Too many unbounded quantifiers";
                            }
                        }
                        else if (!maximum.empty())
                        {
                            try
                            {
                                if (std::stoull(maximum) > CompiledFileFilter::maxCandidateLength)
                                {
                                    return "Repetition exceeds the candidate length limit";
                                }
                            }
                            catch (const std::out_of_range&)
                            {
                                return "Repetition exceeds the candidate length limit";
                            }
                            catch (const std::invalid_argument&)
                            {
                                // std::regex reports malformed repetitions precisely.
                            }
                        }
                    }
                }
                return std::nullopt;
            }

            std::string getText(const ftk::Path& path, FileFilterTarget target)
            {
                switch (target)
                {
                case FileFilterTarget::Name:
                    return path.getFileName();
                case FileFilterTarget::Extension:
                {
                    std::string out = path.getExt();
                    if (!out.empty() && '.' == out.front())
                    {
                        out.erase(out.begin());
                    }
                    return out;
                }
                case FileFilterTarget::Directory:
                    return path.getDir();
                case FileFilterTarget::Path:
                default:
                    return path.get();
                }
            }

            bool matchDirectoryComponents(
                const std::string& value,
                const std::regex& regex)
            {
                size_t begin = 0;
                while (begin < value.size())
                {
                    while (begin < value.size() &&
                        ('/' == value[begin] || '\\' == value[begin]))
                    {
                        ++begin;
                    }
                    size_t end = begin;
                    while (end < value.size() &&
                        '/' != value[end] && '\\' != value[end])
                    {
                        ++end;
                    }
                    if (end > begin && std::regex_search(
                        value.begin() + begin,
                        value.begin() + end,
                        regex))
                    {
                        return true;
                    }
                    begin = end;
                }
                return false;
            }

            bool matchesTerm(
                const ftk::Path& path,
                FileFilterTarget target,
                const std::regex& regex)
            {
                return FileFilterTarget::Directory == target ?
                    matchDirectoryComponents(path.getDir(), regex) :
                    std::regex_search(getText(path, target), regex);
            }

            FileFilterCompileResult makeError(
                FileFilterErrorCode code,
                size_t offset,
                const std::string& token,
                const std::string& message)
            {
                FileFilterCompileResult out;
                out.error = FileFilterError{ code, offset, token, message };
                return out;
            }
        }

        struct CompiledFileFilter::Private
        {
            struct Term
            {
                FileFilterTerm info;
                std::regex regex;
            };

            std::string expression;
            std::vector<FileFilterTerm> termInfo;
            std::vector<Term> terms;
        };

        FileFilterCompileResult::operator bool() const
        {
            return static_cast<bool>(filter) && !error.has_value();
        }

        CompiledFileFilter::CompiledFileFilter() :
            _p(new Private)
        {}

        CompiledFileFilter::~CompiledFileFilter()
        {}

        bool CompiledFileFilter::matches(const ftk::Path& path) const
        {
            if (path.get().size() > maxCandidateLength)
            {
                return false;
            }
            for (const auto& term : _p->terms)
            {
                const bool match = matchesTerm(path, term.info.target, term.regex);
                if ((term.info.include && !match) || (!term.info.include && match))
                {
                    return false;
                }
            }
            return true;
        }

        bool CompiledFileFilter::excludesDirectory(const ftk::Path& path) const
        {
            if (path.get().size() > maxCandidateLength)
            {
                return false;
            }
            for (const auto& term : _p->terms)
            {
                if (!term.info.include &&
                    FileFilterTarget::Directory == term.info.target &&
                    matchDirectoryComponents(path.get(), term.regex))
                {
                    return true;
                }
            }
            return false;
        }

        bool CompiledFileFilter::isEmpty() const
        {
            return _p->terms.empty();
        }

        const std::string& CompiledFileFilter::getExpression() const
        {
            return _p->expression;
        }

        const std::vector<FileFilterTerm>& CompiledFileFilter::getTerms() const
        {
            return _p->termInfo;
        }

        FileFilterCompileResult compileFileFilter(const std::string& expression)
        {
            if (expression.size() > CompiledFileFilter::maxExpressionLength)
            {
                return makeError(
                    FileFilterErrorCode::ExpressionTooLong,
                    CompiledFileFilter::maxExpressionLength,
                    std::string(),
                    "File filter expression is too long");
            }

            auto filter = std::shared_ptr<CompiledFileFilter>(new CompiledFileFilter);
            filter->_p->expression = expression;
            size_t position = 0;
            while (position < expression.size())
            {
                while (position < expression.size() && isSpace(expression[position]))
                {
                    ++position;
                }
                if (position >= expression.size())
                {
                    break;
                }

                const size_t tokenOffset = position;
                while (position < expression.size() && !isSpace(expression[position]))
                {
                    ++position;
                }
                const std::string token = expression.substr(tokenOffset, position - tokenOffset);
                if (filter->_p->terms.size() >= CompiledFileFilter::maxTermCount)
                {
                    return makeError(
                        FileFilterErrorCode::TooManyTerms,
                        tokenOffset,
                        token,
                        "File filter contains too many terms");
                }

                FileFilterTerm info;
                size_t patternOffset = 0;
                if ('+' == token.front() || '-' == token.front() || '!' == token.front())
                {
                    info.include = '+' == token.front();
                    patternOffset = 1;
                }
                if (patternOffset >= token.size())
                {
                    return makeError(
                        FileFilterErrorCode::EmptyPattern,
                        tokenOffset + patternOffset,
                        token,
                        "File filter term has no pattern");
                }

                const size_t colon = token.find(':', patternOffset);
                if (std::string::npos != colon)
                {
                    const auto target = parseTarget(
                        token.substr(patternOffset, colon - patternOffset));
                    if (!target.has_value())
                    {
                        return makeError(
                            FileFilterErrorCode::UnknownTarget,
                            tokenOffset + patternOffset,
                            token,
                            "Unknown file filter target");
                    }
                    info.target = target.value();
                    patternOffset = colon + 1;
                }

                if (patternOffset >= token.size())
                {
                    return makeError(
                        FileFilterErrorCode::EmptyPattern,
                        tokenOffset + patternOffset,
                        token,
                        "File filter term has no pattern");
                }
                info.pattern = token.substr(patternOffset);
                if (info.pattern.size() > CompiledFileFilter::maxPatternLength)
                {
                    return makeError(
                        FileFilterErrorCode::PatternTooLong,
                        tokenOffset + patternOffset,
                        token,
                        "File filter pattern is too long");
                }
                if (const auto unsafe = validateRegexSafety(info.pattern))
                {
                    return makeError(
                        FileFilterErrorCode::UnsafeRegularExpression,
                        tokenOffset + patternOffset,
                        token,
                        unsafe.value());
                }

                try
                {
                    CompiledFileFilter::Private::Term term;
                    term.info = info;
                    term.regex.imbue(std::locale::classic());
                    term.regex.assign(
                        info.pattern,
                        std::regex_constants::ECMAScript |
                            std::regex_constants::icase |
                            std::regex_constants::optimize);
                    filter->_p->termInfo.push_back(info);
                    filter->_p->terms.push_back(std::move(term));
                }
                catch (const std::regex_error& e)
                {
                    return makeError(
                        FileFilterErrorCode::InvalidRegularExpression,
                        tokenOffset + patternOffset,
                        token,
                        std::string("Invalid regular expression: ") + e.what());
                }
            }

            FileFilterCompileResult out;
            out.filter = std::move(filter);
            return out;
        }

        std::vector<std::string> getDefaultFileFilterPresets()
        {
            return
            {
                "ext:^(mov|mp4|m4v|mxf|avi|mkv|webm)$",
                "ext:^(exr|dpx|tif|tiff|png|jpg|jpeg)$",
                "-dir:^(cache|tmp|temp|proxy)$",
                "-name:(proxy|thumb|thumbnail)",
                "name:beauty -name:proxy"
            };
        }
    }
}
