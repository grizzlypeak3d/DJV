// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/ColorPickerTool.h>

#include <djvApp/Models/SettingsModel.h>
#include <djvApp/Models/ViewportModel.h>
#include <djvApp/App.h>

#include <ftk/UI/ColorWidget.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct ColorPickerTool::Private
        {
            std::shared_ptr<ftk::ColorWidget> colorWidget;
            std::shared_ptr<ftk::Label> label;

            std::shared_ptr<ftk::Observer<ftk::Color4F> > colorPickerObserver;
            std::shared_ptr<ftk::Observer<MouseSettings> > settingsObserver;
        };

        void ColorPickerTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                Tool::ColorPicker,
                "djv::app::ColorPickerTool",
                parent);
            FTK_P();

            p.colorWidget = ftk::ColorWidget::create(context);
            p.colorWidget->setColor(ftk::Color4F(0.F, 0.F, 0.F));

            p.label = ftk::Label::create(context);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setMarginRole(ftk::SizeRole::MarginSmall);
            layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.colorWidget->setParent(layout);
            p.label->setParent(layout);

            auto scrollWidget = ftk::ScrollWidget::create(context);
            scrollWidget->setBorder(false);
            scrollWidget->setWidget(layout);
            _setWidget(scrollWidget);

            p.colorPickerObserver = ftk::Observer<ftk::Color4F>::create(
                app->getViewportModel()->observeColorPicker(),
                [this](const ftk::Color4F& value)
                {
                    _p->colorWidget->setColor(value);
                });

            p.settingsObserver = ftk::Observer<MouseSettings>::create(
                app->getSettingsModel()->observeMouse(),
                [this](const MouseSettings& value)
                {
                    std::vector<std::string> s;
                    auto i = value.bindings.find(MouseAction::Pick);
                    if (i != value.bindings.end())
                    {
                        if (i->second.button != ftk::MouseButton::None)
                        {
                            if (i->second.modifier != ftk::KeyModifier::None)
                            {
                                s.push_back(ftk::to_string(i->second.modifier));
                            }
                            s.push_back(ftk::getLabel(i->second.button));
                        }
                    }
                    _p->label->setText(ftk::Format("Mouse binding: {0} Click").
                        arg(ftk::join(s, " + ")));
                });
        }

        ColorPickerTool::ColorPickerTool() :
            _p(new Private)
        {}

        ColorPickerTool::~ColorPickerTool()
        {}

        std::shared_ptr<ColorPickerTool> ColorPickerTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ColorPickerTool>(new ColorPickerTool);
            out->_init(context, app, parent);
            return out;
        }

        void ColorPickerTool::_widgetUpdate()
        {
            FTK_P();
        }
    }
}
