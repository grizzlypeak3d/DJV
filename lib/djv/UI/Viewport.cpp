// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/Viewport.h>

#include <djv/UI/ViewportBinding.h>

#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/HUD.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/Stroke.h>
#include <djv/UI/StrokeDraw.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/Timeline/Util.h>

#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/IWindow.h>
#include <ftk/Core/FontSystem.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/SysLogModel.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Spacer.h>
#include <ftk/Core/Format.h>

#include <chrono>
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

            // The top row of the HUD: one box on the left and one on the
            // right. A row layout with a spacer between them let the spacer
            // go negative when the two were wider than the viewport, and the
            // right box was drawn over the left. Here the right box has the
            // room it asks for, and the left box what is left, which its
            // labels elide to.
            class HUDRow : public ftk::IWidget
            {
            protected:
                void _init(
                    const std::shared_ptr<ftk::Context>& context,
                    const std::shared_ptr<ftk::IWidget>& parent)
                {
                    IWidget::_init(context, "djv::ui::HUDRow", parent);
                }

                HUDRow() = default;

            public:
                virtual ~HUDRow() {}

                static std::shared_ptr<HUDRow> create(
                    const std::shared_ptr<ftk::Context>& context,
                    const std::shared_ptr<ftk::IWidget>& parent = nullptr)
                {
                    auto out = std::shared_ptr<HUDRow>(new HUDRow);
                    out->_init(context, parent);
                    return out;
                }

                void setBoxes(
                    const std::shared_ptr<ftk::IWidget>& left,
                    const std::shared_ptr<ftk::IWidget>& right)
                {
                    _left = left;
                    _right = right;
                    _left->setParent(shared_from_this());
                    _right->setParent(shared_from_this());
                    setSizeUpdate();
                }

                ftk::Size2I getSizeHint() const override
                {
                    ftk::Size2I out;
                    int count = 0;
                    for (const auto& box : { _left, _right })
                    {
                        if (box && box->isVisible(false))
                        {
                            const ftk::Size2I hint = box->getSizeHint();
                            out.w += hint.w;
                            out.h = std::max(out.h, hint.h);
                            ++count;
                        }
                    }
                    if (count > 1)
                    {
                        out.w += _spacing;
                    }
                    return out;
                }

                void setGeometry(const ftk::Box2I& value) override
                {
                    IWidget::setGeometry(value);
                    const ftk::Box2I& g = getGeometry();
                    // Each box at the top or the bottom of the row, as it
                    // says, so the boxes of a bottom row line up at the
                    // bottom.
                    const auto y = [&g](const std::shared_ptr<ftk::IWidget>& box, int h)
                    {
                        return ftk::VAlign::Bottom == box->getVAlign() ?
                            g.max.y + 1 - h :
                            g.min.y;
                    };
                    int rightWidth = 0;
                    if (_right && _right->isVisible(false))
                    {
                        const ftk::Size2I hint = _right->getSizeHint();
                        rightWidth = std::min(hint.w, g.w());
                        _right->setGeometry(ftk::Box2I(
                            g.max.x + 1 - rightWidth,
                            y(_right, hint.h),
                            rightWidth,
                            hint.h));
                    }
                    if (_left && _left->isVisible(false))
                    {
                        const ftk::Size2I hint = _left->getSizeHint();
                        const int room = g.w() - (rightWidth > 0 ? rightWidth + _spacing : 0);
                        _left->setGeometry(ftk::Box2I(
                            g.min.x,
                            y(_left, hint.h),
                            std::max(0, std::min(hint.w, room)),
                            hint.h));
                    }
                }

                void sizeHintEvent(const ftk::SizeHintEvent& event) override
                {
                    IWidget::sizeHintEvent(event);
                    _spacing = event.style->getSizeRole(
                        ftk::SizeRole::SpacingSmall,
                        event.displayScale);
                }

            private:
                std::shared_ptr<ftk::IWidget> _left;
                std::shared_ptr<ftk::IWidget> _right;
                int _spacing = 0;
            };
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
            // Whether the picture stands in for a frame the media does not
            // have, and the frame it repeats when there is one.
            bool missing = false;
            std::optional<int64_t> heldFrom;
            // Kept for the annotation hit test and overlay, which need the
            // image sizes to map screen to image pixels.
            std::vector<tl::VideoFrame> videoFrames;
            tl::PlayerCacheInfo cacheInfo;
            double viewZoom = 0.0;
            models::MouseActionBinding frameShuttleBinding =
                models::MouseActionBinding(ftk::MouseButton::Left, ftk::KeyModifier::Shift);
            float frameShuttleScale = 1.F;
            // The last plain left press, for the double click that frames
            // the view.
            std::chrono::steady_clock::time_point lastPressTime;
            ftk::V2I lastPressPos;

            std::shared_ptr<ftk::Label> fileNameLabel;
            std::shared_ptr<ftk::Label> cacheLabel;
            std::shared_ptr<ftk::Label> timeLabel;
            std::shared_ptr<ftk::Label> viewZoomLabel;
            std::shared_ptr<ftk::ColorSwatch> colorPickerSwatch;
            std::shared_ptr<ftk::Label> colorPickerLabel;
            std::shared_ptr<ftk::Label> infoLabel;
            std::shared_ptr<ftk::Label> renderLabel;
            std::shared_ptr<ftk::Label> lutLabel;
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
            std::shared_ptr<ViewportBinding> binding;
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
            std::shared_ptr<ftk::Observer<tl::CompareTime> > compareTimeObserver;
            std::shared_ptr<ftk::Observer<bool> > drawEnabledObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > fgOptionsObserver;
            std::shared_ptr<ftk::Observer<double> > viewZoomObserver;
            std::shared_ptr<ftk::ListObserver<ftk::LogItem> > messagesObserver;
            std::shared_ptr<ftk::Observer<models::HUDOptions> > hudOptionsObserver;
            std::shared_ptr<ftk::Observer<bool> > timeUnitsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            models::MouseSettings mouseSettings;
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
            //! Whether the press under way is a finger's.
            bool touchInput = false;
            //! How many undo steps the eraser has made since the press, to
            //! take back if the press is cancelled.
            size_t eraseSteps = 0;

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
            // Elided in the middle, which keeps the frame number and the
            // extension, when the HUD is too narrow for it.
            p.fileNameLabel->setElide(true, ftk::ElideMode::Middle);
            p.fileNameLabel->setElideFullWidth(true);

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

            p.lutLabel = ftk::Label::create(context);
            p.lutLabel->setFont(ftk::FontType::Mono);
            p.lutLabel->setMarginRole(ftk::SizeRole::MarginSmall);
            p.lutLabel->setElide(true, ftk::ElideMode::Middle);
            p.lutLabel->setElideFullWidth(true);

            p.hudWidgets[models::HUDItem::FileName] = p.fileNameLabel;
            p.hudWidgets[models::HUDItem::Cache] = p.cacheLabel;
            p.hudWidgets[models::HUDItem::Time] = p.timeLabel;
            p.hudWidgets[models::HUDItem::ViewZoom] = p.viewZoomLabel;
            p.hudWidgets[models::HUDItem::ColorPicker] = colorPickerLayout;
            p.hudWidgets[models::HUDItem::Info] = p.infoLabel;
            p.hudWidgets[models::HUDItem::Render] = p.renderLabel;
            p.hudWidgets[models::HUDItem::LUT] = p.lutLabel;

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

            auto topRow = HUDRow::create(context, p.hudLayout);
            topRow->setVAlign(ftk::VAlign::Top);
            // Filling, so the box takes the width the row gives it and its
            // labels elide to it, rather than the width it asks for.
            p.hudLayouts[models::HUDPos::TopLeft]->setHAlign(ftk::HAlign::Fill);
            p.hudLayouts[models::HUDPos::TopLeft]->setClipChildren(true);
            topRow->setBoxes(
                p.hudLayouts[models::HUDPos::TopLeft],
                p.hudLayouts[models::HUDPos::TopRight]);

            auto spacer = ftk::Spacer::create(
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
            auto bottomRow = HUDRow::create(context, bottomLayout);
            bottomRow->setVAlign(ftk::VAlign::Bottom);
            p.hudLayouts[models::HUDPos::BottomLeft]->setHAlign(ftk::HAlign::Fill);
            p.hudLayouts[models::HUDPos::BottomLeft]->setClipChildren(true);
            bottomRow->setBoxes(
                p.hudLayouts[models::HUDPos::BottomLeft],
                p.hudLayouts[models::HUDPos::BottomRight]);

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
                    // A finger draws from where it lands, rather than after
                    // the touch screen's wait for a second finger; a second
                    // finger still cancels the stroke, as it cancels any
                    // press (see mouseReleaseEvent).
                    setTouchImmediate(value);
                    _mouseBindingsUpdate();
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

            p.fgOptionsObserver = ftk::Observer<tl::ForegroundOptions>::create(
                viewportModel->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
                {
                    setForegroundOptions(value);
                });

            p.binding = ViewportBinding::create(
                filesModel,
                colorModel,
                viewportModel,
                std::dynamic_pointer_cast<tl::ui::Viewport>(shared_from_this()));
            p.binding->setChangedCallback(
                [this]
                {
                    FTK_P();
                    p.compare = getCompareOptions().compare;
                    _compareUpdate();
                    // The heads up display says what is being rendered,
                    // which the aspect ratio and the color buffer change.
                    // Without this it would only catch up when something
                    // else refreshes it -- which is every frame while
                    // playing, and nothing at all while stopped.
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

            // The units or whether timecode is drop frame.
            p.timeUnitsObserver = ftk::Observer<bool>::create(
                timeUnitsModel->observeTimeUnitsChanged(),
                [this](bool)
                {
                    _hudUpdate();
                });

            // The LUT the HUD names.
            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                observeLUTOptions(),
                [this](const tl::LUTOptions&)
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
                    _p->mouseSettings = value;
                    _mouseBindingsUpdate();
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
                        p.binding->setVideoFramesSize(value.size());
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
                _drawContinue(event.pos - getGeometry().min, event.keyModifiers, event.pressure);
                break;
            case Private::MouseMode::Erase:
                _erase(event.pos - getGeometry().min);
                break;
            case Private::MouseMode::Select:
                _selectMove(event.pos - getGeometry().min, event.keyModifiers);
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

            // A double click, or a double tap, frames the view: the
            // convention of every touch screen, and the frame key's
            // equivalent for a mouse. Not while drawing, where two quick
            // dots are two dots.
            if (ftk::MouseButton::Left == event.button &&
                0 == event.modifiers &&
                !p.drawModel->isEnabled())
            {
                const auto now = std::chrono::steady_clock::now();
                const bool doubleClick =
                    now - p.lastPressTime < std::chrono::milliseconds(400) &&
                    ftk::length(event.pos - p.lastPressPos) < 24;
                p.lastPressTime = doubleClick ? std::chrono::steady_clock::time_point() : now;
                p.lastPressPos = event.pos;
                if (doubleClick)
                {
                    event.accept = true;
                    setFrameView(true);
                    return;
                }
            }

            // Drawing owns the plain left button while it is enabled, which is
            // why it is an explicit mode: it displaces the frame shuttle.
            if (p.drawModel->isEnabled() &&
                ftk::MouseButton::Left == event.button &&
                0 == event.modifiers)
            {
                p.touchInput = event.touch;
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
                    p.eraseSteps = 0;
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
                    _drawBegin(pos, event.pen, event.pressure);
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
            else if (Private::MouseMode::Erase == p.mouse.mode && event.cancel)
            {
                // The first finger of a pinch was not erasing: what it
                // took is put back.
                for (; p.eraseSteps > 0; --p.eraseSteps)
                {
                    p.annotationsModel->undo();
                }
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
                p.binding->getDisplayOptions().aspectRatio,
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

        models::ImageMapping Viewport::_imageMapping(int index) const
        {
            FTK_P();
            models::ImageMapping out;
            const auto boxes = _sourceBoxes();
            if (index < 0 ||
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
            out.box = boxes[index];
            out.imageSize = video.layers[0].image->getSize();
            out.mirrorX = p.binding->getDisplayOptions().mirror.x;
            out.mirrorY = p.binding->getDisplayOptions().mirror.y;
            return out;
        }

        ftk::V2F Viewport::_widgetToRender(const ftk::V2I& widgetPos) const
        {
            // Widget -> render space: the inverse of translate(viewPos) * scale.
            const double zoom = getZoom();
            const ftk::V2I& viewPos = getViewPos();
            return zoom > 0.0 ?
                ftk::V2F(
                    static_cast<float>((widgetPos.x - viewPos.x) / zoom),
                    static_cast<float>((widgetPos.y - viewPos.y) / zoom)) :
                ftk::V2F();
        }

        std::optional<ftk::V2F> Viewport::_imagePosClamped(int index, const ftk::V2I& widgetPos) const
        {
            std::optional<ftk::V2F> out;
            const models::ImageMapping mapping = _imageMapping(index);
            if (getZoom() <= 0.0 || !mapping.isValid())
            {
                return out;
            }
            // The same mapping as the hit test, without the test.
            out = mapping.clamp(mapping.toImage(_widgetToRender(widgetPos)));
            return out;
        }

        Viewport::SourceHit Viewport::_hitTest(const ftk::V2I& widgetPos) const
        {
            SourceHit out;
            if (getZoom() <= 0.0)
            {
                return out;
            }
            const ftk::V2F render = _widgetToRender(widgetPos);
            const size_t count = _sourceBoxes().size();
            for (size_t i = 0; i < count; ++i)
            {
                if (!_sourceShown(static_cast<int>(i)))
                {
                    continue;
                }
                const models::ImageMapping mapping = _imageMapping(static_cast<int>(i));
                if (!mapping.contains(render))
                {
                    continue;
                }
                // Render space -> the source's own pixels, unmirrored.
                // Strokes are stored unmirrored, and follow the image when
                // the mirror changes.
                out.index = static_cast<int>(i);
                out.pos = mapping.toImage(render);
                out.scale = mapping.scale();
                break;
            }
            return out;
        }

        ftk::V2F Viewport::_imageToWidget(int index, const ftk::V2F& imagePos) const
        {
            const models::ImageMapping mapping = _imageMapping(index);
            if (!mapping.isValid())
            {
                return ftk::V2F();
            }
            // Stored image pixels back to where the mirror shows them.
            const ftk::V2F render = mapping.toRender(imagePos);
            const double zoom = getZoom();
            const ftk::V2I& viewPos = getViewPos();
            return ftk::V2F(
                static_cast<float>(viewPos.x + render.x * zoom),
                static_cast<float>(viewPos.y + render.y * zoom));
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
            //! The size of a handle on the screen, in pixels at a display
            //! scale of one.
            const float handleSize = 8.F;

            //! How near a press has to be to take a handle or a stroke, and
            //! the least an eraser reaches, in pixels at a display scale of
            //! one: a pointer's, and a fingertip's, which covers far more
            //! than it hits.
            const float pointerReach = 8.F;
            const float fingerReach = 22.F;
        }

        void Viewport::_drawBegin(const ftk::V2I& widgetPos, bool pen, float pressure)
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
                models::strokeBegin(p.stroke, hit.pos, pen, pressure);
                setDrawUpdate();
            }
        }

        void Viewport::_drawContinue(const ftk::V2I& widgetPos, int modifiers, float pressure)
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
                models::strokeContinue(p.stroke, *clamped, pressure, _isConstrained(modifiers));
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
            models::strokeContinue(p.stroke, hit.pos, pressure, false);
            setDrawUpdate();
        }

        void Viewport::_drawEnd()
        {
            FTK_P();
            // A shape wants its second point: a click with no drag is nothing.
            // Ink and text want one.
            if (p.strokeSource >= 0 &&
                p.strokeTime.has_value() &&
                models::strokeComplete(p.stroke))
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
            // Where the text is, then the keyboard: iOS works out how far to
            // lift the window as the keyboard is asked for, and with the
            // place told after it the first keyboard covered the text.
            _textInputAreaUpdate();
            if (auto window = getWindow())
            {
                window->setTextInput(true);
            }
            setDrawUpdate();
        }

        void Viewport::_textInputAreaUpdate()
        {
            FTK_P();
            auto window = getWindow();
            if (!window || p.strokeSource < 0 || p.stroke.points.empty())
            {
                return;
            }
            // The text's top left and the height of its line on the screen:
            // the line, not the font size, which leaves out what hangs below
            // the baseline, and a large size lost the bottom of its letters
            // under the keyboard. A little room below it besides.
            const ftk::V2F& a = p.stroke.points.front();
            const ftk::V2F top = _imageToWidget(p.strokeSource, a);
            const ftk::V2F bottom = _imageToWidget(
                p.strokeSource,
                ftk::V2F(a.x, a.y + p.stroke.textSize));
            const float size = std::abs(bottom.y - top.y);
            float lineHeight = size * 1.4F;
            if (auto context = getContext())
            {
                if (auto fontSystem = context->getSystem<ftk::FontSystem>())
                {
                    ftk::FontInfo fontInfo;
                    fontInfo.size = std::max(1, static_cast<int>(std::round(size)));
                    lineHeight = fontSystem->getMetrics(fontInfo).lineHeight;
                }
            }
            const ftk::Box2I& g = getGeometry();
            window->setTextInputArea(ftk::Box2I(
                g.min.x + static_cast<int>(top.x),
                g.min.y + static_cast<int>(std::min(top.y, bottom.y)),
                1,
                std::max(1, static_cast<int>(std::round(lineHeight + 8.F * _displayScale())))));
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
                    models::isTwoPointShape(strokes[p.selection->index].kind) &&
                    strokes[p.selection->index].points.size() > 1)
                {
                    const auto& stroke = strokes[p.selection->index];
                    for (int i = 0; i < 2; ++i)
                    {
                        const ftk::V2F s = _imageToWidget(p.selection->source, stroke.points[i]);
                        const float reach = _reach();
                        if (std::abs(s.x - widgetPos.x) <= reach &&
                            std::abs(s.y - widgetPos.y) <= reach)
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
                // Within reach on the screen, whatever the zoom.
                const double zoom = getZoom();
                const float radius = _reach() / std::max(.001F, hit.scale * static_cast<float>(zoom));
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
                    stroke.points[p.handle] = models::constrainShape(stroke.kind, other, *clamped, _isConstrained(modifiers));
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
            // Where this text is, not where the last was; see _textBegin().
            _textInputAreaUpdate();
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

        float Viewport::_displayScale()
        {
            if (auto window = getWindow())
            {
                return window->getDisplayScale();
            }
            return 1.F;
        }

        float Viewport::_reach()
        {
            return (_p->touchInput ? fingerReach : pointerReach) * _displayScale();
        }

        bool Viewport::_isConstrained(int modifiers) const
        {
            return
                (modifiers & static_cast<int>(ftk::KeyModifier::Shift)) ||
                _p->drawModel->isConstrain();
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
                    // follows the tool size, and is never less than a
                    // fingertip's for a finger.
                    float radius = p.drawModel->getSize();
                    if (p.touchInput)
                    {
                        radius = models::eraserRadius(
                            radius,
                            _reach(),
                            hit.scale * static_cast<float>(getZoom()));
                    }
                    const size_t before = p.annotationsModel->getStrokes(
                        active[hit.index]->id,
                        *p.currentTime).size();
                    p.annotationsModel->eraseStrokes(
                        active[hit.index]->id,
                        *p.currentTime,
                        hit.pos,
                        radius);
                    if (p.annotationsModel->getStrokes(
                        active[hit.index]->id,
                        *p.currentTime).size() < before)
                    {
                        ++p.eraseSteps;
                    }
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

        void Viewport::_mouseBindingsUpdate()
        {
            FTK_P();
            const models::MouseSettings& value = p.mouseSettings;
            // The tool, when one is set, takes the plain left
            // button, and any other action bound to that gives
            // it up; the rest keep their bindings, so the
            // modifier keys still work beside the tool.
            // Drawing, while it is on, owns the plain left button the same
            // way a tool does (see mousePressEvent): no tool takes it, and
            // any action bound to it gives it up. The app turns the tool
            // off when drawing is turned on; this holds whatever the order.
            const bool drawing = p.drawModel->isEnabled();
            const auto binding = [&value, drawing](models::MouseAction action, models::MouseTool tool)
            {
                models::MouseActionBinding out;
                const auto i = value.bindings.find(action);
                if (i != value.bindings.end())
                {
                    out = i->second;
                }
                if (drawing)
                {
                    if (ftk::MouseButton::Left == out.button &&
                        ftk::KeyModifier::None == out.modifier)
                    {
                        out = models::MouseActionBinding();
                    }
                }
                else if (models::MouseTool::None != value.tool)
                {
                    if (tool == value.tool)
                    {
                        out = models::MouseActionBinding(ftk::MouseButton::Left);
                    }
                    else if (ftk::MouseButton::Left == out.button &&
                        ftk::KeyModifier::None == out.modifier)
                    {
                        out = models::MouseActionBinding();
                    }
                }
                return out;
            };
            auto b = binding(models::MouseAction::PanView, models::MouseTool::PanView);
            setPanBinding(b.button, b.modifier);
            b = binding(models::MouseAction::CompareWipe, models::MouseTool::CompareWipe);
            setWipeBinding(b.button, b.modifier);
            b = binding(models::MouseAction::Pick, models::MouseTool::Pick);
            setPickBinding(b.button, b.modifier);
            p.frameShuttleBinding = binding(models::MouseAction::FrameShuttle, models::MouseTool::FrameShuttle);
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
            if (models::isTwoPointShape(stroke.kind) && stroke.points.size() > 1)
            {
                for (int i = 0; i < 2; ++i)
                {
                    ftk::V2F s = _imageToWidget(index, stroke.points[i]);
                    s.x += g.min.x;
                    s.y += g.min.y;
                    const float h = handleSize * _displayScale();
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

            // The lines are the models', shared with the iPad.
            models::HUDData data;
            data.path = p.path;
            data.ioInfo = p.ioInfo;
            data.videoLayer = p.videoLayer;
            data.currentTime = p.currentTime;
            if (auto timeUnitsModel = p.timeUnitsModel.lock())
            {
                data.timeLabel = timeUnitsModel->getLabel(p.currentTime);
            }
            data.fps = p.fps;
            data.droppedFrames = p.droppedFrames;
            data.missing = p.missing;
            data.heldFrom = p.heldFrom;
            data.cacheInfo = p.cacheInfo;
            if (p.binding)
            {
                data.aspectRatio = p.binding->getDisplayOptions().aspectRatio;
            }
            data.lutOptions = getLUTOptions();
            data.viewZoom = p.viewZoom;

            p.fileNameLabel->setText(models::getHUDText(models::HUDItem::FileName, data));
            ftk::setScreenshotTag(p.fileNameLabel, "View.HUD.FileName");

            std::string s = models::getHUDText(models::HUDItem::Info, data);
            p.infoLabel->setText(s);
            p.infoLabel->setVisible(!s.empty());
            ftk::setScreenshotTag(p.infoLabel, "View.HUD.Info");

            s = models::getHUDText(models::HUDItem::Render, data);
            p.renderLabel->setText(s);
            p.renderLabel->setVisible(!s.empty());
            ftk::setScreenshotTag(p.renderLabel, "View.HUD.Render");

            p.lutLabel->setText(models::getHUDText(models::HUDItem::LUT, data));
            ftk::setScreenshotTag(p.lutLabel, "View.HUD.LUT");

            p.timeLabel->setText(models::getHUDText(models::HUDItem::Time, data));
            ftk::setScreenshotTag(p.timeLabel, "View.HUD.Time");

            p.viewZoomLabel->setText(models::getHUDText(models::HUDItem::ViewZoom, data));
            ftk::setScreenshotTag(p.viewZoomLabel, "View.HUD.ViewZoom");

            const auto& colorSample = observeColorSample()->get();
            const auto& pick = observePick()->get();
            p.colorPickerSwatch->setColor(
                colorSample.has_value() ?
                    getColorSampleDisplay(colorSample.value()) :
                    ftk::Color4F());
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

            s = models::getHUDText(models::HUDItem::Cache, data);
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
            // In the listed order rather than the enumeration's, so that
            // items sharing a corner stack in it.
            for (const auto item : models::getHUDItemOrder())
            {
                const auto i = options.items.find(item);
                const models::HUDPos pos = i != options.items.end() ?
                    i->second :
                    models::HUDPos::None;
                const bool visible =
                    pos != models::HUDPos::None &&
                    (file || models::HUDItem::FileName == item);
                // Taken out first, so that an item comes back at the end of
                // its corner, in order, rather than where it was.
                p.hudWidgets[item]->setParent(nullptr);
                if (visible)
                {
                    p.hudWidgets[item]->setParent(p.hudLayouts[pos]);
                }
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
