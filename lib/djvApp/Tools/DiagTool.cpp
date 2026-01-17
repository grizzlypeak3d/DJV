// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/DiagTool.h>

#include <tlRender/Core/Audio.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/GraphWidget.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>
#include <ftk/Core/Timer.h>

namespace djv
{
    namespace app
    {
        struct DiagTool::Private
        {
            std::map<std::string, std::shared_ptr<ftk::Label> > labels;
            std::map<std::string, std::shared_ptr<ftk::GraphWidget> > graphs;
            std::shared_ptr<ftk::Timer> timer;
        };

        void DiagTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                Tool::Diag,
                "djv::app::DiagTool",
                parent);
            FTK_P();

            p.labels["Images"] = ftk::Label::create(context);
            p.graphs["Images"] = ftk::GraphWidget::create(context);
            p.labels["ImagesSize"] = ftk::Label::create(context);
            p.graphs["ImagesSize"] = ftk::GraphWidget::create(context);
            p.labels["Audio"] = ftk::Label::create(context);
            p.graphs["Audio"] = ftk::GraphWidget::create(context);
            p.labels["AudioSize"] = ftk::Label::create(context);
            p.graphs["AudioSize"] = ftk::GraphWidget::create(context);
            p.labels["Widgets"] = ftk::Label::create(context);
            p.graphs["Widgets"] = ftk::GraphWidget::create(context);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);
            auto vLayout = ftk::VerticalLayout::create(context, layout);
            vLayout->setMarginRole(ftk::SizeRole::Margin);
            vLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.labels["Images"]->setParent(vLayout);
            p.graphs["Images"]->setParent(vLayout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            vLayout = ftk::VerticalLayout::create(context, layout);
            vLayout->setMarginRole(ftk::SizeRole::Margin);
            vLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.labels["ImagesSize"]->setParent(vLayout);
            p.graphs["ImagesSize"]->setParent(vLayout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            vLayout = ftk::VerticalLayout::create(context, layout);
            vLayout->setMarginRole(ftk::SizeRole::Margin);
            vLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.labels["Audio"]->setParent(vLayout);
            p.graphs["Audio"]->setParent(vLayout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            vLayout = ftk::VerticalLayout::create(context, layout);
            vLayout->setMarginRole(ftk::SizeRole::Margin);
            vLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.labels["AudioSize"]->setParent(vLayout);
            p.graphs["AudioSize"]->setParent(vLayout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            vLayout = ftk::VerticalLayout::create(context, layout);
            vLayout->setMarginRole(ftk::SizeRole::Margin);
            vLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.labels["Widgets"]->setParent(vLayout);
            p.graphs["Widgets"]->setParent(vLayout);

            auto scrollWidget = ftk::ScrollWidget::create(context);
            scrollWidget->setWidget(layout);
            scrollWidget->setBorder(false);
            scrollWidget->setVStretch(ftk::Stretch::Expanding);
            _setWidget(scrollWidget);

            p.timer = ftk::Timer::create(context);
            p.timer->setRepeating(true);
            p.timer->start(
                std::chrono::milliseconds(1000),
                [this]
                {
                    FTK_P();
                    size_t count = ftk::Image::getObjectCount();
                    p.labels["Images"]->setText(ftk::Format("Image objects: {0}").arg(count));
                    p.graphs["Images"]->addSample(count);
                    count = ftk::Image::getTotalByteCount() / ftk::megabyte;
                    p.labels["ImagesSize"]->setText(ftk::Format("Total images size: {0}MB").arg(count));
                    p.graphs["ImagesSize"]->addSample(count);

                    count = tl::Audio::getObjectCount();
                    p.labels["Audio"]->setText(ftk::Format("Audio objects: {0}").arg(count));
                    p.graphs["Audio"]->addSample(count);
                    count = tl::Audio::getTotalByteCount() / ftk::megabyte;
                    p.labels["AudioSize"]->setText(ftk::Format("Total audio size: {0}MB").arg(count));
                    p.graphs["AudioSize"]->addSample(count);

                    count = ftk::IWidget::getObjectCount();
                    p.labels["Widgets"]->setText(ftk::Format("Widgets: {0}").arg(count));
                    p.graphs["Widgets"]->addSample(count);
                });
        }

        DiagTool::DiagTool() :
            _p(new Private)
        {}

        DiagTool::~DiagTool()
        {}

        std::shared_ptr<DiagTool> DiagTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<DiagTool>(new DiagTool);
            out->_init(context, app, parent);
            return out;
        }
    }
}
