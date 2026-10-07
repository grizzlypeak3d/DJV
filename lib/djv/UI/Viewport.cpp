// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/Viewport.h>

#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/Stroke.h>
#include <djv/UI/StrokeDraw.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/Timeline/Util.h>

#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/IWindow.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/SysLogModel.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Spacer.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>
#include <ftk/Core/Timer.h>

#include <algorithm>

#include <chrono>
#include <cmath>
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
            const std::chrono::seconds hintTimeout(3);
        }

        struct Viewport::Private
        {
            std::weak_ptr<models::ViewportModel> viewportModel;
            std::weak_ptr<models::TimeUnitsModel> timeUnitsModel;
            models::HUDOptions hudOptions;
            ftk::Path path;
            tl::IOInfo ioInfo;
            //! Which layer of the file is being shown. The HUD reports that
            //! layer, not the first one: a file can hold layers of different
            //! types.
            int videoLayer = 0;
            std::optional<OTIO_NS::RationalTime> currentTime;
            double fps = 0.0;
            size_t droppedFrames = 0;
            size_t videoFramesSize = 0;
            std::vector<std::string> ocioInputs;
            // Whether the picture stands in for a frame the media does not
            // have, and the frame it repeats when there is one.
            bool missing = false;
            std::optional<int64_t> heldFrom;
            // Kept for the annotation hit test and overlay, which need the
            // image sizes to map screen to image pixels.
            std::vector<tl::VideoFrame> videoFrames;
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
            tl::CompareTime compareTime = tl::CompareTime::Relative;
            std::shared_ptr<models::FilesModelItem> a;
            std::vector<std::shared_ptr<models::FilesModelItem> > b;
            std::shared_ptr<ftk::Label> compareLabel;

            bool toastActive = false;
            bool toastHint = false;
            bool hudActive = true;
            std::shared_ptr<ftk::Label> toastLabel;
            std::shared_ptr<ftk::Timer> toastTimer;
            std::shared_ptr<ftk::VerticalLayout> hudLayout;
            std::map<models::HUDPos, std::shared_ptr<ftk::VerticalLayout> > hudLayouts;

            std::shared_ptr<ftk::Observer<OTIO_NS::RationalTime> > currentTimeObserver;
            std::shared_ptr<ftk::Observer<std::string> > mediaReferenceKeyObserver;
            std::shared_ptr<ftk::Observer<int> > videoLayerObserver;
            std::shared_ptr<ftk::ListObserver<tl::VideoFrame> > videoObserver;
            std::shared_ptr<ftk::Observer<tl::PlayerCacheInfo> > cacheObserver;
            std::shared_ptr<ftk::Observer<double> > fpsObserver;
            std::shared_ptr<ftk::Observer<size_t> > droppedFramesObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<models::FilesModelItem> > > aObserver;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > bObserver;
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::CompareTime> > compareTimeObserver;
            std::shared_ptr<ftk::Observer<bool> > drawEnabledObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<std::vector<std::string> > > resolvedInputsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageOptions> > imageOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::BackgroundOptions> > bgOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > fgOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageType> > colorBufferObserver;
            std::shared_ptr<ftk::Observer<double> > viewZoomObserver;
            std::shared_ptr<ftk::ListObserver<ftk::LogItem> > messagesObserver;
            std::shared_ptr<ftk::Observer<models::HUDOptions> > hudOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::TimeUnits> > timeUnitsObserver;
            std::shared_ptr<ftk::Observer<models::MouseSettings> > mouseSettingsObserver;
            std::shared_ptr<ftk::Observer<std::optional<ftk::V2I> > > pickObserver;
            std::shared_ptr<ftk::Observer<std::optional<ftk::Color4F> > > colorSampleObserver;
            std::shared_ptr<ftk::ListObserver<models::ReviewAnnotation> > annotationsObserver;

            enum class MouseMode
            {
                None,
                Shuttle,
                Draw,
                Erase,
                Select
            };
            struct MouseData
            {
                MouseMode mode = MouseMode::None;
                std::optional<OTIO_NS::RationalTime> shuttleStart;
                tl::Playback shuttlePlayback = tl::Playback::Stop;
            };
            MouseData mouse;

            //! The stroke being drawn, in the image pixels of its source,
            //! and the frame it started on: the frame can change under a
            //! text being typed, and the text belongs where it was begun.
            models::ReviewStroke stroke;
            int strokeSource = -1;
            std::optional<OTIO_NS::RationalTime> strokeTime;
            //! Whether the stroke is text being typed, and whether that text
            //! is the selected stroke written again rather than a new one.
            bool textEditing = false;
            bool textEditingSelection = false;

            //! The selected stroke: the source and frame it is on, and its
            //! index among that frame's strokes. While it is moved, a copy
            //! with the offset applied is drawn in its place.
            struct Selection
            {
                int source = -1;
                std::string sourceId;
                OTIO_NS::RationalTime time;
                size_t index = 0;
            };
            std::optional<Selection> selection;
            std::optional<models::ReviewStroke> moving;
            ftk::V2F movePressPos;
            //! The handle being dragged, one of the two points of a shape,
            //! or none while the whole stroke moves.
            int handle = -1;
            //! When the selection was last pressed, for the double click
            //! that opens text.
            std::chrono::steady_clock::time_point selectPressTime;
            std::shared_ptr<ftk::Observer<models::DrawTool> > toolObserver;
            std::shared_ptr<ftk::Observer<ftk::Color4F> > drawColorObserver;
            std::shared_ptr<ftk::Observer<float> > drawSizeObserver;
            std::shared_ptr<ftk::Observer<float> > drawTextSizeObserver;

            std::shared_ptr<models::FilesModel> filesModel;
            std::shared_ptr<models::AnnotationsModel> annotationsModel;
            std::shared_ptr<models::DrawModel> drawModel;
        };

        void Viewport::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<models::AnnotationsModel>& annotationsModel,
            const std::shared_ptr<models::DrawModel>& drawModel,
            const std::shared_ptr<ftk::SysLogModel>& sysLogModel,
            const std::shared_ptr<IWidget>& parent)
        {
            tl::ui::Viewport::_init(context, parent);
            FTK_P();

            setClipChildren(true);

            p.filesModel = filesModel;
            p.annotationsModel = annotationsModel;
            p.drawModel = drawModel;
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

            p.drawEnabledObserver = ftk::Observer<bool>::create(
                drawModel->observeEnabled(),
                [this](bool value)
                {
                    // Turning drawing off keeps what was typed so far, and
                    // lets the selection go.
                    if (!value && _p->textEditing)
                    {
                        _textEnd(true);
                    }
                    if (!value)
                    {
                        _selectionClear();
                    }
                    _cursorUpdate();
                });

            p.toolObserver = ftk::Observer<models::DrawTool>::create(
                drawModel->observeTool(),
                [this](models::DrawTool value)
                {
                    if (value != models::DrawTool::Select)
                    {
                        _selectionClear();
                    }
                    _cursorUpdate();
                });

            // The color and the sizes are for the next stroke, and for the
            // selected one while there is one.
            p.drawColorObserver = ftk::Observer<ftk::Color4F>::create(
                drawModel->observeColor(),
                [this](const ftk::Color4F& value)
                {
                    _selectionUpdate([&value](models::ReviewStroke& s) { s.color = value; });
                });
            p.drawSizeObserver = ftk::Observer<float>::create(
                drawModel->observeSize(),
                [this](float value)
                {
                    _selectionUpdate([value](models::ReviewStroke& s) { s.width = value; });
                });
            p.drawTextSizeObserver = ftk::Observer<float>::create(
                drawModel->observeTextSize(),
                [this](float value)
                {
                    _selectionUpdate(
                        [value](models::ReviewStroke& s)
                        {
                            if (models::ReviewStrokeKind::Text == s.kind)
                            {
                                s.textSize = value;
                            }
                        });
                });

            p.compareTimeObserver = ftk::Observer<tl::CompareTime>::create(
                filesModel->observeCompareTime(),
                [this](tl::CompareTime value)
                {
                    _p->compareTime = value;
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

            p.colorBufferObserver = ftk::Observer<ftk::ImageType>::create(
                viewportModel->observeColorBuffer(),
                [this](ftk::ImageType value)
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
                    // over the image is better spent on what went wrong. It
                    // takes the place of a hint, and is shown or not the way
                    // errors are.
                    p.toastHint = false;
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

            // The overlay is drawn straight from the model, so any change to it
            // has to ask for a redraw. Drawing does that from the mouse events,
            // but undo, redo and "Clear Drawing" only touch the model: without
            // this the frame keeps showing stale strokes until some unrelated
            // event happens to repaint the window.
            p.annotationsObserver = ftk::ListObserver<models::ReviewAnnotation>::create(
                annotationsModel->observeAnnotations(),
                [this](const std::vector<models::ReviewAnnotation>&)
                {
                    FTK_P();
                    // The selection names a stroke by its place; an undo
                    // or an erase can take that place away.
                    if (p.selection.has_value() &&
                        p.selection->index >= p.annotationsModel->getStrokes(
                            p.selection->sourceId, p.selection->time).size())
                    {
                        _selectionClear();
                    }
                    setDrawUpdate();
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
                    // Here with the other mouse settings rather than set by the
                    // main window, which left a secondary window's viewport
                    // zooming at the default scale.
                    setMouseWheelScale(value.wheelScale);
                    auto j = value.wheelBindings.find(models::WheelAction::Zoom);
                    setWheelZoomBinding(j != value.wheelBindings.end() ?
                        j->second :
                        ftk::KeyModifier::None);
                    j = value.wheelBindings.find(models::WheelAction::Scrub);
                    setWheelScrubBinding(j != value.wheelBindings.end() ?
                        j->second :
                        ftk::KeyModifier::Control);
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
            const std::shared_ptr<models::AnnotationsModel>& annotationsModel,
            const std::shared_ptr<models::DrawModel>& drawModel,
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
                annotationsModel,
                drawModel,
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

                p.videoLayerObserver = ftk::Observer<int>::create(
                    player->observeVideoLayer(),
                    [this](int value)
                    {
                        _p->videoLayer = value;
                        _hudUpdate();
                    });

                p.currentTimeObserver = ftk::Observer<OTIO_NS::RationalTime>::create(
                    player->observeCurrentTime(),
                    [this](const OTIO_NS::RationalTime& value)
                    {
                        _p->currentTime = value;
                        _hudUpdate();
                        // Annotations are per frame, so the overlay changes,
                        // and the selection was on the frame before.
                        _selectionClear();
                        setDrawUpdate();
                    });

                p.videoObserver = ftk::ListObserver<tl::VideoFrame>::create(
                    player->observeCurrentVideo(),
                    [this](const std::vector<tl::VideoFrame>& value)
                    {
                        FTK_P();
                        p.videoFramesSize = value.size();
                        p.videoFrames = value;
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
                p.videoFrames.clear();
                p.currentTimeObserver.reset();
                p.videoObserver.reset();
                p.cacheInfo = tl::PlayerCacheInfo();
                p.cacheObserver.reset();
                _hudUpdate();
            }
            // A file arriving or going changes which items there are.
            _hudLayout();
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

        void Viewport::showHint(const std::string& value)
        {
            FTK_P();
            p.toastHint = true;
            p.toastLabel->setText(value);
            _toastUpdate();
            p.toastTimer->start(
                hintTimeout,
                [this]
                {
                    _p->toastHint = false;
                    _p->toastLabel->setText(std::string());
                    _toastUpdate();
                });
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
            case Private::MouseMode::Draw:
                _drawContinue(event.pos - getGeometry().min, event.modifiers);
                break;
            case Private::MouseMode::Erase:
                _erase(event.pos - getGeometry().min);
                break;
            case Private::MouseMode::Select:
                _selectMove(event.pos - getGeometry().min, event.modifiers);
                break;
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

            // Drawing owns the plain left button while it is enabled, which is
            // why it is an explicit mode: it displaces the frame shuttle.
            if (p.drawModel->isEnabled() &&
                ftk::MouseButton::Left == event.button &&
                0 == event.modifiers)
            {
                // Claim the button, as the shuttle does below. Without this
                // the release never reaches us: the stroke is never
                // committed, keeps growing on every move, and the undo that
                // follows has to copy all of it.
                event.accept = true;
                takeKeyFocus();
                const ftk::Box2I& g = getGeometry();
                const ftk::V2I pos = event.pos - g.min;
                // A click elsewhere finishes the text being typed.
                if (p.textEditing)
                {
                    _textEnd(true);
                }
                switch (p.drawModel->getTool())
                {
                case models::DrawTool::Eraser:
                    p.mouse.mode = Private::MouseMode::Erase;
                    _erase(pos);
                    break;
                case models::DrawTool::Text:
                    _textBegin(pos);
                    break;
                case models::DrawTool::Select:
                    p.mouse.mode = Private::MouseMode::Select;
                    _selectPress(pos);
                    break;
                default:
                    p.mouse.mode = Private::MouseMode::Draw;
                    _drawBegin(pos);
                    break;
                }
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
                    p.mouse.shuttlePlayback = player->getPlayback();
                    player->stop();
                    p.mouse.shuttleStart = player->getCurrentTime();
                }
            }
        }

        void Viewport::mouseReleaseEvent(ftk::MouseClickEvent& event)
        {
            tl::ui::Viewport::mouseReleaseEvent(event);
            FTK_P();
            if (Private::MouseMode::Draw == p.mouse.mode)
            {
                // Commit the stroke as one undoable step, unless the press
                // was cancelled: the first finger of a pinch is not a
                // stroke.
                if (event.cancel)
                {
                    p.stroke = models::ReviewStroke();
                    p.strokeSource = -1;
                    setDrawUpdate();
                }
                else
                {
                    _drawEnd();
                }
            }
            else if (Private::MouseMode::Select == p.mouse.mode)
            {
                _selectRelease(event.cancel);
            }
            else if (Private::MouseMode::Shuttle == p.mouse.mode && event.cancel)
            {
                // Back to the frame, and playing if it was: the first
                // finger of a gesture was not shuttling.
                if (auto player = getPlayer())
                {
                    if (p.mouse.shuttleStart.has_value())
                    {
                        player->seek(p.mouse.shuttleStart.value());
                    }
                    player->setPlayback(p.mouse.shuttlePlayback);
                }
            }
            p.mouse = Private::MouseData();
        }

        void Viewport::mouseEnterEvent(ftk::MouseEnterEvent& event)
        {
            tl::ui::Viewport::mouseEnterEvent(event);
            _cursorUpdate();
        }

        void Viewport::mouseLeaveEvent()
        {
            tl::ui::Viewport::mouseLeaveEvent();
            _cursorUpdate();
        }

        void Viewport::_cursorUpdate()
        {
            FTK_P();
            if (auto window = getWindow())
            {
                window->setCursor(
                    _isMouseInside() &&
                    p.drawModel->isEnabled() &&
                    p.drawModel->getTool() != models::DrawTool::Select ?
                    ftk::CursorShape::Crosshair :
                    ftk::CursorShape::Arrow);
            }
        }

        std::vector<ftk::Box2I> Viewport::_sourceBoxes() const
        {
            FTK_P();
            // Mirror what tl::ui::Viewport uses when it draws, so the hit test
            // and the overlay land exactly where the image does.
            return tl::getBoxes(
                getCompareOptions(),
                p.displayOptions.aspectRatio,
                p.videoFrames);
        }

        bool Viewport::_sourceShown(int index) const
        {
            FTK_P();
            // The comparisons that put the files over each other give them all
            // the same box, so drawings of B landed on A's picture, and a new
            // stroke could only ever reach A, the first box hit. Only A is
            // drawn on there; B's drawings return side by side or on their own.
            switch (p.compare)
            {
            case tl::Compare::Wipe:
            case tl::Compare::Overlay:
            case tl::Compare::Difference:
            case tl::Compare::Butterfly:
                return 0 == index;
            default: break;
            }
            return tl::isShown(p.compare, static_cast<size_t>(index));
        }

        std::optional<ftk::V2F> Viewport::_imagePosClamped(int index, const ftk::V2I& widgetPos) const
        {
            FTK_P();
            std::optional<ftk::V2F> out;
            const double zoom = getZoom();
            const auto boxes = _sourceBoxes();
            if (zoom <= 0.0 ||
                index < 0 ||
                index >= static_cast<int>(boxes.size()) ||
                index >= static_cast<int>(p.videoFrames.size()))
            {
                return out;
            }
            const auto& video = p.videoFrames[index];
            if (video.layers.empty() || !video.layers[0].image)
            {
                return out;
            }
            const ftk::Size2I imageSize = video.layers[0].image->getSize();
            const ftk::Box2I& box = boxes[index];
            if (imageSize.w <= 0 || imageSize.h <= 0 || box.w() <= 0 || box.h() <= 0)
            {
                return out;
            }
            // The same mapping as the hit test, without the test.
            const ftk::V2I& viewPos = getViewPos();
            const ftk::V2F render(
                static_cast<float>((widgetPos.x - viewPos.x) / zoom),
                static_cast<float>((widgetPos.y - viewPos.y) / zoom));
            ftk::V2F pos(
                (render.x - box.min.x) * imageSize.w / static_cast<float>(box.w()),
                (render.y - box.min.y) * imageSize.h / static_cast<float>(box.h()));
            if (p.displayOptions.mirror.x)
            {
                pos.x = imageSize.w - pos.x;
            }
            if (p.displayOptions.mirror.y)
            {
                pos.y = imageSize.h - pos.y;
            }
            pos.x = std::max(0.F, std::min(static_cast<float>(imageSize.w), pos.x));
            pos.y = std::max(0.F, std::min(static_cast<float>(imageSize.h), pos.y));
            out = pos;
            return out;
        }

        Viewport::SourceHit Viewport::_hitTest(const ftk::V2I& widgetPos) const
        {
            FTK_P();
            SourceHit out;
            // Widget -> render space: the inverse of translate(viewPos) * scale.
            const double zoom = getZoom();
            if (zoom <= 0.0)
            {
                return out;
            }
            const ftk::V2I& viewPos = getViewPos();
            const ftk::V2F render(
                static_cast<float>((widgetPos.x - viewPos.x) / zoom),
                static_cast<float>((widgetPos.y - viewPos.y) / zoom));

            const auto boxes = _sourceBoxes();
            for (size_t i = 0; i < boxes.size() && i < p.videoFrames.size(); ++i)
            {
                if (!_sourceShown(static_cast<int>(i)))
                {
                    continue;
                }
                const ftk::Box2I& box = boxes[i];
                if (render.x < box.min.x || render.x > box.max.x ||
                    render.y < box.min.y || render.y > box.max.y ||
                    box.w() <= 0 || box.h() <= 0)
                {
                    continue;
                }
                const auto& video = p.videoFrames[i];
                if (video.layers.empty() || !video.layers[0].image)
                {
                    continue;
                }
                const ftk::Size2I imageSize = video.layers[0].image->getSize();
                if (imageSize.w <= 0 || imageSize.h <= 0)
                {
                    continue;
                }
                // Render space -> the source's own pixels.
                out.index = static_cast<int>(i);
                out.pos = ftk::V2F(
                    (render.x - box.min.x) * imageSize.w / static_cast<float>(box.w()),
                    (render.y - box.min.y) * imageSize.h / static_cast<float>(box.h()));
                // The mirror flips the image inside its box, so the same
                // flip takes the hit back to the image's own pixels. Strokes
                // are stored unmirrored, and follow the image when the
                // mirror changes.
                if (p.displayOptions.mirror.x)
                {
                    out.pos.x = imageSize.w - out.pos.x;
                }
                if (p.displayOptions.mirror.y)
                {
                    out.pos.y = imageSize.h - out.pos.y;
                }
                out.scale = box.w() / static_cast<float>(imageSize.w);
                break;
            }
            return out;
        }

        ftk::V2F Viewport::_imageToWidget(int index, const ftk::V2F& imagePos) const
        {
            FTK_P();
            const auto boxes = _sourceBoxes();
            if (index < 0 ||
                index >= static_cast<int>(boxes.size()) ||
                index >= static_cast<int>(p.videoFrames.size()))
            {
                return ftk::V2F();
            }
            const ftk::Box2I& box = boxes[index];
            const auto& video = p.videoFrames[index];
            if (video.layers.empty() || !video.layers[0].image)
            {
                return ftk::V2F();
            }
            const ftk::Size2I imageSize = video.layers[0].image->getSize();
            if (imageSize.w <= 0 || imageSize.h <= 0)
            {
                return ftk::V2F();
            }
            // The inverse of the flip in _hitTest: stored image pixels back
            // to where the mirror shows them.
            ftk::V2F pos = imagePos;
            if (p.displayOptions.mirror.x)
            {
                pos.x = imageSize.w - pos.x;
            }
            if (p.displayOptions.mirror.y)
            {
                pos.y = imageSize.h - pos.y;
            }
            const float renderX = box.min.x + pos.x * box.w() / static_cast<float>(imageSize.w);
            const float renderY = box.min.y + pos.y * box.h() / static_cast<float>(imageSize.h);
            const double zoom = getZoom();
            const ftk::V2I& viewPos = getViewPos();
            return ftk::V2F(
                static_cast<float>(viewPos.x + renderX * zoom),
                static_cast<float>(viewPos.y + renderY * zoom));
        }

        void Viewport::keyPressEvent(ftk::KeyEvent& event)
        {
            FTK_P();
            if (p.textEditing)
            {
                // Every key is the text's while it is being typed, so that
                // a letter bound to a shortcut goes into the text and not to
                // the application.
                event.accept = true;
                switch (event.key)
                {
                case ftk::Key::Return:
                    _textEnd(true);
                    break;
                case ftk::Key::Escape:
                    _textEnd(false);
                    break;
                case ftk::Key::Backspace:
                    if (!p.stroke.text.empty())
                    {
                        // The last character, however many bytes it is.
                        size_t i = p.stroke.text.size() - 1;
                        while (i > 0 && (static_cast<uint8_t>(p.stroke.text[i]) & 0xC0) == 0x80)
                        {
                            --i;
                        }
                        p.stroke.text.erase(i);
                        setDrawUpdate();
                    }
                    break;
                default: break;
                }
                return;
            }
            if (p.selection.has_value())
            {
                switch (event.key)
                {
                case ftk::Key::Delete:
                case ftk::Key::Backspace:
                    event.accept = true;
                    p.annotationsModel->removeStroke(
                        p.selection->sourceId,
                        p.selection->time,
                        p.selection->index);
                    _selectionClear();
                    return;
                case ftk::Key::Return:
                    event.accept = true;
                    _selectionEditText();
                    return;
                case ftk::Key::Escape:
                    event.accept = true;
                    _selectionClear();
                    return;
                default: break;
                }
            }
            tl::ui::Viewport::keyPressEvent(event);
        }

        void Viewport::keyFocusEvent(bool value)
        {
            tl::ui::Viewport::keyFocusEvent(value);
            // The keyboard went elsewhere: what was typed is kept.
            if (!value && _p->textEditing)
            {
                _textEnd(true);
            }
        }

        void Viewport::textEvent(ftk::TextEvent& event)
        {
            FTK_P();
            if (p.textEditing)
            {
                event.accept = true;
                p.stroke.text += event.text;
                setDrawUpdate();
                return;
            }
            tl::ui::Viewport::textEvent(event);
        }

        namespace
        {
            //! Whether a stroke is a shape between two points, which the
            //! handles edit.
            bool isTwoPointShape(models::ReviewStrokeKind kind)
            {
                return
                    models::ReviewStrokeKind::Line == kind ||
                    models::ReviewStrokeKind::Arrow == kind ||
                    models::ReviewStrokeKind::Rectangle == kind ||
                    models::ReviewStrokeKind::Ellipse == kind;
            }

            //! One point of a shape against its other point: with Shift,
            //! square or round, or at a multiple of forty-five degrees.
            ftk::V2F constrainShape(
                models::ReviewStrokeKind kind,
                const ftk::V2F& anchor,
                const ftk::V2F& pos,
                int modifiers)
            {
                ftk::V2F out = pos;
                if (!(modifiers & static_cast<int>(ftk::KeyModifier::Shift)))
                {
                    return out;
                }
                const float dx = pos.x - anchor.x;
                const float dy = pos.y - anchor.y;
                switch (kind)
                {
                case models::ReviewStrokeKind::Rectangle:
                case models::ReviewStrokeKind::Ellipse:
                {
                    const float d = std::max(std::abs(dx), std::abs(dy));
                    out.x = anchor.x + (dx < 0.F ? -d : d);
                    out.y = anchor.y + (dy < 0.F ? -d : d);
                    break;
                }
                default:
                {
                    const float length = std::sqrt(dx * dx + dy * dy);
                    if (length > 0.F)
                    {
                        const float step = 3.14159265F / 4.F;
                        const float angle = std::round(std::atan2(dy, dx) / step) * step;
                        out.x = anchor.x + std::cos(angle) * length;
                        out.y = anchor.y + std::sin(angle) * length;
                    }
                    break;
                }
                }
                return out;
            }

            //! The size of a handle on the screen, and how near a press has
            //! to be to take it.
            const float handleSize = 8.F;
        }

        void Viewport::_drawBegin(const ftk::V2I& widgetPos)
        {
            FTK_P();
            const SourceHit hit = _hitTest(widgetPos);
            if (hit.index < 0)
            {
                return;
            }
            {
                const auto& drawModel = p.drawModel;
                p.strokeSource = hit.index;
                p.strokeTime = p.currentTime;
                p.stroke = models::ReviewStroke();
                p.stroke.kind = models::getStrokeKind(drawModel->getTool());
                p.stroke.color = drawModel->getColor();
                p.stroke.width = drawModel->getSize();
                p.stroke.points.push_back(hit.pos);
                setDrawUpdate();
            }
        }

        void Viewport::_drawContinue(const ftk::V2I& widgetPos, int modifiers)
        {
            FTK_P();
            if (p.strokeSource < 0)
            {
                return;
            }
            if (p.stroke.kind != models::ReviewStrokeKind::Freehand)
            {
                // A shape is its first point and wherever the mouse is now,
                // held inside the picture. With Shift it is held square, or
                // round, or to a line at a multiple of forty-five degrees.
                const auto clamped = _imagePosClamped(p.strokeSource, widgetPos);
                if (!clamped.has_value())
                {
                    return;
                }
                p.stroke.points.resize(2);
                p.stroke.points[1] = constrainShape(p.stroke.kind, p.stroke.points.front(), *clamped, modifiers);
                setDrawUpdate();
                return;
            }
            const SourceHit hit = _hitTest(widgetPos);
            // Keep the stroke on the source it started on, so dragging across a
            // comparison boundary does not tear it in two.
            if (hit.index != p.strokeSource)
            {
                return;
            }
            if (!p.stroke.points.empty())
            {
                const ftk::V2F& last = p.stroke.points.back();
                const float dx = hit.pos.x - last.x;
                const float dy = hit.pos.y - last.y;
                // Drop points the mouse barely moved, to keep strokes compact.
                if (dx * dx + dy * dy < .25F)
                {
                    return;
                }
            }
            p.stroke.points.push_back(hit.pos);
            setDrawUpdate();
        }

        void Viewport::_drawEnd()
        {
            FTK_P();
            // A shape wants its second point: a click with no drag is nothing.
            // Ink and text want one.
            const bool shape =
                p.stroke.kind != models::ReviewStrokeKind::Freehand &&
                p.stroke.kind != models::ReviewStrokeKind::Text;
            if (p.strokeSource >= 0 &&
                p.strokeTime.has_value() &&
                (shape ? p.stroke.points.size() > 1 : !p.stroke.points.empty()))
            {
                const auto& active = p.filesModel->getActive();
                if (p.strokeSource < static_cast<int>(active.size()))
                {
                    p.annotationsModel->addStroke(
                        active[p.strokeSource]->id,
                        *p.strokeTime,
                        p.stroke);
                }
            }
            p.stroke = models::ReviewStroke();
            p.strokeSource = -1;
            p.strokeTime.reset();
            setDrawUpdate();
        }

        void Viewport::_textBegin(const ftk::V2I& widgetPos)
        {
            FTK_P();
            const SourceHit hit = _hitTest(widgetPos);
            if (hit.index < 0)
            {
                return;
            }
            const auto& drawModel = p.drawModel;
            p.strokeSource = hit.index;
            p.strokeTime = p.currentTime;
            p.stroke = models::ReviewStroke();
            p.stroke.kind = models::ReviewStrokeKind::Text;
            p.stroke.color = drawModel->getColor();
            p.stroke.width = drawModel->getSize();
            p.stroke.textSize = drawModel->getTextSize();
            p.stroke.points.push_back(hit.pos);
            p.textEditing = true;
            // The viewport takes the keyboard only while text is typed into
            // it: the rest of the time the keys are the application's. The
            // window has to be told text input is wanted, as a line edit
            // tells it, or the keys arrive and the characters never do.
            setAcceptsKeyFocus(true);
            takeKeyFocus();
            if (auto window = getWindow())
            {
                window->setTextInput(true);
                const ftk::Box2I& g = getGeometry();
                window->setTextInputArea(ftk::Box2I(
                    g.min.x + widgetPos.x,
                    g.min.y + widgetPos.y,
                    1,
                    static_cast<int>(drawModel->getTextSize() * hit.scale * getZoom())));
            }
            setDrawUpdate();
        }

        void Viewport::_textEnd(bool commit)
        {
            FTK_P();
            if (!p.textEditing)
            {
                return;
            }
            p.textEditing = false;
            setAcceptsKeyFocus(p.selection.has_value());
            if (auto window = getWindow())
            {
                window->setTextInput(false);
            }
            if (p.textEditingSelection)
            {
                // The selected text, written again: kept as it is now, or
                // removed where nothing is left of it, or left as it was.
                p.textEditingSelection = false;
                if (commit && p.selection.has_value())
                {
                    if (!p.stroke.text.empty())
                    {
                        p.annotationsModel->setStroke(
                            p.selection->sourceId,
                            p.selection->time,
                            p.selection->index,
                            p.stroke);
                    }
                    else
                    {
                        p.annotationsModel->removeStroke(
                            p.selection->sourceId,
                            p.selection->time,
                            p.selection->index);
                        _selectionClear();
                    }
                }
                p.stroke = models::ReviewStroke();
                p.strokeSource = -1;
                p.strokeTime.reset();
                setDrawUpdate();
                return;
            }
            if (!commit || p.stroke.text.empty())
            {
                p.stroke = models::ReviewStroke();
                p.strokeSource = -1;
                p.strokeTime.reset();
                setDrawUpdate();
                return;
            }
            _drawEnd();
        }

        void Viewport::_selectPress(const ftk::V2I& widgetPos)
        {
            FTK_P();
            p.handle = -1;
            // A press on a handle of the selected shape drags that point.
            if (p.selection.has_value())
            {
                const auto strokes = p.annotationsModel->getStrokes(p.selection->sourceId, p.selection->time);
                if (p.selection->index < strokes.size() &&
                    isTwoPointShape(strokes[p.selection->index].kind) &&
                    strokes[p.selection->index].points.size() > 1)
                {
                    const auto& stroke = strokes[p.selection->index];
                    for (int i = 0; i < 2; ++i)
                    {
                        const ftk::V2F s = _imageToWidget(p.selection->source, stroke.points[i]);
                        if (std::abs(s.x - widgetPos.x) <= handleSize &&
                            std::abs(s.y - widgetPos.y) <= handleSize)
                        {
                            p.handle = i;
                            p.moving = stroke;
                            setDrawUpdate();
                            return;
                        }
                    }
                }
            }
            const SourceHit hit = _hitTest(widgetPos);
            std::optional<size_t> found;
            if (hit.index >= 0 && p.currentTime.has_value())
            {
                // Within a few screen pixels, whatever the zoom.
                const double zoom = getZoom();
                const float radius = 6.F / std::max(.001F, hit.scale * static_cast<float>(zoom));
                const auto& active = p.filesModel->getActive();
                if (hit.index < static_cast<int>(active.size()))
                {
                    found = p.annotationsModel->findStroke(
                        active[hit.index]->id,
                        *p.currentTime,
                        hit.pos,
                        radius);
                }
            }
            if (!found.has_value())
            {
                _selectionClear();
                return;
            }
            const auto& active = p.filesModel->getActive();
            const auto now = std::chrono::steady_clock::now();
            const bool same =
                p.selection.has_value() &&
                p.selection->source == hit.index &&
                p.selection->index == *found;
            const bool doubleClick =
                same &&
                std::chrono::duration_cast<std::chrono::milliseconds>(now - p.selectPressTime).count() < 400;
            p.selectPressTime = now;
            Private::Selection selection;
            selection.source = hit.index;
            selection.sourceId = active[hit.index]->id;
            selection.time = *p.currentTime;
            selection.index = *found;
            p.selection = selection;
            // The keyboard is the selection's: Delete, Return, Escape.
            setAcceptsKeyFocus(true);
            takeKeyFocus();
            if (doubleClick)
            {
                _selectionEditText();
            }
            else
            {
                const auto strokes = p.annotationsModel->getStrokes(selection.sourceId, selection.time);
                p.moving = strokes[selection.index];
                p.movePressPos = hit.pos;
            }
            setDrawUpdate();
        }

        void Viewport::_selectMove(const ftk::V2I& widgetPos, int modifiers)
        {
            FTK_P();
            if (!p.selection.has_value() || !p.moving.has_value())
            {
                return;
            }
            const auto clamped = _imagePosClamped(p.selection->source, widgetPos);
            if (!clamped.has_value())
            {
                return;
            }
            const auto strokes = p.annotationsModel->getStrokes(p.selection->sourceId, p.selection->time);
            if (p.selection->index >= strokes.size())
            {
                return;
            }
            if (p.handle >= 0)
            {
                models::ReviewStroke stroke = strokes[p.selection->index];
                if (p.handle < static_cast<int>(stroke.points.size()))
                {
                    const ftk::V2F& other = stroke.points[1 - p.handle];
                    stroke.points[p.handle] = constrainShape(stroke.kind, other, *clamped, modifiers);
                    p.moving = stroke;
                    setDrawUpdate();
                }
                return;
            }
            ftk::V2F offset(clamped->x - p.movePressPos.x, clamped->y - p.movePressPos.y);
            models::ReviewStroke stroke = strokes[p.selection->index];
            // The stroke stops at the edge of the picture rather than
            // leaving it: the offset is held to what keeps its bounds
            // inside.
            const auto& video = p.videoFrames[p.selection->source];
            if (!video.layers.empty() && video.layers[0].image)
            {
                const ftk::Size2I imageSize = video.layers[0].image->getSize();
                float minX = 0.F, minY = 0.F, maxX = 0.F, maxY = 0.F;
                bool first = true;
                for (const auto& point : models::shapePath(stroke.kind, stroke.points))
                {
                    if (first)
                    {
                        minX = maxX = point.x;
                        minY = maxY = point.y;
                        first = false;
                    }
                    minX = std::min(minX, point.x);
                    minY = std::min(minY, point.y);
                    maxX = std::max(maxX, point.x);
                    maxY = std::max(maxY, point.y);
                }
                if (models::ReviewStrokeKind::Text == stroke.kind)
                {
                    maxX = minX + std::max(stroke.textSize, stroke.text.size() * stroke.textSize * .6F);
                    maxY = minY + stroke.textSize;
                }
                if (!first)
                {
                    offset.x = std::max(-minX, std::min(imageSize.w - maxX, offset.x));
                    offset.y = std::max(-minY, std::min(imageSize.h - maxY, offset.y));
                }
            }
            for (auto& point : stroke.points)
            {
                point.x += offset.x;
                point.y += offset.y;
            }
            p.moving = stroke;
            setDrawUpdate();
        }

        void Viewport::_selectRelease(bool cancel)
        {
            FTK_P();
            if (p.selection.has_value() && p.moving.has_value() && !cancel)
            {
                // One undo step for the move; a press that did not move
                // changes nothing.
                p.annotationsModel->setStroke(
                    p.selection->sourceId,
                    p.selection->time,
                    p.selection->index,
                    *p.moving);
            }
            p.moving.reset();
            p.handle = -1;
            setDrawUpdate();
        }

        void Viewport::_selectionClear()
        {
            FTK_P();
            if (p.textEditingSelection)
            {
                _textEnd(true);
            }
            if (p.selection.has_value())
            {
                p.selection.reset();
                p.moving.reset();
                if (!p.textEditing)
                {
                    setAcceptsKeyFocus(false);
                }
                setDrawUpdate();
            }
        }

        void Viewport::_selectionEditText()
        {
            FTK_P();
            if (!p.selection.has_value() || p.textEditing)
            {
                return;
            }
            const auto strokes = p.annotationsModel->getStrokes(p.selection->sourceId, p.selection->time);
            if (p.selection->index >= strokes.size() ||
                strokes[p.selection->index].kind != models::ReviewStrokeKind::Text)
            {
                return;
            }
            p.moving.reset();
            p.strokeSource = p.selection->source;
            p.strokeTime = p.selection->time;
            p.stroke = strokes[p.selection->index];
            p.textEditing = true;
            p.textEditingSelection = true;
            setAcceptsKeyFocus(true);
            takeKeyFocus();
            if (auto window = getWindow())
            {
                window->setTextInput(true);
            }
            setDrawUpdate();
        }

        void Viewport::_selectionUpdate(const std::function<void(models::ReviewStroke&)>& change)
        {
            FTK_P();
            if (!p.selection.has_value() || p.moving.has_value() || p.textEditing)
            {
                return;
            }
            const auto strokes = p.annotationsModel->getStrokes(p.selection->sourceId, p.selection->time);
            if (p.selection->index >= strokes.size())
            {
                return;
            }
            models::ReviewStroke stroke = strokes[p.selection->index];
            change(stroke);
            p.annotationsModel->setStroke(
                p.selection->sourceId,
                p.selection->time,
                p.selection->index,
                stroke);
        }

        void Viewport::_erase(const ftk::V2I& widgetPos)
        {
            FTK_P();
            const SourceHit hit = _hitTest(widgetPos);
            if (hit.index < 0)
            {
                return;
            }
            {
                const auto& active = p.filesModel->getActive();
                if (hit.index < static_cast<int>(active.size()))
                {
                    // The eraser removes whole strokes it touches; its reach
                    // follows the tool size.
                    p.annotationsModel->eraseStrokes(
                        active[hit.index]->id,
                        *p.currentTime,
                        hit.pos,
                        p.drawModel->getSize());
                }
            }
        }

        void Viewport::drawEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent& event)
        {
            tl::ui::Viewport::drawEvent(drawRect, event);
            FTK_P();

            const ftk::Box2I& g = getGeometry();
            const auto& active = p.filesModel->getActive();

            // Keep the overlay inside the viewport: a stroke zoomed past the
            // edges would otherwise be painted over the panels and toolbars.
            const bool clipRectEnabledPrev = event.render->getClipRectEnabled();
            const ftk::Box2I clipRectPrev = event.render->getClipRect();
            event.render->setClipRectEnabled(true);
            event.render->setClipRect(ftk::intersect(g, clipRectPrev));

            const auto boxes = _sourceBoxes();
            auto sourceScale = [this, &boxes](int index) -> float
            {
                FTK_P();
                if (index < 0 ||
                    index >= static_cast<int>(boxes.size()) ||
                    index >= static_cast<int>(p.videoFrames.size()))
                {
                    return 1.F;
                }
                const auto& video = p.videoFrames[index];
                if (video.layers.empty() || !video.layers[0].image)
                {
                    return 1.F;
                }
                const int w = video.layers[0].image->getSize().w;
                return w > 0 ? boxes[index].w() / static_cast<float>(w) : 1.F;
            };
            auto sourceIndex = [&active](const std::string& id) -> int
            {
                for (size_t i = 0; i < active.size(); ++i)
                {
                    if (active[i]->id == id)
                    {
                        return static_cast<int>(i);
                    }
                }
                return -1;
            };

            // The strokes already committed: this frame's, and faded, the
            // frames' on either side where the onion skin is on.
            const auto& annotations = p.annotationsModel->getAnnotations();
            std::optional<OTIO_NS::RationalTime> prev;
            std::optional<OTIO_NS::RationalTime> next;
            if (p.currentTime.has_value() && p.drawModel->isOnionSkin())
            {
                const OTIO_NS::RationalTime one(1.0, p.currentTime->rate());
                prev = *p.currentTime - one;
                next = *p.currentTime + one;
            }
            for (const auto& annotation : annotations)
            {
                float alpha = 1.F;
                if (models::sameTime(annotation.time, p.currentTime))
                {
                    alpha = 1.F;
                }
                else if (
                    models::sameTime(annotation.time, prev) ||
                    models::sameTime(annotation.time, next))
                {
                    alpha = .35F;
                }
                else
                {
                    continue;
                }
                const int index = sourceIndex(annotation.sourceId);
                if (index < 0 || !_sourceShown(index))
                {
                    continue;
                }
                const float scale = sourceScale(index);
                const bool selectedHere =
                    p.selection.has_value() &&
                    p.selection->source == index &&
                    models::sameTime(annotation.time, p.selection->time);
                for (size_t i = 0; i < annotation.strokes.size(); ++i)
                {
                    const auto& stroke = annotation.strokes[i];
                    const bool selected = selectedHere && p.selection->index == i;
                    // A stroke being moved or written again is drawn from
                    // the copy below, not from here.
                    if (selected && (p.moving.has_value() || p.textEditingSelection))
                    {
                        continue;
                    }
                    _drawStroke(event, index, stroke, scale, alpha, false);
                    if (selected)
                    {
                        _drawSelection(event, index, stroke, scale);
                    }
                }
            }

            // The stroke being moved.
            if (p.selection.has_value() && p.moving.has_value())
            {
                _drawStroke(event, p.selection->source, *p.moving, sourceScale(p.selection->source), 1.F, false);
                _drawSelection(event, p.selection->source, *p.moving, sourceScale(p.selection->source));
            }

            // The stroke under the cursor, or the text being typed.
            if (p.strokeSource >= 0)
            {
                _drawStroke(event, p.strokeSource, p.stroke, sourceScale(p.strokeSource), 1.F, p.textEditing);
            }

            event.render->setClipRectEnabled(clipRectEnabledPrev);
            event.render->setClipRect(clipRectPrev);
        }

        void Viewport::_drawStroke(
            const ftk::DrawEvent& event,
            int index,
            const models::ReviewStroke& stroke,
            float scale,
            float alpha,
            bool caret)
        {
            const ftk::Box2I& g = getGeometry();
            drawStroke(
                event.render,
                event.fontSystem,
                stroke,
                [this, index, &g](const ftk::V2F& point)
                {
                    ftk::V2F out = _imageToWidget(index, point);
                    out.x += g.min.x;
                    out.y += g.min.y;
                    return out;
                },
                scale * static_cast<float>(getZoom()),
                alpha,
                caret);
        }

        void Viewport::_drawSelection(
            const ftk::DrawEvent& event,
            int index,
            const models::ReviewStroke& stroke,
            float scale)
        {
            if (stroke.points.empty())
            {
                return;
            }
            // A thin frame around what the stroke covers on the screen,
            // a little outside it.
            const ftk::Box2I& g = getGeometry();
            const float zoom = static_cast<float>(getZoom());
            float minX = 0.F, minY = 0.F, maxX = 0.F, maxY = 0.F;
            bool first = true;
            auto add = [&](const ftk::V2F& point)
            {
                ftk::V2F s = _imageToWidget(index, point);
                s.x += g.min.x;
                s.y += g.min.y;
                if (first)
                {
                    minX = maxX = s.x;
                    minY = maxY = s.y;
                    first = false;
                }
                minX = std::min(minX, s.x);
                minY = std::min(minY, s.y);
                maxX = std::max(maxX, s.x);
                maxY = std::max(maxY, s.y);
            };
            if (models::ReviewStrokeKind::Text == stroke.kind)
            {
                ftk::FontInfo fontInfo;
                fontInfo.size = std::max(1, static_cast<int>(std::round(stroke.textSize * scale * zoom)));
                const ftk::Size2I size = event.fontSystem->getSize(stroke.text, fontInfo);
                add(stroke.points.front());
                const float w = size.w / (scale * zoom);
                const float h = std::max(static_cast<float>(size.h), fontInfo.size * 1.F) / (scale * zoom);
                add(ftk::V2F(stroke.points.front().x + w, stroke.points.front().y + h));
            }
            else
            {
                for (const auto& point : models::shapePath(stroke.kind, stroke.points))
                {
                    add(point);
                }
            }
            // The two points of a shape, as handles to drag.
            if (isTwoPointShape(stroke.kind) && stroke.points.size() > 1)
            {
                for (int i = 0; i < 2; ++i)
                {
                    ftk::V2F s = _imageToWidget(index, stroke.points[i]);
                    s.x += g.min.x;
                    s.y += g.min.y;
                    const float h = handleSize;
                    event.render->drawRect(
                        ftk::Box2F(s.x - h / 2.F - 1.F, s.y - h / 2.F - 1.F, h + 2.F, h + 2.F),
                        ftk::Color4F(0.F, 0.F, 0.F, .6F));
                    event.render->drawRect(
                        ftk::Box2F(s.x - h / 2.F, s.y - h / 2.F, h, h),
                        ftk::Color4F(1.F, 1.F, 1.F, .95F));
                }
            }
            const float pad = stroke.width * scale * zoom / 2.F + 6.F;
            const ftk::Box2F box(minX - pad, minY - pad, maxX - minX + pad * 2.F, maxY - minY + pad * 2.F);
            const ftk::Color4F color(1.F, 1.F, 1.F, .85F);
            const ftk::Color4F shadow(0.F, 0.F, 0.F, .5F);
            // A dark line under a light one, so the frame reads on any picture.
            for (int pass = 0; pass < 2; ++pass)
            {
                const float t = pass ? 1.F : 3.F;
                const float o = pass ? 0.F : 1.F;
                const ftk::Color4F& c = pass ? color : shadow;
                event.render->drawRect(ftk::Box2F(box.min.x - o, box.min.y - o, box.w() + o * 2.F, t), c);
                event.render->drawRect(ftk::Box2F(box.min.x - o, box.max.y - t + o + 1.F, box.w() + o * 2.F, t), c);
                event.render->drawRect(ftk::Box2F(box.min.x - o, box.min.y - o, t, box.h() + o * 2.F), c);
                event.render->drawRect(ftk::Box2F(box.max.x - t + o + 1.F, box.min.y - o, t, box.h() + o * 2.F), c);
            }
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
                    if (p.b.empty())
                    {
                        s = "No B file selected";
                    }
                    else if (p.a && p.a->videoLayers.empty())
                    {
                        // The player is built from "A", and one with no
                        // video runs no video at all -- the comparison has
                        // nothing to composite, however real the B file is.
                        s = "A has no video to compare";
                    }
                    // Otherwise the frames are still loading, which is not
                    // worth an announcement.
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
                    else if (tl::CompareTime::Absolute == p.compareTime &&
                        p.a->timeRange.has_value())
                    {
                        // Timecode sync with no timecode in common: every B
                        // frame maps outside its file, so B never draws.
                        // Decided from the ranges rather than the missing
                        // frame, which is also what a frame still loading
                        // looks like.
                        const bool noOverlap = std::all_of(
                            p.b.begin(),
                            p.b.end(),
                            [this](const std::shared_ptr<models::FilesModelItem>& i)
                            {
                                return i->timeRange.has_value() &&
                                    !i->timeRange->intersects(
                                        *_p->a->timeRange);
                            });
                        if (noOverlap)
                        {
                            s = "A and B timecodes do not overlap";
                        }
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

            // For a sequence, the image on screen rather than the
            // pattern: which frame a picture came from is the question a
            // sequence of renders gets asked. Sequence time is the frame
            // number, so the value is the name (#490).
            std::string s =
                p.path.isSeq() && p.currentTime.has_value() ?
                p.path.getFrame(static_cast<int64_t>(p.currentTime->value())) :
                p.path.getFileName();
            p.fileNameLabel->setText(!s.empty() ? s : "(No file)");
            ftk::setScreenshotTag(p.fileNameLabel, "View.HUD.FileName");

            std::vector<std::string> info;
            if (!p.ioInfo.video.empty())
            {
                info.push_back(std::string(ftk::Format("V: {0}").
                    arg(ftk::getLabel(
                        tl::getVideoInfo(p.ioInfo, p.videoLayer)))));
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
            // as values come and go moves exactly where the eye is. The
            // widths hold a sign and two digits, so the ordinary zero to one
            // range and a little either side of it do not move the line;
            // beyond that the field grows, as it did before. Without a
            // sample -- every time the pointer leaves the image -- the line
            // is a dash for each part and then spaces out to the width of a
            // sample: the HUD font is monospaced, so the same number of
            // characters is the same width. A dash in every field kept the
            // width too, but read as four missing values rather than none.
            const int colorWidth = 5;
            const int pickWidth = 4;
            const std::string colorPickerFormat =
                "Color: {0} {1} {2} {3}, Pixel: {4}, {5}";
            const size_t colorPickerWidth =
                ftk::Format(colorPickerFormat).
                arg(0.F, 2, colorWidth).
                arg(0.F, 2, colorWidth).
                arg(0.F, 2, colorWidth).
                arg(0.F, 2, colorWidth).
                arg(0, pickWidth).
                arg(0, pickWidth).str().size();
            std::string colorPickerText = "Color: -, Pixel: -";
            colorPickerText.resize(
                std::max(colorPickerText.size(), colorPickerWidth),
                ' ');
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
                (p.toastActive || p.toastHint) && !p.toastLabel->getText().empty();
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
            // With no file there is nothing for the items to be about but
            // that there is no file: a placeholder for each, a black color
            // swatch, an empty cache bar, a zoom of an image that is not
            // there, reads as information (DJV #900). The options stay as
            // they are, and the items come back with the next file.
            const bool file = getPlayer() != nullptr;
            for (const auto& i : options.items)
            {
                const bool visible =
                    i.second != models::HUDPos::None &&
                    (file || models::HUDItem::FileName == i.first);
                p.hudWidgets[i.first]->setParent(visible ?
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
