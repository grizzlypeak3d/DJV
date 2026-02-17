// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

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
            void _shortcutsUpdate(const models::ShortcutsSettings&);

            std::string _name;
            std::map<std::string, std::shared_ptr<ftk::Action> > _actions;
            std::map<std::string, std::string> _tooltips;

        private:
            FTK_PRIVATE();
        };
    }
}
