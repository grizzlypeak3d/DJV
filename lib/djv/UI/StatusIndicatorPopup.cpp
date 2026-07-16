// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/StatusIndicatorPopup.h>

#include <ftk/UI/GridLayout.h>
#include <ftk/UI/Icon.h>
#include <ftk/UI/Label.h>

namespace djv
{
    namespace ui
    {
        struct StatusIndicatorPopup::Private
        {
            std::map<std::string, std::shared_ptr<ftk::Icon> > icons;
            std::map<std::string, std::shared_ptr<ftk::Label> > labels;
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
                p.icons[i.first] = ftk::Icon::create(context, "MenuChecked", layout);
                p.labels[i.first] = ftk::Label::create(context, i.second, layout);
                layout->setGridPos(p.icons[i.first], row, 0);
                layout->setGridPos(p.labels[i.first], row, 1);
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
                const auto j = p.icons.find(i.first);
                if (j != p.icons.end())
                {
                    j->second->setEnabled(i.second);
                    j->second->setBackgroundRole(i.second ?
                        ftk::ColorRole::Checked :
                        ftk::ColorRole::None);
                }
                const auto k = p.labels.find(i.first);
                if (k != p.labels.end())
                {
                    k->second->setEnabled(i.second);
                }
            }
        }
    }
}
