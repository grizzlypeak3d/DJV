// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/CommandsModel.h>

#include <ftk/Core/Context.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/LogSystem.h>

#include <algorithm>
#include <map>

namespace djv
{
    namespace models
    {
        namespace
        {
            const std::string logPrefix = "djv::models::CommandsModel";
        }

        struct CommandsModel::Private
        {
            std::weak_ptr<ftk::Context> context;

            struct Command
            {
                std::string doc;
                CommandFunc func;
            };
            std::map<std::string, Command> commands;
        };

        void CommandsModel::_init(const std::shared_ptr<ftk::Context>& context)
        {
            FTK_P();
            p.context = context;
        }

        CommandsModel::CommandsModel() :
            _p(new Private)
        {}

        CommandsModel::~CommandsModel()
        {}

        std::shared_ptr<CommandsModel> CommandsModel::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            auto out = std::shared_ptr<CommandsModel>(new CommandsModel);
            out->_init(context);
            return out;
        }

        void CommandsModel::add(
            const std::string& name,
            const std::string& doc,
            const CommandFunc& func)
        {
            FTK_P();
            p.commands[name] = { doc, func };
        }

        void CommandsModel::remove(const std::string& name)
        {
            FTK_P();
            const auto i = p.commands.find(name);
            if (i != p.commands.end())
            {
                p.commands.erase(i);
            }
        }

        std::vector<CommandInfo> CommandsModel::getCommands() const
        {
            FTK_P();
            std::vector<CommandInfo> out;
            for (const auto& i : p.commands)
            {
                out.push_back({ i.first, i.second.doc });
            }
            return out;
        }

        bool CommandsModel::exec(
            const std::string& name,
            const nlohmann::json& args)
        {
            FTK_P();
            bool out = false;
            const auto i = p.commands.find(name);
            if (i != p.commands.end())
            {
                try
                {
                    i->second.func(args);
                    out = true;
                }
                catch (const std::exception& e)
                {
                    if (auto context = p.context.lock())
                    {
                        context->getLogSystem()->print(
                            logPrefix,
                            ftk::Format("Command error: \"{0}\": {1}").
                            arg(name).
                            arg(e.what()),
                            ftk::LogType::Error);
                    }
                }
            }
            else if (auto context = p.context.lock())
            {
                context->getLogSystem()->print(
                    logPrefix,
                    ftk::Format("Unknown command: \"{0}\"").arg(name),
                    ftk::LogType::Error);
            }
            return out;
        }
    }
}
