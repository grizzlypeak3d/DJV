// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/ViewWidgets.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>
#include <djv/App/Viewport.h>
#include <djv/Models/ViewportModel.h>

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
    namespace ui
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
            const std::shared_ptr<tl::ui::Viewport>& viewport,
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

            std::weak_ptr<tl::ui::Viewport> viewportWeak(viewport);
            p.posXEdit->setCallback(
                [this, viewportWeak](int value)
                {
                    if (!_p->updating)
                    {
                        if (auto viewport = viewportWeak.lock())
                        {
                            const ftk::V2I& pos = viewport->getViewPos();
                            const double zoom = viewport->getViewZoom();
                            viewport->setViewPosAndZoom(ftk::V2I(value, pos.y), zoom);
                        }
                    }
                });

            p.posYEdit->setCallback(
                [this, viewportWeak](int value)
                {
                    if (!_p->updating)
                    {
                        if (auto viewport = viewportWeak.lock())
                        {
                            const ftk::V2I& pos = viewport->getViewPos();
                            const double zoom = viewport->getViewZoom();
                            viewport->setViewPosAndZoom(ftk::V2I(pos.x, value), zoom);
                        }
                    }
                });

            p.zoomEdit->setCallback(
                [this, viewportWeak](double value)
                {
                    if (!_p->updating)
                    {
                        if (auto viewport = viewportWeak.lock())
                        {
                            const ftk::Box2I& g = viewport->getGeometry();
                            const ftk::V2I focus(g.w() / 2, g.h() / 2);
                            viewport->setViewZoom(value, focus);
                        }
                    }
                });

            p.posZoomObserver = ftk::Observer<std::pair<ftk::V2I, double> >::create(
                viewport->observeViewPosAndZoom(),
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
            const std::shared_ptr<tl::ui::Viewport>& viewport,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ViewPosZoomWidget>(new ViewPosZoomWidget);
            out->_init(context, viewport, parent);
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
            std::vector<ftk::gl::TextureType> colorBuffers;

            std::shared_ptr<ftk::ComboBox> minifyComboBox;
            std::shared_ptr<ftk::ComboBox> magnifyComboBox;
            std::shared_ptr<ftk::ComboBox> videoLevelsComboBox;
            std::shared_ptr<ftk::ComboBox> alphaBlendComboBox;
            std::shared_ptr<ftk::ComboBox> colorBufferComboBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<ftk::ImageOptions> > imageOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::gl::TextureType> > colorBufferObserver;
        };

        void ViewOptionsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
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

            p.colorBuffers.push_back(ftk::gl::TextureType::RGBA_U8);
#if defined(FTK_API_GL_4_1)
            p.colorBuffers.push_back(ftk::gl::TextureType::RGBA_F16);
            p.colorBuffers.push_back(ftk::gl::TextureType::RGBA_F32);
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
                viewportModel->observeImageOptions(),
                [this](const ftk::ImageOptions& value)
                {
                    FTK_P();
                    p.videoLevelsComboBox->setCurrentIndex(static_cast<int>(value.videoLevels));
                    p.alphaBlendComboBox->setCurrentIndex(static_cast<int>(value.alphaBlend));
                });

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                viewportModel->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    FTK_P();
                    p.minifyComboBox->setCurrentIndex(static_cast<int>(value.imageFilters.minify));
                    p.magnifyComboBox->setCurrentIndex(static_cast<int>(value.imageFilters.magnify));
                });

            p.colorBufferObserver = ftk::Observer<ftk::gl::TextureType>::create(
                viewportModel->observeColorBuffer(),
                [this](ftk::gl::TextureType value)
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

            p.minifyComboBox->setIndexCallback(
                [viewportModel](int value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    options.imageFilters.minify = static_cast<ftk::ImageFilter>(value);
                    viewportModel->setDisplayOptions(options);
                });

            p.magnifyComboBox->setIndexCallback(
                [viewportModel](int value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    options.imageFilters.magnify = static_cast<ftk::ImageFilter>(value);
                    viewportModel->setDisplayOptions(options);
                });

            p.videoLevelsComboBox->setIndexCallback(
                [viewportModel](int value)
                {
                    auto options = viewportModel->getImageOptions();
                    options.videoLevels = static_cast<ftk::InputVideoLevels>(value);
                    viewportModel->setImageOptions(options);
                });

            p.alphaBlendComboBox->setIndexCallback(
                [viewportModel](int value)
                {
                    auto options = viewportModel->getImageOptions();
                    options.alphaBlend = static_cast<ftk::AlphaBlend>(value);
                    viewportModel->setImageOptions(options);
                });

            p.colorBufferComboBox->setIndexCallback(
                [this, viewportModel](int value)
                {
                    FTK_P();
                    if (value >= 0 && value < p.colorBuffers.size())
                    {
                        viewportModel->setColorBuffer(p.colorBuffers[value]);
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
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ViewOptionsWidget>(new ViewOptionsWidget);
            out->_init(context, viewportModel, parent);
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

            std::shared_ptr<ftk::Observer<tl::BackgroundOptions> > backgroundOptionsObserver;
        };

        void BackgroundWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IWidget::_init(context, "djv::app::BackgroundWidget", parent);
            FTK_P();

            p.typeComboBox = ftk::ComboBox::create(
                context,
                tl::getBackgroundLabels());
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

            p.backgroundOptionsObserver = ftk::Observer<tl::BackgroundOptions>::create(
                viewportModel->observeBackgroundOptions(),
                [this](const tl::BackgroundOptions& value)
                {
                    _optionsUpdate(value);
                });

            p.typeComboBox->setIndexCallback(
                [viewportModel](int value)
                {
                    auto options = viewportModel->getBackgroundOptions();
                    options.type = static_cast<tl::Background>(value);
                    viewportModel->setBackgroundOptions(options);
                });

            p.solidSwatch->setCallback(
                [viewportModel](const ftk::Color4F& value)
                {
                    auto options = viewportModel->getBackgroundOptions();
                    options.solidColor = value;
                    viewportModel->setBackgroundOptions(options);
                });

            p.checkersSwatch.first->setCallback(
                [viewportModel](const ftk::Color4F& value)
                {
                    auto options = viewportModel->getBackgroundOptions();
                    options.checkersColor.first = value;
                    viewportModel->setBackgroundOptions(options);
                });

            p.checkersSwatch.second->setCallback(
                [viewportModel](const ftk::Color4F& value)
                {
                    auto options = viewportModel->getBackgroundOptions();
                    options.checkersColor.second = value;
                    viewportModel->setBackgroundOptions(options);
                });

            p.checkersSizeSlider->setCallback(
                [viewportModel](int value)
                {
                    auto options = viewportModel->getBackgroundOptions();
                    options.checkersSize.w = value;
                    options.checkersSize.h = value;
                    viewportModel->setBackgroundOptions(options);
                });

            p.gradientSwatch.first->setCallback(
                [viewportModel](const ftk::Color4F& value)
                {
                    auto options = viewportModel->getBackgroundOptions();
                    options.gradientColor.first = value;
                    viewportModel->setBackgroundOptions(options);
                });

            p.gradientSwatch.second->setCallback(
                [viewportModel](const ftk::Color4F& value)
                {
                    auto options = viewportModel->getBackgroundOptions();
                    options.gradientColor.second = value;
                    viewportModel->setBackgroundOptions(options);
                });
        }

        BackgroundWidget::BackgroundWidget() :
            _p(new Private)
        {}

        BackgroundWidget::~BackgroundWidget()
        {}

        std::shared_ptr<BackgroundWidget> BackgroundWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<BackgroundWidget>(new BackgroundWidget);
            out->_init(context, viewportModel, parent);
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

        void BackgroundWidget::_optionsUpdate(const tl::BackgroundOptions& value)
        {
            FTK_P();
            p.typeComboBox->setCurrentIndex(static_cast<int>(value.type));
            p.solidSwatch->setColor(value.solidColor);
            p.checkersSwatch.first->setColor(value.checkersColor.first);
            p.checkersSwatch.second->setColor(value.checkersColor.second);
            p.checkersSizeSlider->setValue(value.checkersSize.w);
            p.gradientSwatch.first->setColor(value.gradientColor.first);
            p.gradientSwatch.second->setColor(value.gradientColor.second);

            p.layout->setRowVisible(p.solidSwatch, value.type == tl::Background::Solid);
            p.layout->setRowVisible(p.checkersSwatch.first, value.type == tl::Background::Checkers);
            p.layout->setRowVisible(p.checkersSwatch.second, value.type == tl::Background::Checkers);
            p.layout->setRowVisible(p.checkersSizeSlider, value.type == tl::Background::Checkers);
            p.layout->setRowVisible(p.gradientSwatch.first, value.type == tl::Background::Gradient);
            p.layout->setRowVisible(p.gradientSwatch.second, value.type == tl::Background::Gradient);
        }

        struct OutlineWidget::Private
        {
            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::shared_ptr<ftk::IntEditSlider> widthSlider;
            std::shared_ptr<ftk::ColorSwatch> colorSwatch;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > optionsObservers;
        };

        void OutlineWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
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

            p.optionsObservers = ftk::Observer<tl::ForegroundOptions>::create(
                viewportModel->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
                {
                    _optionsUpdate(value);
                });

            p.enabledCheckBox->setCheckedCallback(
                [viewportModel](bool value)
                {
                    auto options = viewportModel->getForegroundOptions();
                    options.outline.enabled = value;
                    viewportModel->setForegroundOptions(options);
                });

            p.widthSlider->setCallback(
                [viewportModel](int value)
                {
                    auto options = viewportModel->getForegroundOptions();
                    options.outline.width = value;
                    viewportModel->setForegroundOptions(options);
                });

            p.colorSwatch->setCallback(
                [viewportModel](const ftk::Color4F& value)
                {
                    auto options = viewportModel->getForegroundOptions();
                    options.outline.color = value;
                    viewportModel->setForegroundOptions(options);
                });
        }

        OutlineWidget::OutlineWidget() :
            _p(new Private)
        {}

        OutlineWidget::~OutlineWidget()
        {}

        std::shared_ptr<OutlineWidget> OutlineWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<OutlineWidget>(new OutlineWidget);
            out->_init(context, viewportModel, parent);
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

        void OutlineWidget::_optionsUpdate(const tl::ForegroundOptions& value)
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

            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > optionsObserver;
        };

        void GridWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
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

            p.labelsComboBox = ftk::ComboBox::create(context, tl::getGridLabelsLabels());

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

            p.optionsObserver = ftk::Observer<tl::ForegroundOptions>::create(
                viewportModel->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
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

            p.enabledCheckBox->setCheckedCallback(
                [viewportModel](bool value)
                {
                    auto options = viewportModel->getForegroundOptions();
                    options.grid.enabled = value;
                    viewportModel->setForegroundOptions(options);
                });

            p.sizeSlider->setCallback(
                [viewportModel](int value)
                {
                    auto options = viewportModel->getForegroundOptions();
                    options.grid.size = value;
                    viewportModel->setForegroundOptions(options);
                });

            p.lineWidthSlider->setCallback(
                [viewportModel](int value)
                {
                    auto options = viewportModel->getForegroundOptions();
                    options.grid.lineWidth = value;
                    viewportModel->setForegroundOptions(options);
                });

            p.colorSwatch->setCallback(
                [viewportModel](const ftk::Color4F& value)
                {
                    auto options = viewportModel->getForegroundOptions();
                    options.grid.color = value;
                    viewportModel->setForegroundOptions(options);
                });

            p.labelsComboBox->setIndexCallback(
                [viewportModel](int value)
                {
                    auto options = viewportModel->getForegroundOptions();
                    options.grid.labels = static_cast<tl::GridLabels>(value);
                    viewportModel->setForegroundOptions(options);
                });

            p.textColorSwatch->setCallback(
                [viewportModel](const ftk::Color4F& value)
                {
                    auto options = viewportModel->getForegroundOptions();
                    options.grid.textColor = value;
                    viewportModel->setForegroundOptions(options);
                });

            p.overlayColorSwatch->setCallback(
                [viewportModel](const ftk::Color4F& value)
                {
                    auto options = viewportModel->getForegroundOptions();
                    options.grid.overlayColor = value;
                    viewportModel->setForegroundOptions(options);
                });
        }

        GridWidget::GridWidget() :
            _p(new Private)
        {}

        GridWidget::~GridWidget()
        {}

        std::shared_ptr<GridWidget> GridWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<GridWidget>(new GridWidget);
            out->_init(context, viewportModel, parent);
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
    }
}
