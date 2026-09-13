// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/StatusIndicatorPopup.h>

#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ToolButton.h>

namespace djv
{
    namespace ui
    {
        struct StatusIndicatorPopup::Private
        {
            std::map<std::string, std::shared_ptr<ftk::ToolButton> > buttons;
            std::function<void(const std::string&)> callback;
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

            auto layout = ftk::VerticalLayout::create(context);
            layout->setMarginRole(ftk::SizeRole::MarginSmall);
            layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            setWidget(layout);

            for (const auto& i : indicators)
            {
                const std::string name = i.first;
                auto button = ftk::ToolButton::create(context, i.second, layout);
                button->setHStretch(ftk::Stretch::Expanding);
                button->setTooltip("Show the controls for this option.");
                button->setClickedCallback(
                    [this, name]
                    {
                        if (_p->callback)
                        {
                            _p->callback(name);
                        }
                    });
                p.buttons[name] = button;
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
            for (const auto& i : p.buttons)
            {
                const auto j = values.find(i.first);
                const bool inUse = j != values.end() && j->second;
                // Lit rather than checked: clicking goes to the controls,
                // and a checked button would promise to turn the option off.
                i.second->setButtonRole(inUse ?
                    ftk::ColorRole::Checked :
                    ftk::ColorRole::None);
                i.second->setEnabled(inUse);
            }
        }

        void StatusIndicatorPopup::setCallback(const std::function<void(const std::string&)>& value)
        {
            _p->callback = value;
        }
    }
}
