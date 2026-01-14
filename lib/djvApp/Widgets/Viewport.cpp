// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp//Widgets/Viewport.h>

#include <djvApp/Models/ColorModel.h>
#include <djvApp/Models/FilesModel.h>
#include <djvApp/Models/SettingsModel.h>
#include <djvApp/Models/TimeUnitsModel.h>
#include <djvApp/Models/ViewportModel.h>
#include <djvApp/App.h>

#include <tlRender/Timeline/Util.h>

#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/GridLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/Spacer.h>
#include <ftk/Core/Format.h>

#include <regex>

namespace djv
{
    namespace app
    {
        struct Viewport::Private
        {
            std::weak_ptr<App> app;
            bool hud = false;
            ftk::Path path;
            OTIO_NS::RationalTime currentTime = tl::invalidTime;
            double fps = 0.0;
            size_t droppedFrames = 0;
            size_t videoFramesSize = 0;
            ftk::ImageOptions imageOptions;
            tl::DisplayOptions displayOptions;
            tl::PlayerCacheInfo cacheInfo;
            MouseActionBinding pickBinding =
                MouseActionBinding(ftk::MouseButton::Left);
            MouseActionBinding frameShuttleBinding =
                MouseActionBinding(ftk::MouseButton::Left, ftk::KeyModifier::Shift);
            float frameShuttleScale = 1.F;
            std::shared_ptr<ftk::Observable<ftk::V2I> > pick;
            std::shared_ptr<ftk::Observable<ftk::V2I> > samplePos;
            std::shared_ptr<ftk::Observable<ftk::Color4F> > colorSample;

            std::shared_ptr<ftk::Label> fileNameLabel;
            std::shared_ptr<ftk::Label> timeLabel;
            std::shared_ptr<ftk::ColorSwatch> colorPickerSwatch;
            std::shared_ptr<ftk::Label> colorPickerLabel;
            std::shared_ptr<ftk::Label> cacheLabel;
            std::shared_ptr<ftk::GridLayout> hudLayout;

            std::shared_ptr<ftk::Observer<OTIO_NS::RationalTime> > currentTimeObserver;
            std::shared_ptr<ftk::ListObserver<tl::VideoFrame> > videoObserver;
            std::shared_ptr<ftk::Observer<tl::PlayerCacheInfo> > cacheObserver;
            std::shared_ptr<ftk::Observer<double> > fpsObserver;
            std::shared_ptr<ftk::Observer<size_t> > droppedFramesObserver;
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageOptions> > imageOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::BackgroundOptions> > bgOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > fgOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageType> > colorBufferObserver;
            std::shared_ptr<ftk::Observer<bool> > hudObserver;
            std::shared_ptr<ftk::Observer<tl::TimeUnits> > timeUnitsObserver;
            std::shared_ptr<ftk::Observer<MouseSettings> > mouseSettingsObserver;

            enum class MouseMode
            {
                None,
                Shuttle,
                Picker
            };
            struct MouseData
            {
                MouseMode mode = MouseMode::None;
                OTIO_NS::RationalTime shuttleStart = tl::invalidTime;
            };
            MouseData mouse;
        };

        void Viewport::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            tl::ui::Viewport::_init(context, parent);
            FTK_P();

            p.app = app;

            p.pick = ftk::Observable<ftk::V2I>::create();
            p.samplePos = ftk::Observable<ftk::V2I>::create();
            p.colorSample = ftk::Observable<ftk::Color4F>::create();

            p.fileNameLabel = ftk::Label::create(context);
            p.fileNameLabel->setFontRole(ftk::FontRole::Mono);
            p.fileNameLabel->setMarginRole(ftk::SizeRole::MarginInside);
            p.fileNameLabel->setBackgroundRole(ftk::ColorRole::Overlay);

            p.timeLabel = ftk::Label::create(context);
            p.timeLabel->setFontRole(ftk::FontRole::Mono);
            p.timeLabel->setMarginRole(ftk::SizeRole::MarginInside);
            p.timeLabel->setBackgroundRole(ftk::ColorRole::Overlay);
            p.timeLabel->setHAlign(ftk::HAlign::Right);

            p.colorPickerSwatch = ftk::ColorSwatch::create(context);
            p.colorPickerSwatch->setSizeRole(ftk::SizeRole::MarginLarge);
            p.colorPickerLabel = ftk::Label::create(context);
            p.colorPickerLabel->setFontRole(ftk::FontRole::Mono);

            p.cacheLabel = ftk::Label::create(context);
            p.cacheLabel->setFontRole(ftk::FontRole::Mono);
            p.cacheLabel->setMarginRole(ftk::SizeRole::MarginInside);
            p.cacheLabel->setBackgroundRole(ftk::ColorRole::Overlay);
            p.cacheLabel->setHAlign(ftk::HAlign::Right);

            p.hudLayout = ftk::GridLayout::create(context, shared_from_this());
            p.hudLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            p.hudLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.fileNameLabel->setParent(p.hudLayout);
            p.hudLayout->setGridPos(p.fileNameLabel, 0, 0);
            p.timeLabel->setParent(p.hudLayout);
            p.hudLayout->setGridPos(p.timeLabel, 0, 2);

            auto spacer = ftk::Spacer::create(context, ftk::Orientation::Vertical, p.hudLayout);
            spacer->setStretch(ftk::Stretch::Expanding);
            p.hudLayout->setGridPos(spacer, 1, 1);

            auto hLayout = ftk::HorizontalLayout::create(context, p.hudLayout);
            p.hudLayout->setGridPos(hLayout, 2, 0);
            hLayout->setMarginRole(ftk::SizeRole::MarginInside);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            hLayout->setBackgroundRole(ftk::ColorRole::Overlay);
            p.colorPickerSwatch->setParent(hLayout);
            p.colorPickerLabel->setParent(hLayout);
            p.cacheLabel->setParent(p.hudLayout);
            p.hudLayout->setGridPos(p.cacheLabel, 2, 2);

            p.fpsObserver = ftk::Observer<double>::create(
                observeFPS(),
                [this](double value)
                {
                    _p->fps = value;
                    _hudUpdate();
                });

            p.droppedFramesObserver = ftk::Observer<size_t>::create(
                observeDroppedFrames(),
                [this](size_t value)
                {
                    _p->droppedFrames = value;
                    _hudUpdate();
                });

            p.compareOptionsObserver = ftk::Observer<tl::CompareOptions>::create(
                app->getFilesModel()->observeCompareOptions(),
                [this](const tl::CompareOptions& value)
                {
                    setCompareOptions(value);
                });

            p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                app->getColorModel()->observeOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                   setOCIOOptions(value);
                });

            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                app->getColorModel()->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                   setLUTOptions(value);
                });

            p.imageOptionsObserver = ftk::Observer<ftk::ImageOptions>::create(
                app->getViewportModel()->observeImageOptions(),
                [this](const ftk::ImageOptions& value)
                {
                    _p->imageOptions = value;
                    _videoUpdate();
                });

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                app->getViewportModel()->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    _p->displayOptions = value;
                    _videoUpdate();
                });

            p.bgOptionsObserver = ftk::Observer<tl::BackgroundOptions>::create(
                app->getViewportModel()->observeBackgroundOptions(),
                [this](const tl::BackgroundOptions& value)
                {
                    setBackgroundOptions(value);
                });

            p.fgOptionsObserver = ftk::Observer<tl::ForegroundOptions>::create(
                app->getViewportModel()->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
                {
                    setForegroundOptions(value);
                });

            p.colorBufferObserver = ftk::Observer<ftk::ImageType>::create(
                app->getViewportModel()->observeColorBuffer(),
                [this](ftk::ImageType value)
                {
                    setColorBuffer(value);
                    _hudUpdate();
                });

            p.hudObserver = ftk::Observer<bool>::create(
                app->getViewportModel()->observeHUD(),
                [this](bool value)
                {
                    _p->hud = value;
                    _hudUpdate();
                });

            p.timeUnitsObserver = ftk::Observer<tl::TimeUnits>::create(
                app->getTimeUnitsModel()->observeTimeUnits(),
                [this](tl::TimeUnits value)
                {
                    _hudUpdate();
                });

            p.mouseSettingsObserver = ftk::Observer<MouseSettings>::create(
                app->getSettingsModel()->observeMouse(),
                [this](const MouseSettings& value)
                {
                    FTK_P();
                    auto i = value.bindings.find(MouseAction::PanView);
                    setPanBinding(
                        i != value.bindings.end() ? i->second.button : ftk::MouseButton::None,
                        i != value.bindings.end() ? i->second.modifier : ftk::KeyModifier::None);
                    i = value.bindings.find(MouseAction::CompareWipe);
                    setWipeBinding(
                        i != value.bindings.end() ? i->second.button : ftk::MouseButton::None,
                        i != value.bindings.end() ? i->second.modifier : ftk::KeyModifier::None);
                    i = value.bindings.find(MouseAction::Pick);
                    p.pickBinding = i != value.bindings.end() ? i->second : MouseActionBinding();
                    i = value.bindings.find(MouseAction::FrameShuttle);
                    p.frameShuttleBinding = i != value.bindings.end() ? i->second : MouseActionBinding();
                    p.frameShuttleScale = value.frameShuttleScale;
                });
        }

        Viewport::Viewport() :
            _p(new Private)
        {}

        Viewport::~Viewport()
        {}

        std::shared_ptr<Viewport> Viewport::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<Viewport>(new Viewport);
            out->_init(context, app, parent);
            return out;
        }

        std::shared_ptr<ftk::IObservable<ftk::V2I> > Viewport::observePick() const
        {
            return _p->pick;
        }

        std::shared_ptr<ftk::IObservable<ftk::V2I> > Viewport::observeSamplePos() const
        {
            return _p->samplePos;
        }

        std::shared_ptr<ftk::IObservable<ftk::Color4F> > Viewport::observeColorSample() const
        {
            return _p->colorSample;
        }

        void Viewport::setPlayer(const std::shared_ptr<tl::Player>& player)
        {
            tl::ui::Viewport::setPlayer(player);
            FTK_P();
            if (player)
            {
                p.path = player->getPath();

                p.currentTimeObserver = ftk::Observer<OTIO_NS::RationalTime>::create(
                    player->observeCurrentTime(),
                    [this](const OTIO_NS::RationalTime& value)
                    {
                        _p->currentTime = value;
                        _hudUpdate();
                    });

                p.videoObserver = ftk::ListObserver<tl::VideoFrame>::create(
                    player->observeCurrentVideo(),
                    [this](const std::vector<tl::VideoFrame>& value)
                    {
                        _p->videoFramesSize = value.size();
                        _videoUpdate();
                    });

                p.cacheObserver = ftk::Observer<tl::PlayerCacheInfo>::create(
                    player->observeCacheInfo(),
                    [this](const tl::PlayerCacheInfo& value)
                    {
                        _p->cacheInfo = value;
                        _hudUpdate();
                    });
            }
            else
            {
                p.path = ftk::Path();
                p.currentTime = tl::invalidTime;
                p.currentTimeObserver.reset();
                p.videoObserver.reset();
                p.cacheInfo = tl::PlayerCacheInfo();
                p.cacheObserver.reset();
                _hudUpdate();
            }
        }

        ftk::Size2I Viewport::getSizeHint() const
        {
            return _p->hudLayout->getSizeHint();
        }

        void Viewport::setGeometry(const ftk::Box2I& value)
        {
            tl::ui::Viewport::setGeometry(value);
            FTK_P();
            p.hudLayout->setGeometry(value);
        }

        void Viewport::mouseMoveEvent(ftk::MouseMoveEvent& event)
        {
            tl::ui::Viewport::mouseMoveEvent(event);
            FTK_P();
            switch (p.mouse.mode)
            {
            case Private::MouseMode::Shuttle:
                if (auto player = getPlayer())
                {
                    const OTIO_NS::RationalTime offset = OTIO_NS::RationalTime(
                        (event.pos.x - _getMousePressPos().x) * .05F * p.frameShuttleScale,
                        p.mouse.shuttleStart.rate()).round();
                    const OTIO_NS::TimeRange& timeRange = player->getTimeRange();
                    OTIO_NS::RationalTime t = p.mouse.shuttleStart + offset;
                    if (t < timeRange.start_time())
                    {
                        t = timeRange.end_time_exclusive() - (timeRange.start_time() - t);
                    }
                    else if (t > timeRange.end_time_exclusive())
                    {
                        t = timeRange.start_time() + (t - timeRange.end_time_exclusive());
                    }
                    player->seek(t);
                }
                break;
            case Private::MouseMode::Picker:
                if (auto app = p.app.lock())
                {
                    const ftk::Box2I& g = getGeometry();
                    const ftk::V2I pos = event.pos - g.min;
                    if (p.samplePos->setIfChanged(pos))
                    {
                        p.colorSample->setIfChanged(getColorSample(pos));
                        p.pick->setIfChanged((pos - getViewPos()) / getViewZoom());
                        _hudUpdate();
                    }
                }
                break;
            default: break;
            }
        }

        void Viewport::mousePressEvent(ftk::MouseClickEvent& event)
        {
            tl::ui::Viewport::mousePressEvent(event);
            FTK_P();
            if (p.pickBinding.button == event.button &&
                ftk::checkKeyModifier(p.pickBinding.modifier, event.modifiers))
            {
                p.mouse.mode = Private::MouseMode::Picker;
                const ftk::Box2I& g = getGeometry();
                const ftk::V2I pos = event.pos - g.min;
                if (p.samplePos->setIfChanged(pos))
                {
                    p.colorSample->setIfChanged(getColorSample(pos));
                    p.pick->setIfChanged((pos - getViewPos()) / getViewZoom());
                    _hudUpdate();
                }
            }
            else if (p.frameShuttleBinding.button == event.button &&
                ftk::checkKeyModifier(p.frameShuttleBinding.modifier, event.modifiers))
            {
                p.mouse.mode = Private::MouseMode::Shuttle;
                if (auto player = getPlayer())
                {
                    player->stop();
                    p.mouse.shuttleStart = player->getCurrentTime();
                }
            }
        }

        void Viewport::mouseReleaseEvent(ftk::MouseClickEvent& event)
        {
            tl::ui::Viewport::mouseReleaseEvent(event);
            FTK_P();
            p.mouse = Private::MouseData();
        }

        void Viewport::_videoUpdate()
        {
            FTK_P();
            std::vector<ftk::ImageOptions> imageOptions;
            std::vector<tl::DisplayOptions> displayOptions;
            for (size_t i = 0; i < p.videoFramesSize; ++i)
            {
                imageOptions.push_back(p.imageOptions);
                displayOptions.push_back(p.displayOptions);
            }
            setImageOptions(imageOptions);
            setDisplayOptions(displayOptions);
        }

        void Viewport::_hudUpdate()
        {
            FTK_P();

            std::string s = p.path.getFileName();
            p.fileNameLabel->setText(!s.empty() ? s : "(No file)");

            s = std::string();
            if (auto app = p.app.lock())
            {
                auto timeUnitsModel = app->getTimeUnitsModel();
                s = timeUnitsModel->getLabel(p.currentTime);
            }
            p.timeLabel->setText(ftk::Format("Time: {0}, {1} FPS, {2} dropped").
                arg(s).
                arg(p.fps, 2, 5).
                arg(static_cast<int>(p.droppedFrames), 3));

            const auto& colorSample = p.colorSample->get();
            p.colorPickerSwatch->setColor(colorSample);
            p.colorPickerLabel->setText(
                ftk::Format("Color: {0} {1} {2} {3}, Pixel: {4}").
                arg(colorSample.r, 2).
                arg(colorSample.g, 2).
                arg(colorSample.b, 2).
                arg(colorSample.a, 2).
                arg(p.pick->get()));

            p.cacheLabel->setText(
                ftk::Format("Cache: {0}% V, {1}% A").
                arg(static_cast<int>(p.cacheInfo.videoPercentage), 3).
                arg(static_cast<int>(p.cacheInfo.audioPercentage), 3));

            p.hudLayout->setVisible(p.hud);
        }
    }
}
