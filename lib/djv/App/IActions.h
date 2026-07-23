// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/CommandsModel.h>
#include <djv/Models/SettingsModel.h>

#include <ftk/UI/Action.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Base class for action groups.
        class IActions : public std::enable_shared_from_this<IActions>
        {
            FTK_NON_COPYABLE(IActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::string& name);

            IActions();

        public:
            ~IActions();

            const std::map<std::string, std::shared_ptr<ftk::Action> >& getActions() const;

        protected:
            //! Register a command named "<group>/<name>" with the commands
            //! model. The documentation is also used as the action tooltip.
            //! Commands are removed when the action group is destroyed.
            void _addCommand(
                const std::string& name,
                const std::string& doc,
                const models::CommandFunc&);

            //! Register a command like _addCommand() that receives a checked
            //! state as { "value": <bool> }. An example of the arguments is
            //! appended to the command documentation.
            void _addCheckCommand(
                const std::string& name,
                const std::string& doc,
                const models::CommandFunc&);

            //! Get a callback that executes the command named
            //! "<group>/<name>", suitable for use as an action callback.
            std::function<void(void)> _command(const std::string& name);

            //! Get a callback that executes the command named
            //! "<group>/<name>" with the checked state as
            //! { "value": <bool> }, suitable for use as a checkable action
            //! callback.
            std::function<void(bool)> _checkCommand(const std::string& name);

            //! Register a keyboard shortcut named "<group>/<name>" with the
            //! settings model. Any saved key binding overrides the given
            //! default.
            void _addShortcut(
                const std::string& name,
                const std::string& label,
                const ftk::KeyShortcut& primary = ftk::KeyShortcut(),
                const ftk::KeyShortcut& secondary = ftk::KeyShortcut());

            void _shortcutsUpdate(const models::ShortcutsSettings&);

            std::string _name;
            std::map<std::string, std::shared_ptr<ftk::Action> > _actions;
            std::map<std::string, std::string> _tooltips;

        private:
            FTK_PRIVATE();
        };
    }
}
