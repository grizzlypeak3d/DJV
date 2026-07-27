// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/Core/Path.h>

#include <cstddef>
#include <memory>
#include <optional>
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
            Extension,
            Directory
        };

        //! File filter compilation error.
        enum class FileFilterErrorCode
        {
            ExpressionTooLong,
            TooManyTerms,
            EmptyPattern,
            UnknownTarget,
            PatternTooLong,
            UnsafeRegularExpression,
            InvalidRegularExpression
        };

        struct FileFilterError
        {
            FileFilterErrorCode code = FileFilterErrorCode::InvalidRegularExpression;
            size_t offset = 0;
            std::string token;
            std::string message;
        };

        //! Public description of a compiled term.
        struct FileFilterTerm
        {
            bool include = true;
            FileFilterTarget target = FileFilterTarget::Path;
            std::string pattern;
        };

        class CompiledFileFilter;

        //! Result of compiling a file filter expression.
        struct FileFilterCompileResult
        {
            std::shared_ptr<const CompiledFileFilter> filter;
            std::optional<FileFilterError> error;

            explicit operator bool() const;
        };

        //! Immutable, compiled file filter.
        //!
        //! Grammar:
        //! \code
        //! expression := term *(ASCII-whitespace term)
        //! term       := [ '+' | '-' | '!' ] [ target ':' ] regex
        //! target     := path | name | file | ext | extension | dir | folder
        //! \endcode
        //!
        //! Terms without a target match the full path. All include terms must
        //! match and no exclude term may match. Regular expressions use the
        //! C++ ECMAScript grammar with case-insensitive ASCII matching. Patterns
        //! cannot contain literal whitespace; use a regular-expression class
        //! such as `\\s`.
        //! The extension target does not include the leading dot. The directory
        //! target matches individual directory components, which makes explicit
        //! directory exclusions safe to prune during recursive scans.
        //!
        //! To keep interactive filtering bounded, backreferences, lookaround,
        //! quantified groups, repetitions above the candidate limit, and more
        //! than two unbounded quantifiers in one term are rejected.
        class CompiledFileFilter
        {
        public:
            static constexpr size_t maxExpressionLength = 4096;
            static constexpr size_t maxTermCount = 64;
            static constexpr size_t maxPatternLength = 256;
            static constexpr size_t maxCandidateLength = 4096;

            ~CompiledFileFilter();

            //! Match a file path. Overlong candidates do not match.
            bool matches(const ftk::Path&) const;

            //! Whether an excluded directory/path term permits pruning a
            //! directory before visiting its children.
            bool excludesDirectory(const ftk::Path&) const;

            bool isEmpty() const;
            const std::string& getExpression() const;
            const std::vector<FileFilterTerm>& getTerms() const;

        private:
            CompiledFileFilter();

            struct Private;
            std::unique_ptr<Private> _p;

            friend FileFilterCompileResult compileFileFilter(const std::string&);
        };

        //! Compile a filter. Failure is explicit and never falls back to a
        //! different matching language.
        FileFilterCompileResult compileFileFilter(const std::string&);

        //! Curated expressions suitable for a UI preset list.
        std::vector<std::string> getDefaultFileFilterPresets();
    }
}
