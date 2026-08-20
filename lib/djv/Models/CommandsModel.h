// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

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
        struct DJV_API_TYPE CommandInfo
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
        class DJV_API_TYPE CommandsModel : public std::enable_shared_from_this<CommandsModel>
        {
            FTK_NON_COPYABLE(CommandsModel);

        protected:
            void _init(const std::shared_ptr<ftk::Context>&);

            CommandsModel();

        public:
            DJV_API ~CommandsModel();

            //! Create a new model.
            DJV_API static std::shared_ptr<CommandsModel> create(
                const std::shared_ptr<ftk::Context>&);

            //! Add a command.
            DJV_API void add(
                const std::string& name,
                const std::string& doc,
                const CommandFunc&);

            //! Remove a command.
            DJV_API void remove(const std::string& name);

            //! Get information about the commands, sorted by name.
            DJV_API std::vector<CommandInfo> getCommands() const;

            //! Execute a command. Errors are logged and false is returned.
            DJV_API bool exec(
                const std::string& name,
                const nlohmann::json& args = nlohmann::json());

        private:
            FTK_PRIVATE();
        };
    }
}
