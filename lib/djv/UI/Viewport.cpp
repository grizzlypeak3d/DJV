// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/Viewport.h>

#include <djv/Models/ColorModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/Timeline/Util.h>

#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/SysLogModel.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Spacer.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>
#include <ftk/Core/Timer.h>

#include <algorithm>

#include <regex>

namespace djv
{
    namespace ui
    {
        namespace
        {
            // Long enough for a short path, short enough that the message does
            // not run the width of the viewport. The untruncated text is in
            // the messages tool.
            const size_t toastTextLength = 80;

            const std::chrono::seconds toastTimeout(5);
        }

        struct Viewport::Private
        {
            std::weak_ptr<models::ViewportModel> viewportModel;
            std::weak_ptr<models::TimeUnitsModel> timeUnitsModel;
            models::HUDOptions hudOptions;
            ftk::Path path;
            tl::IOInfo ioInfo;
            std::optional<OTIO_NS::RationalTime> currentTime;
            double fps = 0.0;
            size_t droppedFrames = 0;
            size_t videoFramesSize = 0;
            std::vector<std::string> ocioInputs;
            // Whether the picture stands in for a frame the media does not
            // have, and the frame it repeats when there is one.
            bool missing = false;
            std::optional<int64_t> heldFrom;
            ftk::ImageOptions imageOptions;
            tl::DisplayOptions displayOptions;
            tl::PlayerCacheInfo cacheInfo;
            double viewZoom = 0.0;
            models::MouseActionBinding frameShuttleBinding =
                models::MouseActionBinding(ftk::MouseButton::Left, ftk::KeyModifier::Shift);
            float frameShuttleScale = 1.F;

            std::shared_ptr<ftk::Label> fileNameLabel;
            std::shared_ptr<ftk::Label> cacheLabel;
            std::shared_ptr<ftk::Label> timeLabel;
            std::shared_ptr<ftk::Label> viewZoomLabel;
            std::shared_ptr<ftk::ColorSwatch> colorPickerSwatch;
            std::shared_ptr<ftk::Label> colorPickerLabel;
            std::shared_ptr<ftk::Label> infoLabel;
            std::shared_ptr<ftk::Label> renderLabel;
            std::map<models::HUDItem, std::shared_ptr<ftk::IWidget> > hudWidgets;
            tl::Compare compare = tl::Compare::None;
            std::shared_ptr<models::FilesModelItem> a;
            std::vector<std::shared_ptr<models::FilesModelItem> > b;
            std::shared_ptr<ftk::Label> compareLabel;

            bool toastActive = false;
            bool hudActive = true;
            std::shared_ptr<ftk::Label> toastLabel;
            std::shared_ptr<ftk::Timer> toastTimer;
            std::shared_ptr<ftk::VerticalLayout> hudLayout;
            std::map<models::HUDPos, std::shared_ptr<ftk::VerticalLayout> > hudLayouts;

            std::shared_ptr<ftk::Observer<OTIO_NS::RationalTime> > currentTimeObserver;
            std::shared_ptr<ftk::Observer<std::string> > mediaReferenceKeyObserver;
            std::shared_ptr<ftk::ListObserver<tl::VideoFrame> > videoObserver;
            std::shared_ptr<ftk::Observer<tl::PlayerCacheInfo> > cacheObserver;
            std::shared_ptr<ftk::Observer<double> > fpsObserver;
            std::shared_ptr<ftk::Observer<size_t> > droppedFramesObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<models::FilesModelItem> > > aObserver;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > bObserver;
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<std::vector<std::string> > > resolvedInputsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageOptions> > imageOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::BackgroundOptions> > bgOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > fgOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::gl::TextureType> > colorBufferObserver;
            std::shared_ptr<ftk::Observer<double> > viewZoomObserver;
            std::shared_ptr<ftk::ListObserver<ftk::LogItem> > messagesObserver;
            std::shared_ptr<ftk::Observer<models::HUDOptions> > hudOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::TimeUnits> > timeUnitsObserver;
            std::shared_ptr<ftk::Observer<models::MouseSettings> > mouseSettingsObserver;
            std::shared_ptr<ftk::Observer<std::optional<ftk::V2I> > > pickObserver;
            std::shared_ptr<ftk::Observer<std::optional<ftk::Color4F> > > colorSampleObserver;

            enum class MouseMode
            {
                None,
                Shuttle
            };
            struct MouseData
            {
                MouseMode mode = MouseMode::None;
                std::optional<OTIO_NS::RationalTime> shuttleStart;
            };
            MouseData mouse;
        };

        void Viewport::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<ftk::SysLogModel>& sysLogModel,
            const std::shared_ptr<IWidget>& parent)
        {
            tl::ui::Viewport::_init(context, parent);
            FTK_P();

            setClipChildren(true);

            p.viewportModel = viewportModel;
            p.timeUnitsModel = timeUnitsModel;

            p.fileNameLabel = ftk::Label::create(context);
            p.fileNameLabel->setFont(ftk::FontType::Mono);
            p.fileNameLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            p.cacheLabel = ftk::Label::create(context);
            p.cacheLabel->setFont(ftk::FontType::Mono);
            p.cacheLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            p.timeLabel = ftk::Label::create(context);
            p.timeLabel->setFont(ftk::FontType::Mono);
            p.timeLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            p.viewZoomLabel = ftk::Label::create(context);
            p.viewZoomLabel->setFont(ftk::FontType::Mono);
            p.viewZoomLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            p.colorPickerSwatch = ftk::ColorSwatch::create(context);
            p.colorPickerSwatch->setVAlign(ftk::VAlign::Center);
            p.colorPickerLabel = ftk::Label::create(context);
            p.colorPickerLabel->setFont(ftk::FontType::Mono);
            p.colorPickerLabel->setMarginRole(ftk::SizeRole::MarginSmall);
            auto colorPickerLayout = ftk::HorizontalLayout::create(context, p.hudLayout);
            colorPickerLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.colorPickerSwatch->setParent(colorPickerLayout);
            p.colorPickerLabel->setParent(colorPickerLayout);

            p.infoLabel = ftk::Label::create(context);
            p.infoLabel->setFont(ftk::FontType::Mono);
            p.infoLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            p.renderLabel = ftk::Label::create(context);
            p.renderLabel->setFont(ftk::FontType::Mono);
            p.renderLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            p.hudWidgets[models::HUDItem::FileName] = p.fileNameLabel;
            p.hudWidgets[models::HUDItem::Cache] = p.cacheLabel;
            p.hudWidgets[models::HUDItem::Time] = p.timeLabel;
            p.hudWidgets[models::HUDItem::ViewZoom] = p.viewZoomLabel;
            p.hudWidgets[models::HUDItem::ColorPicker] = colorPickerLayout;
            p.hudWidgets[models::HUDItem::Info] = p.infoLabel;
            p.hudWidgets[models::HUDItem::Render] = p.renderLabel;

            p.hudLayout = ftk::VerticalLayout::create(context, shared_from_this());
            p.hudLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            p.hudLayout->setSpacingRole(ftk::SizeRole::None);
            for (const auto i : models::getHUDPosEnums())
            {
                if (models::HUDPos::None == i)
                    continue;
                p.hudLayouts[i] = ftk::VerticalLayout::create(context);
                p.hudLayouts[i]->setMarginRole(ftk::SizeRole::MarginInside);
                p.hudLayouts[i]->setSpacingRole(ftk::SizeRole::None);
                p.hudLayouts[i]->setBackgroundRole(ftk::ColorRole::Overlay);
            }
            p.hudLayouts[models::HUDPos::TopLeft]->setVAlign(ftk::VAlign::Top);
            p.hudLayouts[models::HUDPos::TopRight]->setVAlign(ftk::VAlign::Top);
            p.hudLayouts[models::HUDPos::BottomLeft]->setVAlign(ftk::VAlign::Bottom);
            p.hudLayouts[models::HUDPos::BottomRight]->setVAlign(ftk::VAlign::Bottom);

            p.compareLabel = ftk::Label::create(context);
            p.compareLabel->setMarginRole(ftk::SizeRole::MarginSmall);
            p.compareLabel->setBackgroundRole(ftk::ColorRole::Overlay);
            p.compareLabel->setVisible(false);

            p.toastLabel = ftk::Label::create(context);
            p.toastLabel->setMarginRole(ftk::SizeRole::MarginSmall);
            p.toastLabel->setBackgroundRole(ftk::ColorRole::Overlay);
            p.toastLabel->setClipText(true);
            p.toastLabel->setVisible(false);

            auto topLayout = ftk::HorizontalLayout::create(context, p.hudLayout);
            topLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            topLayout->setVAlign(ftk::VAlign::Top);
            p.hudLayouts[models::HUDPos::TopLeft]->setParent(topLayout);
            auto spacer = ftk::Spacer::create(
                context, ftk::Orientation::Horizontal, topLayout);
            spacer->setStretch(ftk::Stretch::Expanding);
            p.hudLayouts[models::HUDPos::TopRight]->setParent(topLayout);

            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Vertical, p.hudLayout);
            spacer->setStretch(ftk::Stretch::Expanding);

            auto noBLayout = ftk::HorizontalLayout::create(context, p.hudLayout);
            noBLayout->setVAlign(ftk::VAlign::Center);
            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Horizontal, noBLayout);
            spacer->setStretch(ftk::Stretch::Expanding);
            p.compareLabel->setParent(noBLayout);
            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Horizontal, noBLayout);
            spacer->setStretch(ftk::Stretch::Expanding);
            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Vertical, p.hudLayout);
            spacer->setStretch(ftk::Stretch::Expanding);

            auto bottomLayout = ftk::VerticalLayout::create(context, p.hudLayout);
            bottomLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto toastLayout = ftk::HorizontalLayout::create(context, bottomLayout);
            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Horizontal, toastLayout);
            spacer->setStretch(ftk::Stretch::Expanding);
            p.toastLabel->setParent(toastLayout);
            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Horizontal, toastLayout);
            spacer->setStretch(ftk::Stretch::Expanding);
            auto bottomRowLayout = ftk::HorizontalLayout::create(context, bottomLayout);
            bottomRowLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            bottomRowLayout->setVAlign(ftk::VAlign::Bottom);
            p.hudLayouts[models::HUDPos::BottomLeft]->setParent(bottomRowLayout);
            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Horizontal, bottomRowLayout);
            spacer->setStretch(ftk::Stretch::Expanding);
            p.hudLayouts[models::HUDPos::BottomRight]->setParent(bottomRowLayout);

            p.toastTimer = ftk::Timer::create(context);

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

            p.aObserver = ftk::Observer<std::shared_ptr<models::FilesModelItem> >::create(
                filesModel->observeA(),
                [this](const std::shared_ptr<models::FilesModelItem>& value)
                {
                    _p->a = value;
                    _compareUpdate();
                });

            p.bObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                filesModel->observeB(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    _p->b = value;
                    _compareUpdate();
                });

            p.compareOptionsObserver = ftk::Observer<tl::CompareOptions>::create(
                filesModel->observeCompareOptions(),
                [this](const tl::CompareOptions& value)
                {
                    _p->compare = value.compare;
                    setCompareOptions(value);
                    _compareUpdate();
                });

            // The options as written, not the resolved ones: the per item
            // display options carry each file's resolved input, so a file
            // that resolves nothing falls back to what the user chose
            // rather than to whatever the active file resolved to.
            p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                colorModel->observeOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                   setOCIOOptions(value);
                });

            p.resolvedInputsObserver = ftk::Observer<std::vector<std::string> >::create(
                colorModel->observeResolvedInputs(),
                [this](const std::vector<std::string>& value)
                {
                    _p->ocioInputs = value;
                    _videoUpdate();
                });

            // Layers of a timeline resolve their own input color spaces --
            // each clip of an OTIO file is its own media -- but only when
            // the input is automatic; one the user chose applies to every
            // layer.
            
            setOCIOInputResolver(
                [colorModel](const std::string& path, const ftk::ImageTags& tags)
                {
                    return colorModel->getOCIOOptions().input.empty() ?
                        colorModel->resolveInput(path, tags) :
                        std::string();
                });

            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                colorModel->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                   setLUTOptions(value);
                });

            p.imageOptionsObserver = ftk::Observer<ftk::ImageOptions>::create(
                viewportModel->observeImageOptions(),
                [this](const ftk::ImageOptions& value)
                {
                    _p->imageOptions = value;
                    _videoUpdate();
                });

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                viewportModel->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    _p->displayOptions = value;
                    _videoUpdate();
                    // The heads up display says what is being rendered, which
                    // the aspect ratio changes. Without this it would only
                    // catch up when something else refreshes it -- which is
                    // every frame while playing, and nothing at all while
                    // stopped.
                    _hudUpdate();
                });

            p.bgOptionsObserver = ftk::Observer<tl::BackgroundOptions>::create(
                viewportModel->observeBackgroundOptions(),
                [this](const tl::BackgroundOptions& value)
                {
                    setBackgroundOptions(value);
                });

            p.fgOptionsObserver = ftk::Observer<tl::ForegroundOptions>::create(
                viewportModel->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
                {
                    setForegroundOptions(value);
                });

            p.colorBufferObserver = ftk::Observer<ftk::gl::TextureType>::create(
                viewportModel->observeColorBuffer(),
                [this](ftk::gl::TextureType value)
                {
                    setColorBuffer(value);
                    _hudUpdate();
                });

            p.viewZoomObserver = ftk::Observer<double>::create(
                observeZoom(),
                [this](double value)
                {
                    _p->viewZoom = value;
                    _hudUpdate();
                });

            p.messagesObserver = ftk::ListObserver<ftk::LogItem>::create(
                sysLogModel->observeMessages(),
                [this](const std::vector<ftk::LogItem>& value)
                {
                    FTK_P();
                    if (value.empty())
                    {
                        // Cleared, so there is nothing left to be showing.
                        p.toastLabel->setText(std::string());
                        _toastUpdate();
                        return;
                    }
                    // Errors only. The list carries warnings as well, and the
                    // status bar shows both; drawing over the image is for
                    // what stopped something from working, not for what is
                    // merely worth knowing. A warning arriving is also not a
                    // reason to cut short an error that is still up.
                    if (ftk::LogType::Error != value.back().type)
                    {
                        return;
                    }
                    // The message alone: it has just appeared, and the space
                    // over the image is better spent on what went wrong.
                    p.toastLabel->setText(
                        ftk::elide(
                            ftk::getLabel(value.back(), ftk::LogLabel::Message),
                            toastTextLength));
                    _toastUpdate();
                    if (!p.toastLabel->getText().empty())
                    {
                        p.toastTimer->start(
                            toastTimeout,
                            [this]
                            {
                                _p->toastLabel->setText(std::string());
                                _toastUpdate();
                            });
                    }
                });

            p.hudOptionsObserver = ftk::Observer<models::HUDOptions>::create(
                viewportModel->observeHUDOptions(),
                [this](const models::HUDOptions& value)
                {
                    _p->hudOptions = value;
                    _hudUpdate();
                    _hudLayout();
                });

            p.timeUnitsObserver = ftk::Observer<tl::TimeUnits>::create(
                timeUnitsModel->observeTimeUnits(),
                [this](tl::TimeUnits value)
                {
                    _hudUpdate();
                });

            p.mouseSettingsObserver = ftk::Observer<models::MouseSettings>::create(
                settingsModel->observeMouse(),
                [this](const models::MouseSettings& value)
                {
                    FTK_P();
                    auto i = value.bindings.find(models::MouseAction::PanView);
                    setPanBinding(
                        i != value.bindings.end() ? i->second.button : ftk::MouseButton::None,
                        i != value.bindings.end() ? i->second.modifier : ftk::KeyModifier::None);
                    i = value.bindings.find(models::MouseAction::CompareWipe);
                    setWipeBinding(
                        i != value.bindings.end() ? i->second.button : ftk::MouseButton::None,
                        i != value.bindings.end() ? i->second.modifier : ftk::KeyModifier::None);
                    i = value.bindings.find(models::MouseAction::Pick);
                    setPickBinding(
                        i != value.bindings.end() ? i->second.button : ftk::MouseButton::None,
                        i != value.bindings.end() ? i->second.modifier : ftk::KeyModifier::None);
                    i = value.bindings.find(models::MouseAction::FrameShuttle);
                    p.frameShuttleBinding = i != value.bindings.end() ? i->second : models::MouseActionBinding();
                    p.frameShuttleScale = value.frameShuttleScale;
                });

            p.pickObserver = ftk::Observer<std::optional<ftk::V2I> >::create(
                observePick(),
                [this](const std::optional<ftk::V2I>&)
                {
                    _hudUpdate();
                });

            p.colorSampleObserver = ftk::Observer<std::optional<ftk::Color4F> >::create(
                observeColorSample(),
                [this](const std::optional<ftk::Color4F>&)
                {
                    _hudUpdate();
                });
        }

        Viewport::Viewport() :
            _p(new Private)
        {}

        Viewport::~Viewport()
        {}

        std::shared_ptr<Viewport> Viewport::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<ftk::SysLogModel>& sysLogModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<Viewport>(new Viewport);
            out->_init(
                context,
                filesModel,
                colorModel,
                viewportModel,
                timeUnitsModel,
                settingsModel,
                sysLogModel,
                parent);
            return out;
        }

        void Viewport::setPlayer(const std::shared_ptr<tl::Player>& player)
        {
            tl::ui::Viewport::setPlayer(player);
            FTK_P();
            if (player)
            {
                p.path = player->getPath();

                // The information describes the media reference being read, so
                // it is refreshed when the key changes. The observer also
                // reports the current key, which covers the new player.
                p.mediaReferenceKeyObserver = ftk::Observer<std::string>::create(
                    player->observeMediaReferenceKey(),
                    [this, player](const std::string&)
                    {
                        _p->ioInfo = player->getIOInfo();
                        // Panning and zooming move an image around the view.
                        // Media with no video has none, so the view has
                        // nothing to move and the wheel would change a zoom
                        // that shows nothing.
                        setInputEnabled(!_p->ioInfo.video.empty());
                        _hudUpdate();
                    });

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
                        FTK_P();
                        p.videoFramesSize = value.size();
                        _compareUpdate();
                        p.missing = false;
                        p.heldFrom.reset();
                        // The first source, which is the one the time in the
                        // heads up display is about.
                        if (!value.empty())
                        {
                            for (const auto& layer : value.front().layers)
                            {
                                if (layer.missing)
                                {
                                    p.missing = true;
                                    if (layer.heldFrom.has_value())
                                    {
                                        p.heldFrom = layer.heldFrom;
                                    }
                                }
                            }
                        }
                        _videoUpdate();
                        _hudUpdate();
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
                p.ioInfo = tl::IOInfo();
                setInputEnabled(false);
                p.mediaReferenceKeyObserver.reset();
                p.currentTime.reset();
                p.currentTimeObserver.reset();
                p.videoObserver.reset();
                p.cacheInfo = tl::PlayerCacheInfo();
                p.cacheObserver.reset();
                _hudUpdate();
            }
        }

        void Viewport::setHUDActive(bool value)
        {
            FTK_P();
            if (value == p.hudActive)
                return;
            p.hudActive = value;
            _hudLayout();
        }

        void Viewport::setToastActive(bool value)
        {
            FTK_P();
            if (value == p.toastActive)
                return;
            p.toastActive = value;
            _toastUpdate();
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
                    // The mode is taken on the press whether or not there was
                    // a player to read a position from, so the two can
                    // disagree and the shuttle has nothing to move from.
                    if (p.mouse.shuttleStart.has_value())
                    {
                        const OTIO_NS::RationalTime offset = OTIO_NS::RationalTime(
                            (event.pos.x - _getMousePressPos().x) * .05F * p.frameShuttleScale,
                            p.mouse.shuttleStart->rate()).round();
                        const OTIO_NS::TimeRange& timeRange = player->getTimeRange();
                        OTIO_NS::RationalTime t = *p.mouse.shuttleStart + offset;
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
                }
                break;
            default: break;
            }
        }

        void Viewport::mousePressEvent(ftk::MouseClickEvent& event)
        {
            tl::ui::Viewport::mousePressEvent(event);
            FTK_P();
            // Shuttling is ours rather than the base class's, so it needs
            // this test of its own; without it it would go on working in a
            // view that takes no input.
            if (!isInputEnabled())
            {
                return;
            }
            if (p.frameShuttleBinding.button == event.button &&
                ftk::checkKeyModifier(p.frameShuttleBinding.modifier, event.modifiers))
            {
                // The base class only claims the buttons bound to its own
                // actions, so claim ours here; an unbound button is left
                // free to open a context menu.
                event.accept = true;
                takeKeyFocus();
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
            std::vector<ftk::ImageOptions> imageOptionsList;
            std::vector<tl::DisplayOptions> displayOptionsList;
            for (size_t i = 0; i < p.videoFramesSize; ++i)
            {
                imageOptionsList.push_back(p.imageOptions);
                displayOptionsList.push_back(p.displayOptions);
                // The input color space resolved for this item's file, so
                // a comparison of files in different color spaces shows
                // each of them correctly.
                displayOptionsList.back().ocioInput =
                    i < p.ocioInputs.size() ? p.ocioInputs[i] : std::string();
            }
            setImageOptions(imageOptionsList);
            setDisplayOptions(displayOptionsList);
        }

        void Viewport::_compareUpdate()
        {
            FTK_P();
            // Everything but A needs a B file that is not the A file. Neither
            // draws anything, and an empty picture is the one thing a black
            // frame, an unreadable file and a comparison out of sync all look
            // like as well.
            std::string s;
            if (p.compare != tl::Compare::None)
            {
                if (p.videoFramesSize < 2)
                {
                    s = "No B file selected";
                }
                else if (p.a && !p.b.empty())
                {
                    // Only when there is nothing else in B: a file compared
                    // with itself alongside others still has something to
                    // show.
                    const bool allA = std::all_of(
                        p.b.begin(),
                        p.b.end(),
                        [this](const std::shared_ptr<models::FilesModelItem>& i)
                        {
                            return i == _p->a;
                        });
                    if (allA)
                    {
                        s = "A and B are the same file";
                    }
                }
            }
            p.compareLabel->setText(s);
            p.compareLabel->setVisible(!s.empty());
            ftk::setScreenshotTag(p.compareLabel, !s.empty() ? "View.Compare" : "");
        }

        void Viewport::_hudUpdate()
        {
            FTK_P();

            std::string s = p.path.getFileName();
            p.fileNameLabel->setText(!s.empty() ? s : "(No file)");
            ftk::setScreenshotTag(p.fileNameLabel, "View.HUD.FileName");

            std::vector<std::string> info;
            if (!p.ioInfo.video.empty())
            {
                info.push_back(std::string(ftk::Format("V: {0}").
                    arg(ftk::getLabel(p.ioInfo.video[0]))));
            }
            if (p.ioInfo.audio.isValid())
            {
                info.push_back(std::string(ftk::Format("A: {0}").
                    arg(tl::getLabel(p.ioInfo.audio, true))));
            }
            p.infoLabel->setText(ftk::join(info, ", "));
            p.infoLabel->setVisible(!info.empty());
            ftk::setScreenshotTag(p.infoLabel, "View.HUD.Info");

            // What is actually rendered, which the pixel aspect ratio and the
            // aspect ratio override can both move away from the media size
            // reported above. The effective pixel aspect ratio is taken back
            // out of the render size rather than read from either source, so
            // it holds whether it came from the media or from an override.
            s = std::string();
            if (!p.ioInfo.video.empty())
            {
                const ftk::ImageInfo& videoInfo = p.ioInfo.video[0];
                const ftk::Size2I renderSize = tl::getRenderSize(
                    videoInfo,
                    p.displayOptions.aspectRatio);
                if (renderSize.isValid() && videoInfo.size.w > 0)
                {
                    const float pixelAspectRatio =
                        renderSize.w / static_cast<float>(videoInfo.size.w);
                    s = ftk::Format("Render: {0}x{1}:{2}").
                        arg(renderSize.w).
                        arg(renderSize.h).
                        arg(ftk::aspectRatio(renderSize), 2);
                    // Square pixels are the common case and add nothing.
                    if (std::fabs(pixelAspectRatio - 1.F) > 0.001F)
                    {
                        s += ftk::Format(", PAR: {0}").
                            arg(pixelAspectRatio, 2);
                    }
                }
            }
            p.renderLabel->setText(s);
            p.renderLabel->setVisible(!s.empty());
            ftk::setScreenshotTag(p.renderLabel, "View.HUD.Render");

            s = std::string();
            if (auto timeUnitsModel = p.timeUnitsModel.lock())
            {
                s = timeUnitsModel->getLabel(p.currentTime);
            }
            std::string missing;
            if (p.missing)
            {
                // Said rather than only drawn, so which frame is standing in
                // is known and not merely that one is.
                missing = p.heldFrom.has_value() ?
                    ftk::Format(", held from {0}").
                        arg(p.heldFrom.value()).str() :
                    ", missing";
            }
            // Frames per second and frames dropped are about video, and a
            // file without any has neither -- reporting none and an
            // ever-growing count of what was never going to arrive reads as
            // a fault rather than as an absence.
            std::string timeText = ftk::Format("Time: {0}{1}").
                arg(s).
                arg(missing);
            if (!p.ioInfo.video.empty())
            {
                timeText = ftk::Format("Time: {0}, {1} FPS, {2} dropped{3}").
                    arg(s).
                    arg(p.fps, 2, 6).
                    arg(static_cast<int>(p.droppedFrames), 3).
                    arg(missing);
            }
            p.timeLabel->setText(timeText);
            ftk::setScreenshotTag(p.timeLabel, "View.HUD.Time");

            p.viewZoomLabel->setText(ftk::Format("Zoom: {0}").
                arg(p.viewZoom, 2, 6));
            ftk::setScreenshotTag(p.viewZoomLabel, "View.HUD.ViewZoom");

            const auto& colorSample = observeColorSample()->get();
            const auto& pick = observePick()->get();
            p.colorPickerSwatch->setColor(
                colorSample.has_value() ? colorSample.value() : ftk::Color4F());
            // The HUD sits under the pointer, so a line that changes width
            // as values come and go moves exactly where the eye is. Both
            // forms are built from the same layout and field widths, so the
            // line keeps its size whether or not there is a sample -- which
            // is what happens every time the pointer leaves the image. The
            // widths hold a sign and two digits, so the ordinary zero to one
            // range and a little either side of it do not move the line
            // either; beyond that the field grows, as it did before.
            const int colorWidth = 5;
            const int pickWidth = 4;
            const std::string colorPickerFormat =
                "Color: {0} {1} {2} {3}, Pixel: {4}, {5}";
            std::string colorPickerText =
                ftk::Format(colorPickerFormat).
                arg("-", colorWidth).
                arg("-", colorWidth).
                arg("-", colorWidth).
                arg("-", colorWidth).
                arg("-", pickWidth).
                arg("-", pickWidth);
            if (colorSample.has_value() && pick.has_value())
            {
                colorPickerText =
                    ftk::Format(colorPickerFormat).
                    arg(colorSample.value().r, 2, colorWidth).
                    arg(colorSample.value().g, 2, colorWidth).
                    arg(colorSample.value().b, 2, colorWidth).
                    arg(colorSample.value().a, 2, colorWidth).
                    arg(pick.value().x, pickWidth).
                    arg(pick.value().y, pickWidth);
            }
            p.colorPickerLabel->setText(colorPickerText);
            ftk::setScreenshotTag(p.colorPickerLabel, "View.HUD.ColorPicker");
            ftk::setScreenshotTag(p.colorPickerSwatch, "View.HUD.ColorPickerSwatch");

            std::vector<std::string> cache;
            if (!p.ioInfo.video.empty())
            {
                cache.push_back(std::string(ftk::Format("{0}% V").
                    arg(static_cast<int>(p.cacheInfo.videoPercentage), 3)));
            }
            if (p.ioInfo.audio.isValid())
            {
                cache.push_back(std::string(ftk::Format("{0}% A").
                    arg(static_cast<int>(p.cacheInfo.audioPercentage), 3)));
            }
            s = !cache.empty() ?
                std::string(ftk::Format("Cache: {0}").arg(ftk::join(cache, ", "))) :
                std::string();
            p.cacheLabel->setText(s);
            p.cacheLabel->setVisible(!s.empty());
            ftk::setScreenshotTag(p.cacheLabel, "View.HUD.Cache");
        }

        void Viewport::_toastUpdate()
        {
            FTK_P();
            const bool visible =
                p.toastActive && !p.toastLabel->getText().empty();
            p.toastLabel->setVisible(visible);
            // Tagged only while it is up, the same as the compare label, so
            // that a capture says whether anything was drawn over the image
            // rather than only what it would have said.
            ftk::setScreenshotTag(p.toastLabel, visible ? "View.Toast" : "");
        }

        void Viewport::_hudLayout()
        {
            FTK_P();
            auto viewportModel = p.viewportModel.lock();
            const auto options = viewportModel->getHUDOptions();
            for (const auto& i : options.items)
            {
                p.hudWidgets[i.first]->setParent(i.second != models::HUDPos::None ?
                    p.hudLayouts[i.second] :
                    nullptr);
            }
            for (const auto& i : p.hudLayouts)
            {
                i.second->setVisible(
                    p.hudActive &&
                    options.enabled &&
                    i.second->getChildren().size() > 0);
            }
        }
    }
}
