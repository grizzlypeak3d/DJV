// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/StatusIndicatorPopup.h>

#include <ftk/UI/GridLayout.h>
#include <ftk/UI/ToolButton.h>

namespace djv
{
    namespace ui
    {
        struct StatusIndicatorPopup::Private
        {
            std::map<std::string, std::shared_ptr<ftk::ToolButton> > offButtons;
            std::map<std::string, std::shared_ptr<ftk::ToolButton> > toolButtons;
            std::function<void(const std::string&)> offCallback;
            std::function<void(const std::string&)> toolCallback;
        };

        void StatusIndicatorPopup::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::vector<std::pair<std::string, std::string> >& indicators,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidgetPopup::_init(
                context,
                "djv::ui::StatusIndicatorPopup",
                parent);
            FTK_P();

            auto layout = ftk::GridLayout::create(context);
            layout->setMarginRole(ftk::SizeRole::MarginSmall);
            layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            setWidget(layout);

            int row = 0;
            for (const auto& i : indicators)
            {
                const std::string name = i.first;

                auto offButton = ftk::ToolButton::create(context, layout);
                offButton->setIcon(std::string("MenuChecked"));
                offButton->setTooltip("Turn off");
                offButton->setClickedCallback(
                    [this, name]
                    {
                        if (_p->offCallback)
                        {
                            _p->offCallback(name);
                        }
                    });
                p.offButtons[name] = offButton;

                auto toolButton = ftk::ToolButton::create(context, i.second, layout);
                toolButton->setTooltip("Show the tool");
                toolButton->setClickedCallback(
                    [this, name]
                    {
                        if (_p->toolCallback)
                        {
                            _p->toolCallback(name);
                        }
                    });
                p.toolButtons[name] = toolButton;

                layout->setGridPos(offButton, row, 0);
                layout->setGridPos(toolButton, row, 1);
                ++row;
            }
        }

        StatusIndicatorPopup::StatusIndicatorPopup() :
            _p(new Private)
        {}

        StatusIndicatorPopup::~StatusIndicatorPopup()
        {}

        std::shared_ptr<StatusIndicatorPopup> StatusIndicatorPopup::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::vector<std::pair<std::string, std::string> >& indicators,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<StatusIndicatorPopup>(new StatusIndicatorPopup);
            out->_init(context, indicators, parent);
            return out;
        }

        void StatusIndicatorPopup::setIndicators(const std::map<std::string, bool>& values)
        {
            FTK_P();
            for (const auto& i : values)
            {
                if (const auto j = p.offButtons.find(i.first); j != p.offButtons.end())
                {
                    j->second->setEnabled(i.second);
                    j->second->setBackgroundRole(i.second ?
                        ftk::ColorRole::Checked :
                        ftk::ColorRole::None);
                }
                if (const auto k = p.toolButtons.find(i.first); k != p.toolButtons.end())
                {
                    k->second->setEnabled(i.second);
                }
            }
        }

        void StatusIndicatorPopup::setOffCallback(const std::function<void(const std::string&)>& value)
        {
            _p->offCallback = value;
        }

        void StatusIndicatorPopup::setToolCallback(const std::function<void(const std::string&)>& value)
        {
            _p->toolCallback = value;
        }
    }
}
