// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/Core/Util.h>

#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ftk
{
    class Context;
}

namespace djv
{
    namespace models
    {
        //! Command function.
        typedef std::function<void(const nlohmann::json&)> CommandFunc;

        //! Command information.
        struct CommandInfo
        {
            std::string name;
            std::string doc;
        };

        //! Commands model.
        //!
        //! Commands are named, scriptable operations that manipulate the
        //! application. Menu actions, keyboard shortcuts, and automation
        //! (e.g., screenshot capture) share this single entry point for
        //! invoking application functionality.
        class CommandsModel : public std::enable_shared_from_this<CommandsModel>
        {
            FTK_NON_COPYABLE(CommandsModel);

        protected:
            void _init(const std::shared_ptr<ftk::Context>&);

            CommandsModel();

        public:
            ~CommandsModel();

            //! Create a new model.
            static std::shared_ptr<CommandsModel> create(
                const std::shared_ptr<ftk::Context>&);

            //! Add a command.
            void add(
                const std::string& name,
                const std::string& doc,
                const CommandFunc&);

            //! Remove a command.
            void remove(const std::string& name);

            //! Get information about the commands, sorted by name.
            std::vector<CommandInfo> getCommands() const;

            //! Execute a command. Errors are logged and false is returned.
            bool exec(
                const std::string& name,
                const nlohmann::json& args = nlohmann::json());

        private:
            FTK_PRIVATE();
        };
    }
}
