// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace djv
{
    namespace models
    {
        //! A shell "open" command, as Windows stores it in the registry: the
        //! program, and whatever follows it.
        struct DJV_MODELS_API_TYPE ShellCommand
        {
            std::string program;
            std::string arguments;
        };

        //! Split a shell command into the program and its arguments. The
        //! program is either quoted, or runs to the first space. Returns
        //! nothing for an empty command or an unterminated quote.
        DJV_MODELS_API std::optional<ShellCommand> parseShellCommand(
            const std::string&);

        //! Given a shell command and the executable that should be answering
        //! it, return the command rewritten to name that executable -- only
        //! when the command names a program with the same file name that no
        //! longer exists. Anything else is left alone and returns nothing: a
        //! command for another program, one already right, and one naming a
        //! copy that is still installed somewhere else, which is a choice
        //! rather than a leftover.
        DJV_MODELS_API std::optional<std::string> repairShellCommand(
            const std::string& command,
            const std::filesystem::path& exe,
            const std::function<bool(const std::filesystem::path&)>& exists);
    }
}
