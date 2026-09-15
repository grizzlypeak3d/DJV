// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/ShellCommand.h>

#include <ftk/Core/Path.h>
#include <ftk/Core/String.h>

namespace djv
{
    namespace models
    {
        namespace
        {
            bool isSpace(char c)
            {
                return ' ' == c || '\t' == c;
            }
        }

        std::optional<ShellCommand> parseShellCommand(const std::string& command)
        {
            size_t i = 0;
            while (i < command.size() && isSpace(command[i]))
            {
                ++i;
            }
            if (i == command.size())
            {
                return std::nullopt;
            }
            ShellCommand out;
            size_t end = 0;
            if ('"' == command[i])
            {
                const size_t close = command.find('"', i + 1);
                if (std::string::npos == close)
                {
                    return std::nullopt;
                }
                out.program = command.substr(i + 1, close - i - 1);
                end = close + 1;
            }
            else
            {
                // Unquoted, Windows guesses where a path with spaces in it
                // ends. Taking the first space is the conservative reading:
                // a path it cuts short names nothing, and nothing is changed.
                end = i;
                while (end < command.size() && !isSpace(command[end]))
                {
                    ++end;
                }
                out.program = command.substr(i, end - i);
            }
            while (end < command.size() && isSpace(command[end]))
            {
                ++end;
            }
            out.arguments = command.substr(end);
            if (out.program.empty())
            {
                return std::nullopt;
            }
            return out;
        }

        std::optional<std::string> repairShellCommand(
            const std::string& command,
            const std::filesystem::path& exe,
            const std::function<bool(const std::filesystem::path&)>& exists)
        {
            const auto parsed = parseShellCommand(command);
            if (!parsed)
            {
                return std::nullopt;
            }
            const std::filesystem::path program = ftk::toFileSystem(parsed->program);
            const std::string exeString = ftk::fromFileSystem(exe);

            // Another program's command: not ours to touch.
            if (!ftk::compare(
                ftk::fromFileSystem(program.filename()),
                ftk::fromFileSystem(exe.filename()),
                ftk::CaseCompare::Insensitive))
            {
                return std::nullopt;
            }

            // Already this executable.
            if (ftk::compare(parsed->program, exeString, ftk::CaseCompare::Insensitive))
            {
                return std::nullopt;
            }

            // A copy that is still there, which someone chose.
            if (exists && exists(program))
            {
                return std::nullopt;
            }

            std::string out = "\"" + exeString + "\"";
            if (!parsed->arguments.empty())
            {
                out += " " + parsed->arguments;
            }
            return out;
        }
    }
}
