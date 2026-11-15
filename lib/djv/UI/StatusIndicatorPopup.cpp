// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/StatusIndicatorPopup.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/GridLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/ToolButton.h>

#include <algorithm>

namespace djv
{
    namespace ui
    {
        struct StatusIndicatorPopup::Private
        {
            std::map<std::string, std::shared_ptr<ftk::ToolButton> > buttons;
            std::map<std::string, std::string> itemGroups;
            std::map<std::string, std::shared_ptr<ftk::Label> > letters;
            std::function<void(const std::string&)> callback;
        };

        void StatusIndicatorPopup::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::vector<models::StatusItem>& items,
            const std::vector<models::StatusGroup>& groups,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidgetPopup::_init(
                context,
                "djv::ui::StatusIndicatorPopup",
                parent);
            FTK_P();

            // The letter beside the first of its group's options, as the
            // indicator shows it, so the popup reads as the letters spelled
            // out, and a divider between the groups: one in each column, no
            // space between the columns so the two join, and the letters
            // given their room by a margin of their own instead.
            auto layout = ftk::GridLayout::create(context);
            layout->setMarginRole(ftk::SizeRole::MarginSmall);
            layout->setSpacingRole(ftk::SizeRole::None, ftk::SizeRole::SpacingTool);
            setWidget(layout);

            int row = 0;
            for (const auto& group : groups)
            {
                const bool empty = std::none_of(
                    items.begin(),
                    items.end(),
                    [&group](const models::StatusItem& item)
                    {
                        return item.group == group.id;
                    });
                if (empty)
                    continue;
                if (row > 0)
                {
                    for (int column = 0; column < 2; ++column)
                    {
                        auto divider = ftk::Divider::create(
                            context,
                            ftk::Orientation::Vertical,
                            layout);
                        layout->setGridPos(divider, row, column);
                    }
                    ++row;
                }
                bool first = true;
                for (const auto& item : items)
                {
                    if (item.group != group.id)
                        continue;
                    if (first)
                    {
                        first = false;
                        auto label = ftk::Label::create(context, group.letter, layout);
                        label->setFont(ftk::FontType::Bold);
                        label->setTextRole(ftk::ColorRole::TextDisabled);
                        label->setHMarginRole(ftk::SizeRole::MarginSmall);
                        layout->setGridPos(label, row, 0);
                        p.letters[group.id] = label;
                    }
                    const std::string id = item.id;
                    auto button = ftk::ToolButton::create(context, item.label, layout);
                    button->setHStretch(ftk::Stretch::Expanding);
                    button->setTooltip("Show the controls for this option.");
                    button->setClickedCallback(
                        [this, id]
                        {
                            if (_p->callback)
                            {
                                _p->callback(id);
                            }
                        });
                    layout->setGridPos(button, row, 1);
                    p.buttons[id] = button;
                    p.itemGroups[id] = group.id;
                    ++row;
                }
            }
        }

        StatusIndicatorPopup::StatusIndicatorPopup() :
            _p(new Private)
        {}

        StatusIndicatorPopup::~StatusIndicatorPopup()
        {}

        std::shared_ptr<StatusIndicatorPopup> StatusIndicatorPopup::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::vector<models::StatusItem>& items,
            const std::vector<models::StatusGroup>& groups,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<StatusIndicatorPopup>(new StatusIndicatorPopup);
            out->_init(context, items, groups, parent);
            return out;
        }

        void StatusIndicatorPopup::setIndicators(const std::map<std::string, bool>& values)
        {
            FTK_P();
            std::map<std::string, bool> groups;
            for (const auto& i : p.buttons)
            {
                const auto j = values.find(i.first);
                const bool inUse = j != values.end() && j->second;
                // Checked for the tint, but not checkable: clicking goes to
                // the controls rather than turning the option off.
                i.second->setChecked(inUse);
                i.second->setEnabled(inUse);
                groups[p.itemGroups[i.first]] |= inUse;
            }
            for (const auto& i : p.letters)
            {
                i.second->setTextRole(groups[i.first] ?
                    ftk::ColorRole::Text :
                    ftk::ColorRole::TextDisabled);
            }
        }

        void StatusIndicatorPopup::setCallback(const std::function<void(const std::string&)>& value)
        {
            _p->callback = value;
        }
    }
}
