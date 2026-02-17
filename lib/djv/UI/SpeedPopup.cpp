// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/SpeedPopup.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/DoubleEdit.h>
#include <ftk/UI/ListItemsWidget.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace ui
    {
        struct SpeedPopup::Private
        {
            std::vector<double> speeds;
            std::shared_ptr<ftk::ListItemsWidget> listWidget;
            std::shared_ptr<ftk::DoubleEdit> speedEdit;
            std::function<void(double)> callback;
        };

        void SpeedPopup::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::DoubleModel>& model,
            double defaultSpeed,
            const std::shared_ptr<IWidget>& parent)
        {
            IMenuPopup::_init(
                context,
                "djv::ui::SpeedPopup",
                parent);
            FTK_P();

            p.speeds =
            {
                120.0,
                96.0,
                90.0,
                60.0,
                60000.0 / 1001.0,
                50.0,
                48.0,
                30.0,
                30000.0 / 1001.0,
                25.0,
                24.0,
                24000.0 / 1001.0,
                18.0,
                16.0,
                15.0,
                12.0,
                6.0,
                3.0,
                1.0,

                defaultSpeed
            };

            p.listWidget = ftk::ListItemsWidget::create(context, ftk::ButtonGroupType::Click);

            p.speedEdit = ftk::DoubleEdit::create(context, model);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);
            p.listWidget->setParent(layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            auto hLayout = ftk::HorizontalLayout::create(context, layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            p.speedEdit->setParent(hLayout);
            setWidget(layout);

            _widgetUpdate();

            auto weak = std::weak_ptr<SpeedPopup>(std::dynamic_pointer_cast<SpeedPopup>(shared_from_this()));
            p.listWidget->setCallback(
                [weak](int index, bool value)
                {
                    if (auto widget = weak.lock())
                    {
                        if (value && index >= 0 && index < widget->_p->speeds.size())
                        {
                            if (widget->_p->callback)
                            {
                                widget->_p->callback(widget->_p->speeds[index]);
                            }
                        }
                    }
                });
        }

        SpeedPopup::SpeedPopup() :
            _p(new Private)
        {}

        SpeedPopup::~SpeedPopup()
        {}

        std::shared_ptr<SpeedPopup> SpeedPopup::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::DoubleModel>& model,
            double defaultSpeed,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SpeedPopup>(new SpeedPopup);
            out->_init(context, model, defaultSpeed, parent);
            return out;
        }

        void SpeedPopup::setCallback(const std::function<void(double)>& value)
        {
            _p->callback = value;
        }

        void SpeedPopup::open(
            const std::shared_ptr<ftk::IWindow>& window,
            const ftk::Box2I& buttonGeometry)
        {
            IMenuPopup::open(window, buttonGeometry);
            _p->speedEdit->takeKeyFocus();
        }

        void SpeedPopup::_widgetUpdate()
        {
            FTK_P();
            std::vector<std::string> items;
            const size_t size = p.speeds.size();
            for (size_t i = 0; i < size; ++i)
            {
                items.push_back((size - 1) == i ?
                    ftk::Format("Default: {0}").arg(p.speeds[i], 2) :
                    ftk::Format("{0}").arg(p.speeds[i], 2));
            }
            p.listWidget->setItems(items);
        }
    }
}
