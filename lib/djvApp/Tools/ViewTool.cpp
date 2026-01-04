// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/ViewToolPrivate.h>

#include <djvApp/Models/ViewportModel.h>
#include <djvApp/Widgets/Viewport.h>
#include <djvApp/App.h>
#include <djvApp/MainWindow.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/DoubleEdit.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/GroupBox.h>
#include <ftk/UI/IntEdit.h>
#include <ftk/UI/IntEditSlider.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/LineEdit.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>

#include <sstream>

namespace djv
{
    namespace app
    {
        struct ViewPosZoomWidget::Private
        {
            std::shared_ptr<ftk::IntEdit> posXEdit;
            std::shared_ptr<ftk::IntResetButton> posXReset;
            std::shared_ptr<ftk::IntEdit> posYEdit;
            std::shared_ptr<ftk::IntResetButton> posYReset;
            std::shared_ptr<ftk::DoubleEdit> zoomEdit;
            std::shared_ptr<ftk::DoubleResetButton> zoomReset;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<std::pair<ftk::V2I, double> > > posZoomObserver;
            bool updating = false;
        };

        void ViewPosZoomWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IWidget::_init(context, "djv::app::ViewPosZoomWidget", parent);
            FTK_P();

            p.posXEdit = ftk::IntEdit::create(context);
            p.posXEdit->setRange(-100000000, 100000000);
            p.posXEdit->setDefaultValue(0.0);
            p.posXReset = ftk::IntResetButton::create(context, p.posXEdit->getModel());

            p.posYEdit = ftk::IntEdit::create(context);
            p.posYEdit->setRange(-100000000, 100000000);
            p.posYEdit->setDefaultValue(0.0);
            p.posYReset = ftk::IntResetButton::create(context, p.posYEdit->getModel());

            p.zoomEdit = ftk::DoubleEdit::create(context);
            p.zoomEdit->setRange(0.000001, 100000.0);
            p.zoomEdit->setStep(1.0);
            p.zoomEdit->setLargeStep(2.0);
            p.zoomEdit->setDefaultValue(1.0);
            p.zoomReset = ftk::DoubleResetButton::create(context, p.zoomEdit->getModel());

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingTool);
            p.posXEdit->setParent(hLayout);
            p.posXReset->setParent(hLayout);
            p.layout->addRow("X:", hLayout);
            hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingTool);
            p.posYEdit->setParent(hLayout);
            p.posYReset->setParent(hLayout);
            p.layout->addRow("Y:", hLayout);
            hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingTool);
            p.zoomEdit->setParent(hLayout);
            p.zoomReset->setParent(hLayout);
            p.layout->addRow("Zoom:", hLayout);

            std::weak_ptr<MainWindow> mainWindowWeak(mainWindow);
            p.posXEdit->setCallback(
                [this, mainWindowWeak](int value)
                {
                    if (!_p->updating)
                    {
                        if (auto mainWindow = mainWindowWeak.lock())
                        {
                            auto viewport = mainWindow->getViewport();
                            const ftk::V2I& pos = viewport->getViewPos();
                            const double zoom = viewport->getViewZoom();
                            viewport->setViewPosAndZoom(ftk::V2I(value, pos.y), zoom);
                        }
                    }
                });

            p.posYEdit->setCallback(
                [this, mainWindowWeak](int value)
                {
                    if (!_p->updating)
                    {
                        if (auto mainWindow = mainWindowWeak.lock())
                        {
                            auto viewport = mainWindow->getViewport();
                            const ftk::V2I& pos = viewport->getViewPos();
                            const double zoom = viewport->getViewZoom();
                            viewport->setViewPosAndZoom(ftk::V2I(pos.x, value), zoom);
                        }
                    }
                });

            p.zoomEdit->setCallback(
                [this, mainWindowWeak](double value)
                {
                    if (!_p->updating)
                    {
                        if (auto mainWindow = mainWindowWeak.lock())
                        {
                            auto viewport = mainWindow->getViewport();
                            const ftk::Box2I& g = viewport->getGeometry();
                            const ftk::V2I focus(g.w() / 2, g.h() / 2);
                            mainWindow->getViewport()->setViewZoom(value, focus);
                        }
                    }
                });

            p.posZoomObserver = ftk::Observer<std::pair<ftk::V2I, double> >::create(
                mainWindow->getViewport()->observeViewPosAndZoom(),
                [this](const std::pair<ftk::V2I, double>& value)
                {
                    FTK_P();
                    p.updating = true;
                    p.posXEdit->setValue(value.first.x);
                    p.posYEdit->setValue(value.first.y);
                    p.zoomEdit->setValue(value.second);
                    p.updating = false;
                });
        }

        ViewPosZoomWidget::ViewPosZoomWidget() :
            _p(new Private)
        {}

        ViewPosZoomWidget::~ViewPosZoomWidget()
        {}

        std::shared_ptr<ViewPosZoomWidget> ViewPosZoomWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ViewPosZoomWidget>(new ViewPosZoomWidget);
            out->_init(context, app, mainWindow, parent);
            return out;
        }

        ftk::Size2I ViewPosZoomWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void ViewPosZoomWidget::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        struct ViewOptionsWidget::Private
        {
            std::vector<ftk::ImageType> colorBuffers;

            std::shared_ptr<ftk::ComboBox> minifyComboBox;
            std::shared_ptr<ftk::ComboBox> magnifyComboBox;
            std::shared_ptr<ftk::ComboBox> videoLevelsComboBox;
            std::shared_ptr<ftk::ComboBox> alphaBlendComboBox;
            std::shared_ptr<ftk::ComboBox> colorBufferComboBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<ftk::ImageOptions> > imageOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::timeline::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageType> > colorBufferObserver;
        };

        void ViewOptionsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IWidget::_init(context, "djv::app::ViewOptionsWidget", parent);
            FTK_P();

            p.minifyComboBox = ftk::ComboBox::create(
                context,
                ftk::getImageFilterLabels());
            p.minifyComboBox->setHStretch(ftk::Stretch::Expanding);

            p.magnifyComboBox = ftk::ComboBox::create(
                context,
                ftk::getImageFilterLabels());
            p.magnifyComboBox->setHStretch(ftk::Stretch::Expanding);

            p.videoLevelsComboBox = ftk::ComboBox::create(
                context,
                ftk::getInputVideoLevelsLabels());
            p.videoLevelsComboBox->setHStretch(ftk::Stretch::Expanding);

            p.alphaBlendComboBox = ftk::ComboBox::create(
                context,
                ftk::getAlphaBlendLabels());
            p.videoLevelsComboBox->setHStretch(ftk::Stretch::Expanding);

            p.colorBuffers.push_back(ftk::ImageType::RGBA_U8);
#if defined(FTK_API_GL_4_1)
            p.colorBuffers.push_back(ftk::ImageType::RGBA_F16);
            p.colorBuffers.push_back(ftk::ImageType::RGBA_F32);
#endif // FTK_API_GL_4_1
            std::vector<std::string> items;
            for (size_t i = 0; i < p.colorBuffers.size(); ++i)
            {
                std::stringstream ss;
                ss << p.colorBuffers[i];
                items.push_back(ss.str());
            }
            p.colorBufferComboBox = ftk::ComboBox::create(context, items);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Minify:", p.minifyComboBox);
            p.layout->addRow("Magnify:", p.magnifyComboBox);
            p.layout->addRow("Video levels:", p.videoLevelsComboBox);
            p.layout->addRow("Alpha blend:", p.alphaBlendComboBox);
            p.layout->addRow("Color buffer:", p.colorBufferComboBox);

            p.imageOptionsObserver = ftk::Observer<ftk::ImageOptions>::create(
                app->getViewportModel()->observeImageOptions(),
                [this](const ftk::ImageOptions& value)
                {
                    _p->alphaBlendComboBox->setCurrentIndex(static_cast<int>(value.alphaBlend));
                });

            p.displayOptionsObserver = ftk::Observer<tl::timeline::DisplayOptions>::create(
                app->getViewportModel()->observeDisplayOptions(),
                [this](const tl::timeline::DisplayOptions& value)
                {
                    FTK_P();
                    p.minifyComboBox->setCurrentIndex(static_cast<int>(value.imageFilters.minify));
                    p.magnifyComboBox->setCurrentIndex(static_cast<int>(value.imageFilters.magnify));
                    p.videoLevelsComboBox->setCurrentIndex(static_cast<int>(value.videoLevels));
                });

            p.colorBufferObserver = ftk::Observer<ftk::ImageType>::create(
                app->getViewportModel()->observeColorBuffer(),
                [this](ftk::ImageType value)
                {
                    FTK_P();
                    int index = -1;
                    const auto i = std::find(p.colorBuffers.begin(), p.colorBuffers.end(), value);
                    if (i != p.colorBuffers.end())
                    {
                        index = i - p.colorBuffers.begin();
                    }
                    _p->colorBufferComboBox->setCurrentIndex(index);
                });

            auto appWeak = std::weak_ptr<App>(app);
            p.minifyComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.imageFilters.minify = static_cast<ftk::ImageFilter>(value);
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.magnifyComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.imageFilters.magnify = static_cast<ftk::ImageFilter>(value);
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.videoLevelsComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.videoLevels = static_cast<ftk::VideoLevels>(value);
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.alphaBlendComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getImageOptions();
                        options.alphaBlend = static_cast<ftk::AlphaBlend>(value);
                        app->getViewportModel()->setImageOptions(options);
                    }
                });

            p.colorBufferComboBox->setIndexCallback(
                [this, appWeak](int value)
                {
                    FTK_P();
                    if (auto app = appWeak.lock())
                    {
                        if (value >= 0 && value < p.colorBuffers.size())
                        {
                            app->getViewportModel()->setColorBuffer(p.colorBuffers[value]);
                        }
                    }
                });
        }

        ViewOptionsWidget::ViewOptionsWidget() :
            _p(new Private)
        {}

        ViewOptionsWidget::~ViewOptionsWidget()
        {}

        std::shared_ptr<ViewOptionsWidget> ViewOptionsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ViewOptionsWidget>(new ViewOptionsWidget);
            out->_init(context, app, parent);
            return out;
        }

        ftk::Size2I ViewOptionsWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void ViewOptionsWidget::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        struct BackgroundWidget::Private
        {
            std::shared_ptr<ftk::ComboBox> typeComboBox;
            std::shared_ptr<ftk::ColorSwatch> solidSwatch;
            std::pair< std::shared_ptr<ftk::ColorSwatch>, std::shared_ptr<ftk::ColorSwatch> > checkersSwatch;
            std::shared_ptr<ftk::IntEditSlider> checkersSizeSlider;
            std::pair< std::shared_ptr<ftk::ColorSwatch>, std::shared_ptr<ftk::ColorSwatch> > gradientSwatch;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::timeline::BackgroundOptions> > backgroundOptionsObserver;
        };

        void BackgroundWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IWidget::_init(context, "djv::app::BackgroundWidget", parent);
            FTK_P();

            p.typeComboBox = ftk::ComboBox::create(
                context,
                tl::timeline::getBackgroundLabels());
            p.typeComboBox->setHStretch(ftk::Stretch::Expanding);

            p.solidSwatch = ftk::ColorSwatch::create(context);
            p.solidSwatch->setEditable(true);
            p.solidSwatch->setHAlign(ftk::HAlign::Left);

            p.checkersSwatch.first = ftk::ColorSwatch::create(context);
            p.checkersSwatch.first->setEditable(true);
            p.checkersSwatch.second = ftk::ColorSwatch::create(context);
            p.checkersSwatch.second->setEditable(true);
            p.checkersSizeSlider = ftk::IntEditSlider::create(context);
            p.checkersSizeSlider->setRange(10, 100);

            p.gradientSwatch.first = ftk::ColorSwatch::create(context);
            p.gradientSwatch.first->setEditable(true);
            p.gradientSwatch.second = ftk::ColorSwatch::create(context);
            p.gradientSwatch.second->setEditable(true);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Type:", p.typeComboBox);
            p.layout->addRow("Color:", p.solidSwatch);
            p.layout->addRow("Color 1:", p.checkersSwatch.first);
            p.layout->addRow("Color 2:", p.checkersSwatch.second);
            p.layout->addRow("Size:", p.checkersSizeSlider);
            p.layout->addRow("Color 1:", p.gradientSwatch.first);
            p.layout->addRow("Color 2:", p.gradientSwatch.second);

            p.backgroundOptionsObserver = ftk::Observer<tl::timeline::BackgroundOptions>::create(
                app->getViewportModel()->observeBackgroundOptions(),
                [this](const tl::timeline::BackgroundOptions& value)
                {
                    _optionsUpdate(value);
                });

            auto appWeak = std::weak_ptr<App>(app);
            p.typeComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getBackgroundOptions();
                        options.type = static_cast<tl::timeline::Background>(value);
                        app->getViewportModel()->setBackgroundOptions(options);
                    }
                });

            p.solidSwatch->setColorCallback(
                [appWeak](const ftk::Color4F& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getBackgroundOptions();
                        options.solidColor = value;
                        app->getViewportModel()->setBackgroundOptions(options);
                    }
                });

            p.checkersSwatch.first->setColorCallback(
                [appWeak](const ftk::Color4F& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getBackgroundOptions();
                        options.checkersColor.first = value;
                        app->getViewportModel()->setBackgroundOptions(options);
                    }
                });

            p.checkersSwatch.second->setColorCallback(
                [appWeak](const ftk::Color4F& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getBackgroundOptions();
                        options.checkersColor.second = value;
                        app->getViewportModel()->setBackgroundOptions(options);
                    }
                });

            p.checkersSizeSlider->setCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getBackgroundOptions();
                        options.checkersSize.w = value;
                        options.checkersSize.h = value;
                        app->getViewportModel()->setBackgroundOptions(options);
                    }
                });

            p.gradientSwatch.first->setColorCallback(
                [appWeak](const ftk::Color4F& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getBackgroundOptions();
                        options.gradientColor.first = value;
                        app->getViewportModel()->setBackgroundOptions(options);
                    }
                });

            p.gradientSwatch.second->setColorCallback(
                [appWeak](const ftk::Color4F& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getBackgroundOptions();
                        options.gradientColor.second = value;
                        app->getViewportModel()->setBackgroundOptions(options);
                    }
                });
        }

        BackgroundWidget::BackgroundWidget() :
            _p(new Private)
        {}

        BackgroundWidget::~BackgroundWidget()
        {}

        std::shared_ptr<BackgroundWidget> BackgroundWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<BackgroundWidget>(new BackgroundWidget);
            out->_init(context, app, parent);
            return out;
        }

        ftk::Size2I BackgroundWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void BackgroundWidget::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void BackgroundWidget::_optionsUpdate(const tl::timeline::BackgroundOptions& value)
        {
            FTK_P();
            p.typeComboBox->setCurrentIndex(static_cast<int>(value.type));
            p.solidSwatch->setColor(value.solidColor);
            p.checkersSwatch.first->setColor(value.checkersColor.first);
            p.checkersSwatch.second->setColor(value.checkersColor.second);
            p.checkersSizeSlider->setValue(value.checkersSize.w);
            p.gradientSwatch.first->setColor(value.gradientColor.first);
            p.gradientSwatch.second->setColor(value.gradientColor.second);

            p.layout->setRowVisible(p.solidSwatch, value.type == tl::timeline::Background::Solid);
            p.layout->setRowVisible(p.checkersSwatch.first, value.type == tl::timeline::Background::Checkers);
            p.layout->setRowVisible(p.checkersSwatch.second, value.type == tl::timeline::Background::Checkers);
            p.layout->setRowVisible(p.checkersSizeSlider, value.type == tl::timeline::Background::Checkers);
            p.layout->setRowVisible(p.gradientSwatch.first, value.type == tl::timeline::Background::Gradient);
            p.layout->setRowVisible(p.gradientSwatch.second, value.type == tl::timeline::Background::Gradient);
        }

        struct OutlineWidget::Private
        {
            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::shared_ptr<ftk::IntEditSlider> widthSlider;
            std::shared_ptr<ftk::ColorSwatch> colorSwatch;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::timeline::ForegroundOptions> > optionsObservers;
        };

        void OutlineWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IWidget::_init(context, "djv::app::OutlineWidget", parent);
            FTK_P();

            p.enabledCheckBox = ftk::CheckBox::create(context);
            p.enabledCheckBox->setHStretch(ftk::Stretch::Expanding);

            p.widthSlider = ftk::IntEditSlider::create(context);
            p.widthSlider->setRange(1, 100);

            p.colorSwatch = ftk::ColorSwatch::create(context);
            p.colorSwatch->setEditable(true);
            p.colorSwatch->setHAlign(ftk::HAlign::Left);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Enabled:", p.enabledCheckBox);
            p.layout->addRow("Width:", p.widthSlider);
            p.layout->addRow("Color:", p.colorSwatch);

            p.optionsObservers = ftk::Observer<tl::timeline::ForegroundOptions>::create(
                app->getViewportModel()->observeForegroundOptions(),
                [this](const tl::timeline::ForegroundOptions& value)
                {
                    _optionsUpdate(value);
                });

            auto appWeak = std::weak_ptr<App>(app);
            p.enabledCheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getForegroundOptions();
                        options.outline.enabled = value;
                        app->getViewportModel()->setForegroundOptions(options);
                    }
                });

            p.widthSlider->setCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getForegroundOptions();
                        options.outline.width = value;
                        app->getViewportModel()->setForegroundOptions(options);
                    }
                });

            p.colorSwatch->setColorCallback(
                [appWeak](const ftk::Color4F& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getForegroundOptions();
                        options.outline.color = value;
                        app->getViewportModel()->setForegroundOptions(options);
                    }
                });
        }

        OutlineWidget::OutlineWidget() :
            _p(new Private)
        {}

        OutlineWidget::~OutlineWidget()
        {}

        std::shared_ptr<OutlineWidget> OutlineWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<OutlineWidget>(new OutlineWidget);
            out->_init(context, app, parent);
            return out;
        }

        ftk::Size2I OutlineWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void OutlineWidget::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void OutlineWidget::_optionsUpdate(const tl::timeline::ForegroundOptions& value)
        {
            FTK_P();
            p.enabledCheckBox->setChecked(value.outline.enabled);
            p.widthSlider->setValue(value.outline.width);
            p.colorSwatch->setColor(value.outline.color);
        }

        struct GridWidget::Private
        {
            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::shared_ptr<ftk::IntEditSlider> sizeSlider;
            std::shared_ptr<ftk::IntEditSlider> lineWidthSlider;
            std::shared_ptr<ftk::ColorSwatch> colorSwatch;
            std::shared_ptr<ftk::ComboBox> labelsComboBox;
            std::shared_ptr<ftk::ColorSwatch> textColorSwatch;
            std::shared_ptr<ftk::ColorSwatch> overlayColorSwatch;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::timeline::ForegroundOptions> > optionsObserver;
        };

        void GridWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IWidget::_init(context, "djv::app::GridWidget", parent);
            FTK_P();

            p.enabledCheckBox = ftk::CheckBox::create(context);
            p.enabledCheckBox->setHStretch(ftk::Stretch::Expanding);

            p.sizeSlider = ftk::IntEditSlider::create(context);
            p.sizeSlider->setRange(1, 1000);

            p.lineWidthSlider = ftk::IntEditSlider::create(context);
            p.lineWidthSlider->setRange(1, 10);

            p.colorSwatch = ftk::ColorSwatch::create(context);
            p.colorSwatch->setEditable(true);
            p.colorSwatch->setHAlign(ftk::HAlign::Left);

            p.labelsComboBox = ftk::ComboBox::create(context, tl::timeline::getGridLabelsLabels());

            p.textColorSwatch = ftk::ColorSwatch::create(context);
            p.textColorSwatch->setEditable(true);
            p.textColorSwatch->setHAlign(ftk::HAlign::Left);

            p.overlayColorSwatch = ftk::ColorSwatch::create(context);
            p.overlayColorSwatch->setEditable(true);
            p.overlayColorSwatch->setHAlign(ftk::HAlign::Left);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Enabled:", p.enabledCheckBox);
            p.layout->addRow("Size:", p.sizeSlider);
            p.layout->addRow("Line width:", p.lineWidthSlider);
            p.layout->addRow("Color:", p.colorSwatch);
            p.layout->addRow("Labels:", p.labelsComboBox);
            p.layout->addRow("Text color:", p.textColorSwatch);
            p.layout->addRow("Overlay color:", p.overlayColorSwatch);

            p.optionsObserver = ftk::Observer<tl::timeline::ForegroundOptions>::create(
                app->getViewportModel()->observeForegroundOptions(),
                [this](const tl::timeline::ForegroundOptions& value)
                {
                    FTK_P();
                    p.enabledCheckBox->setChecked(value.grid.enabled);
                    p.sizeSlider->setValue(value.grid.size);
                    p.lineWidthSlider->setValue(value.grid.lineWidth);
                    p.colorSwatch->setColor(value.grid.color);
                    p.labelsComboBox->setCurrentIndex(static_cast<int>(value.grid.labels));
                    p.textColorSwatch->setColor(value.grid.textColor);
                    p.overlayColorSwatch->setColor(value.grid.overlayColor);
                });

            auto appWeak = std::weak_ptr<App>(app);
            p.enabledCheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getForegroundOptions();
                        options.grid.enabled = value;
                        app->getViewportModel()->setForegroundOptions(options);
                    }
                });

            p.sizeSlider->setCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getForegroundOptions();
                        options.grid.size = value;
                        app->getViewportModel()->setForegroundOptions(options);
                    }
                });

            p.lineWidthSlider->setCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getForegroundOptions();
                        options.grid.lineWidth = value;
                        app->getViewportModel()->setForegroundOptions(options);
                    }
                });

            p.colorSwatch->setColorCallback(
                [appWeak](const ftk::Color4F& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getForegroundOptions();
                        options.grid.color = value;
                        app->getViewportModel()->setForegroundOptions(options);
                    }
                });

            p.labelsComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getForegroundOptions();
                        options.grid.labels = static_cast<tl::timeline::GridLabels>(value);
                        app->getViewportModel()->setForegroundOptions(options);
                    }
                });

            p.textColorSwatch->setColorCallback(
                [appWeak](const ftk::Color4F& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getForegroundOptions();
                        options.grid.textColor = value;
                        app->getViewportModel()->setForegroundOptions(options);
                    }
                });

            p.overlayColorSwatch->setColorCallback(
                [appWeak](const ftk::Color4F& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getForegroundOptions();
                        options.grid.overlayColor = value;
                        app->getViewportModel()->setForegroundOptions(options);
                    }
                });
        }

        GridWidget::GridWidget() :
            _p(new Private)
        {}

        GridWidget::~GridWidget()
        {}

        std::shared_ptr<GridWidget> GridWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<GridWidget>(new GridWidget);
            out->_init(context, app, parent);
            return out;
        }

        ftk::Size2I GridWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void GridWidget::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        struct ViewTool::Private
        {
            std::shared_ptr<ViewPosZoomWidget> viewPosZoomWidget;
            std::shared_ptr<ViewOptionsWidget> viewOptionsWidget;
            std::shared_ptr<BackgroundWidget> backgroundWidget;
            std::shared_ptr<OutlineWidget> outlineWidget;
            std::shared_ptr<GridWidget> gridWidget;
            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
        };

        void ViewTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                Tool::View,
                "djv::app::ViewTool",
                parent);
            FTK_P();

            p.viewPosZoomWidget = ViewPosZoomWidget::create(context, app, mainWindow);
            p.viewOptionsWidget = ViewOptionsWidget::create(context, app);
            p.backgroundWidget = BackgroundWidget::create(context, app);
            p.outlineWidget = OutlineWidget::create(context, app);
            p.gridWidget = GridWidget::create(context, app);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);
            p.bellows["PosZoom"] = ftk::Bellows::create(context, "Position and Zoom", layout);
            p.bellows["PosZoom"]->setWidget(p.viewPosZoomWidget);
            p.bellows["Options"] = ftk::Bellows::create(context, "Options", layout);
            p.bellows["Options"]->setWidget(p.viewOptionsWidget);
            p.bellows["Background"] = ftk::Bellows::create(context, "Background", layout);
            p.bellows["Background"]->setWidget(p.backgroundWidget);
            p.bellows["Outline"] = ftk::Bellows::create(context, "Outline", layout);
            p.bellows["Outline"]->setWidget(p.outlineWidget);
            p.bellows["Grid"] = ftk::Bellows::create(context, "Grid", layout);
            p.bellows["Grid"]->setWidget(p.gridWidget);
            auto scrollWidget = ftk::ScrollWidget::create(context);
            scrollWidget->setBorder(false);
            scrollWidget->setWidget(layout);
            _setWidget(scrollWidget);

            _loadSettings(p.bellows);
        }

        ViewTool::ViewTool() :
            _p(new Private)
        {}

        ViewTool::~ViewTool()
        {
            _saveSettings(_p->bellows);
        }

        std::shared_ptr<ViewTool> ViewTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ViewTool>(new ViewTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }
    }
}
