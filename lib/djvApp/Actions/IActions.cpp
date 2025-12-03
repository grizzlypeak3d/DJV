// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Actions/IActions.h>

#include <djvApp/App.h>

#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct IActions::Private
        {
            std::shared_ptr<ftk::Observer<ShortcutsSettings> > shortcutsSettingsObserver;
        };

        void IActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::string& name)
        {
            FTK_P();

            _name = name;

            p.shortcutsSettingsObserver = ftk::Observer<ShortcutsSettings>::create(
                app->getSettingsModel()->observeShortcuts(),
                [this](const ShortcutsSettings& value)
                {
                    _shortcutsUpdate(value);
                });
        }

        IActions::IActions() :
            _p(new Private)
        {}

        IActions::~IActions()
        {}

        const std::map<std::string, std::shared_ptr<ftk::Action> >& IActions::getActions() const
        {
            return _actions;
        }

        void IActions::_shortcutsUpdate(const ShortcutsSettings& value)
        {
            for (const auto& i : _actions)
            {
                const std::string name = ftk::Format("{0}/{1}").arg(_name).arg(i.first);
                const auto j = std::find_if(
                    value.shortcuts.begin(),
                    value.shortcuts.end(),
                    [name](const Shortcut& value)
                    {
                        return name == value.name;
                    });
                if (j != value.shortcuts.end())
                {
                    i.second->setShortcut(j->key);
                    i.second->setShortcutModifiers(j->modifiers);
                    const auto k = _tooltips.find(i.first);
                    if (k != _tooltips.end())
                    {
                        i.second->setTooltip(ftk::Format(
                            "{0}\n"
                            "\n"
                            "Shortcut: {1}").
                            arg(k->second).
                            arg(ftk::getShortcutLabel(j->key, j->modifiers)));
                    }
                }
            }
        }
    }
}
