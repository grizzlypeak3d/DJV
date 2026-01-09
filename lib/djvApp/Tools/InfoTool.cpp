// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/InfoTool.h>

#include <djvApp/App.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/GridLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/SearchBox.h>
#include <ftk/Core/String.h>

namespace djv
{
    namespace app
    {
        struct InfoTool::Private
        {
            tl::IOInfo info;
            std::string search;

            std::shared_ptr<ftk::SearchBox> searchBox;
            std::shared_ptr<ftk::GridLayout> layout;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
        };

        void InfoTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                Tool::Info,
                "djv::app::InfoTool",
                parent);
            FTK_P();

            p.searchBox = ftk::SearchBox::create(context);
            p.searchBox->setHStretch(ftk::Stretch::Expanding);

            p.layout = ftk::GridLayout::create(context);
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto scrollWidget = ftk::ScrollWidget::create(context);
            scrollWidget->setWidget(p.layout);
            scrollWidget->setBorder(false);
            scrollWidget->setVStretch(ftk::Stretch::Expanding);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);
            scrollWidget->setParent(layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            auto hLayout = ftk::HorizontalLayout::create(context, layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginInside);
            p.searchBox->setParent(hLayout);
            _setWidget(layout);

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    _p->info = value ? value->getIOInfo() : tl::IOInfo();
                    _widgetUpdate();
                });

            p.searchBox->setCallback(
                [this](const std::string& value)
                {
                    _p->search = value;
                    _widgetUpdate();
                });
        }

        InfoTool::InfoTool() :
            _p(new Private)
        {}

        InfoTool::~InfoTool()
        {}

        std::shared_ptr<InfoTool> InfoTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<InfoTool>(new InfoTool);
            out->_init(context, app, parent);
            return out;
        }

        void InfoTool::_widgetUpdate()
        {
            FTK_P();
            auto children = p.layout->getChildren();
            for (const auto& child : children)
            {
                child->setParent(nullptr);
            }
            if (auto context = getContext())
            {
                int row = 0;
                for (const auto& tag : p.info.tags)
                {
                    bool filter = false;
                    if (!p.search.empty() &&
                        !ftk::contains(
                            tag.first,
                            p.search,
                            ftk::CaseCompare::Insensitive) &&
                        !ftk::contains(
                            tag.second,
                            p.search,
                            ftk::CaseCompare::Insensitive))
                    {
                        filter = true;
                    }
                    if (!filter)
                    {
                        auto label = ftk::Label::create(context, tag.first + ":", p.layout);
                        p.layout->setGridPos(label, row, 0);
                        label = ftk::Label::create(context, tag.second, p.layout);
                        p.layout->setGridPos(label, row, 1);
                        ++row;
                    }
                }
            }
        }
    }
}
