// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/StatusIndicator.h>

#include <djv/Models/AudioModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/Status.h>
#include <djv/Models/ToolsModel.h>
#include <djv/Models/ViewportModel.h>

#include <djv/UI/StatusIndicatorPopup.h>

#include <ftk/UI/DrawUtil.h>
#include <ftk/UI/IButton.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/Core/Context.h>
#include <ftk/Core/String.h>

#include <optional>

namespace djv
{
    namespace ui
    {
        namespace
        {
            //! A letter for each group of options, lit while one of them is
            //! on and dim otherwise, in places that never change, so the
            //! pattern is known at a glance; and the arrow of a button that
            //! opens a popup. Checked while any is lit, for what watches it,
            //! but not tinted: the letters are the tint.
            class LettersButton : public ftk::IButton
            {
            protected:
                void _init(const std::shared_ptr<ftk::Context>& context)
                {
                    IButton::_init(context, "djv::ui::StatusIndicator::LettersButton", nullptr);
                    setAcceptsKeyFocus(true);
                    _buttonRole = ftk::ColorRole::None;
                }

            public:
                static std::shared_ptr<LettersButton> create(
                    const std::shared_ptr<ftk::Context>& context)
                {
                    auto out = std::shared_ptr<LettersButton>(new LettersButton);
                    out->_init(context);
                    return out;
                }

                void setLetters(
                    const std::vector<std::string>& letters,
                    const std::vector<bool>& lit)
                {
                    if (letters == _letters && lit == _lit)
                        return;
                    if (letters != _letters)
                    {
                        _letters = letters;
                        _size.reset();
                        setSizeUpdate();
                    }
                    _lit = lit;
                    _glyphs.clear();
                    setDrawUpdate();
                }

                ftk::Size2I getSizeHint() const override
                {
                    return _size.has_value() ? _size->sizeHint : ftk::Size2I();
                }

                void setGeometry(const ftk::Box2I& value) override
                {
                    if (value != getGeometry())
                    {
                        _mesh.reset();
                    }
                    IButton::setGeometry(value);
                }

                void styleEvent(const ftk::StyleEvent& event) override
                {
                    IButton::styleEvent(event);
                    if (event.hasChanges())
                    {
                        _size.reset();
                        _mesh.reset();
                        _glyphs.clear();
                    }
                }

                void sizeHintEvent(const ftk::SizeHintEvent& event) override
                {
                    IButton::sizeHintEvent(event);
                    if (_size.has_value() && _size->displayScale == event.displayScale)
                        return;
                    SizeData size;
                    size.displayScale = event.displayScale;
                    size.margin = event.style->getSizeRole(ftk::SizeRole::MarginInside, event.displayScale);
                    size.keyFocus = event.style->getSizeRole(ftk::SizeRole::KeyFocus, event.displayScale);
                    size.pad = event.style->getSizeRole(ftk::SizeRole::LabelPad, event.displayScale);
                    size.cornerRadius = event.style->getSizeRole(ftk::SizeRole::CornerRadius, event.displayScale);
                    size.fontInfo = event.style->getFont(ftk::FontType::Bold, event.displayScale);
                    size.fontMetrics = event.fontSystem->getMetrics(size.fontInfo);
                    // One width for every letter, the widest, so each keeps
                    // its place whichever are lit.
                    for (const auto& letter : _letters)
                    {
                        const ftk::Size2I s = event.fontSystem->getSize(letter, size.fontInfo);
                        size.letterSize.w = std::max(size.letterSize.w, s.w);
                        size.letterSize.h = std::max(size.letterSize.h, s.h);
                    }
                    size.slot = size.letterSize.w + size.pad / 2;
                    size.arrow = event.iconSystem->get("MenuArrow", event.displayScale);
                    size.sizeHint.w = size.slot * _letters.size() + size.pad;
                    size.sizeHint.h = size.fontMetrics.lineHeight;
                    if (size.arrow)
                    {
                        size.sizeHint.w += size.arrow->getWidth();
                        size.sizeHint.h = std::max(size.sizeHint.h, size.arrow->getHeight());
                    }
                    size.sizeHint = ftk::margin(size.sizeHint, size.margin + size.keyFocus);
                    _size = size;
                    _mesh.reset();
                    _glyphs.clear();
                }

                void drawEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent& event) override
                {
                    IButton::drawEvent(drawRect, event);
                    if (!_size.has_value())
                        return;
                    const ftk::Box2I& g = getGeometry();
                    if (!_mesh.has_value())
                    {
                        _mesh = ftk::rect(g, _size->cornerRadius);
                        _keyFocusMesh = ftk::border(g, _size->keyFocus, _size->cornerRadius);
                    }
                    if (_isMousePressed())
                    {
                        event.render->drawMesh(*_mesh, event.style->getColorRole(ftk::ColorRole::Pressed));
                    }
                    else if (_isMouseInside())
                    {
                        event.render->drawMesh(*_mesh, event.style->getColorRole(ftk::ColorRole::Hover));
                    }
                    if (showKeyFocus())
                    {
                        event.render->drawMesh(_keyFocusMesh, event.style->getColorRole(ftk::ColorRole::KeyFocus));
                    }

                    if (_glyphs.size() != _letters.size())
                    {
                        _glyphs.clear();
                        for (const auto& letter : _letters)
                        {
                            _glyphs.push_back(event.fontSystem->getGlyphs(letter, _size->fontInfo));
                        }
                    }
                    const ftk::Box2I inside = ftk::margin(g, -(_size->margin + _size->keyFocus));
                    const ftk::Color4F litColor = ftk::checkedHighlight(
                        event.style->getColorRole(ftk::ColorRole::Checked, isEnabled()));
                    const ftk::Color4F dimColor = event.style->getColorRole(ftk::ColorRole::TextDisabled);
                    int x = inside.x() + _size->pad / 2;
                    for (size_t i = 0; i < _letters.size(); ++i)
                    {
                        const ftk::Size2I s = event.fontSystem->getSize(_letters[i], _size->fontInfo);
                        const bool lit = i < _lit.size() && _lit[i];
                        event.render->drawText(
                            _glyphs[i],
                            _size->fontMetrics,
                            ftk::V2I(
                                x + _size->slot / 2 - s.w / 2,
                                inside.y() + inside.h() / 2 - s.h / 2),
                            lit ? litColor : dimColor);
                        x += _size->slot;
                    }
                    x += _size->pad / 2;
                    if (_size->arrow)
                    {
                        const ftk::Size2I& s = _size->arrow->getSize();
                        event.render->drawImage(
                            _size->arrow,
                            ftk::Box2I(x, inside.y() + inside.h() / 2 - s.h / 2, s.w, s.h),
                            event.style->getColorRole(ftk::ColorRole::Arrow, isEnabled()));
                    }
                }

                void keyPressEvent(ftk::KeyEvent& event) override
                {
                    if (0 == event.modifiers && ftk::Key::Return == event.key)
                    {
                        event.accept = true;
                        if (_pressedCallback)
                        {
                            _pressedCallback();
                        }
                    }
                    else if (0 == event.modifiers && ftk::Key::Escape == event.key &&
                        hasKeyFocus() && showKeyFocus())
                    {
                        event.accept = true;
                        releaseKeyFocus();
                    }
                    if (!event.accept)
                    {
                        IButton::keyPressEvent(event);
                    }
                }

                void keyReleaseEvent(ftk::KeyEvent& event) override
                {
                    IButton::keyReleaseEvent(event);
                    event.accept = true;
                }

            private:
                std::vector<std::string> _letters;
                std::vector<bool> _lit;

                struct SizeData
                {
                    float displayScale = 0.F;
                    int margin = 0;
                    int keyFocus = 0;
                    int pad = 0;
                    int cornerRadius = 0;
                    ftk::FontInfo fontInfo;
                    ftk::FontMetrics fontMetrics;
                    ftk::Size2I letterSize;
                    int slot = 0;
                    std::shared_ptr<ftk::Image> arrow;
                    ftk::Size2I sizeHint;
                };
                std::optional<SizeData> _size;
                std::optional<ftk::TriMesh2F> _mesh;
                ftk::TriMesh2F _keyFocusMesh;
                std::vector<std::vector<std::shared_ptr<ftk::Glyph> > > _glyphs;
            };
        }

        struct StatusIndicator::Private
        {
            // The options watched, which the models say what is on from;
            // see models::getStatus().
            models::StatusOptions options;
            // Which frame is stored, for the tooltip.
            std::string storedFrameLabel;

            std::weak_ptr<models::ViewportModel> viewportModel;
            std::weak_ptr<models::ToolsModel> toolsModel;

            std::shared_ptr<LettersButton> button;
            std::shared_ptr<StatusIndicatorPopup> popup;

            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > fgOptionsObserver;
            std::shared_ptr<ftk::Observer<models::AspectRatioOptions> > aspectRatioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<double> > audioSyncOffsetObserver;
            std::shared_ptr<ftk::Observer<int> > audioChannelObserver;
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<ftk::ListObserver<int> > bIndexesObserver;
            std::shared_ptr<ftk::Observer<std::optional<models::StoredFrame> > > frameStoreObserver;
        };

        void StatusIndicator::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::AudioModel>& audioModel,
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ToolsModel>& toolsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(
                context,
                "djv::ui::StatusIndicator",
                parent);
            FTK_P();

            p.viewportModel = viewportModel;
            p.toolsModel = toolsModel;

            p.button = LettersButton::create(context);
            _setWidget(p.button);
            ftk::setScreenshotTag(p.button, "Status.Indicator");

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                viewportModel->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    _p->options.display = value;
                    _indicatorUpdate();
                });

            p.fgOptionsObserver = ftk::Observer<tl::ForegroundOptions>::create(
                viewportModel->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
                {
                    _p->options.foreground = value;
                    _indicatorUpdate();
                });

            p.aspectRatioOptionsObserver = ftk::Observer<models::AspectRatioOptions>::create(
                viewportModel->observeAspectRatioOptions(),
                [this](const models::AspectRatioOptions& value)
                {
                    _p->options.aspectRatio = value;
                    _indicatorUpdate();
                });

            p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                colorModel->observeOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                    _p->options.ocio = value;
                    _indicatorUpdate();
                });

            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                colorModel->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                    _p->options.lut = value;
                    _indicatorUpdate();
                });

            p.audioSyncOffsetObserver = ftk::Observer<double>::create(
                audioModel->observeSyncOffset(),
                [this](double value)
                {
                    _p->options.audioOffset = value;
                    _indicatorUpdate();
                });

            p.audioChannelObserver = ftk::Observer<int>::create(
                filesModel->observeAudioChannel(),
                [this](int value)
                {
                    _p->options.audioChannel = value;
                    _indicatorUpdate();
                });

            p.compareOptionsObserver = ftk::Observer<tl::CompareOptions>::create(
                filesModel->observeCompareOptions(),
                [this](const tl::CompareOptions& value)
                {
                    _p->options.compare = value.compare;
                    _indicatorUpdate();
                });

            p.bIndexesObserver = ftk::ListObserver<int>::create(
                filesModel->observeBIndexes(),
                [this](const std::vector<int>& value)
                {
                    _p->options.bFiles = !value.empty();
                    _indicatorUpdate();
                });

            p.frameStoreObserver = ftk::Observer<std::optional<models::StoredFrame> >::create(
                filesModel->observeFrameStore(),
                [this](const std::optional<models::StoredFrame>& value)
                {
                    _p->options.storedFrame = value.has_value();
                    _p->storedFrameLabel = value.has_value() ? value->label : std::string();
                    _indicatorUpdate();
                });

            p.button->setPressedCallback(
                [this]
                {
                    _showIndicatorPopup();
                });
        }

        StatusIndicator::StatusIndicator() :
            _p(new Private)
        {}

        StatusIndicator::~StatusIndicator()
        {}

        std::shared_ptr<StatusIndicator> StatusIndicator::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::AudioModel>& audioModel,
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ToolsModel>& toolsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<StatusIndicator>(new StatusIndicator);
            out->_init(context, viewportModel, colorModel, audioModel, filesModel, toolsModel, parent);
            return out;
        }

        bool StatusIndicator::_hasIndicator() const
        {
            for (const auto& i : models::getStatus(_p->options))
            {
                if (i.second)
                {
                    return true;
                }
            }
            return false;
        }

        std::vector<models::StatusItem> StatusIndicator::_getIndicators() const
        {
            return models::getStatusItems();
        }

        std::vector<models::StatusGroup> StatusIndicator::_getIndicatorGroups() const
        {
            return models::getStatusGroups();
        }

        std::map<std::string, bool> StatusIndicator::_getIndicatorValues() const
        {
            return models::getStatus(_p->options);
        }

        std::string StatusIndicator::_getIndicatorTool(const std::string& name) const
        {
            std::string out;
            if ("Channels" == name ||
                "Negative" == name ||
                "ClippingWarning" == name ||
                "Mirror" == name ||
                "AspectRatio" == name)
            {
                out = "View";
            }
            else if (
                "OCIO" == name ||
                "LUT" == name ||
                "Color" == name)
            {
                out = "Color";
            }
            else if (
                "Compare" == name ||
                "FrameStore" == name)
            {
                // The stored frame's name and the button to let it go are
                // with the other compare options.
                out = "Files";
            }
            else if (
                "AudioOffset" == name ||
                "AudioChannel" == name)
            {
                out = "Audio";
            }
            return out;
        }

        std::string StatusIndicator::_getIndicatorSection(const std::string& name) const
        {
            FTK_P();
            std::string out;
            if ("Channels" == name ||
                "Negative" == name ||
                "Mirror" == name)
            {
                out = "Options";
            }
            else if (
                "Compare" == name ||
                "FrameStore" == name)
            {
                out = "Compare";
            }
            else if (
                "ClippingWarning" == name ||
                "AspectRatio" == name ||
                "OCIO" == name ||
                "LUT" == name)
            {
                out = name;
            }
            else if ("Color" == name)
            {
                // Levels has a section of its own; everything else the
                // indicator covers is in the Color section.
                out = "Color";
                if (auto viewportModel = p.viewportModel.lock())
                {
                    const tl::DisplayOptions& options = viewportModel->getDisplayOptions();
                    tl::Color color = options.color;
                    color.enabled = false;
                    tl::Levels levels = options.levels;
                    levels.enabled = false;
                    if (options.levels.enabled && levels != tl::Levels() &&
                        !(options.color.enabled && color != tl::Color()) &&
                        !(options.exposure.enabled && options.exposure.exposure != 0.F) &&
                        !(options.softClip.enabled && options.softClip.value > 0.F))
                    {
                        out = "Levels";
                    }
                }
            }
            return out;
        }

        void StatusIndicator::_indicatorUpdate()
        {
            FTK_P();
            // Checked while anything is on, for what watches it: the button
            // is not checkable, so clicking it still only opens the popup.
            p.button->setChecked(_hasIndicator());
            const auto items = _getIndicators();
            const auto groups = _getIndicatorGroups();
            const auto values = _getIndicatorValues();
            std::vector<std::string> letters;
            std::vector<bool> lit;
            std::string tooltip;
            for (const auto& group : groups)
            {
                std::vector<std::string> on;
                for (const auto& item : items)
                {
                    const auto i = values.find(item.id);
                    if (item.group == group.id && i != values.end() && i->second)
                    {
                        // Compare and the frame store are each the only
                        // item of their group, with its name, so what is
                        // on is how the files are compared and which frame
                        // is stored (DJV #969).
                        std::string text = item.label;
                        if ("Compare" == item.id)
                        {
                            text = tl::getLabel(p.options.compare);
                        }
                        else if ("FrameStore" == item.id && !p.storedFrameLabel.empty())
                        {
                            text = p.storedFrameLabel;
                        }
                        on.push_back(text);
                    }
                }
                letters.push_back(group.letter);
                lit.push_back(!on.empty());
                tooltip += group.letter + "  " + group.label;
                if (!on.empty())
                {
                    tooltip += ": " + ftk::join(on, ", ");
                }
                tooltip += "\n";
            }
            tooltip += "\nClick to show the options and go to their controls.";
            p.button->setLetters(letters, lit);
            p.button->setTooltip(tooltip);
            if (p.popup)
            {
                p.popup->setIndicators(values);
            }
        }

        void StatusIndicator::_showIndicatorPopup()
        {
            FTK_P();
            if (!p.popup)
            {
                std::weak_ptr<StatusIndicator> weak(std::dynamic_pointer_cast<StatusIndicator>(shared_from_this()));
                p.popup = StatusIndicatorPopup::create(
                    getContext(),
                    _getIndicators(),
                    _getIndicatorGroups());
                p.popup->setIndicators(_getIndicatorValues());
                p.popup->setCallback(
                    [weak](const std::string& name)
                    {
                        if (auto widget = weak.lock())
                        {
                            const std::string tool = widget->_getIndicatorTool(name);
                            auto toolsModel = widget->_p->toolsModel.lock();
                            if (!tool.empty() && toolsModel)
                            {
                                toolsModel->showSection(
                                    tool,
                                    widget->_getIndicatorSection(name));
                            }
                            // Last: closing lets go of the popup this
                            // callback belongs to.
                            if (widget->_p->popup)
                            {
                                widget->_p->popup->close();
                            }
                        }
                    });
                p.popup->open(getWindow(), p.button->getGeometry());
                p.popup->setCloseCallback(
                    [weak]
                    {
                        if (auto widget = weak.lock())
                        {
                            widget->_p->popup.reset();
                        }
                    });
            }
            else
            {
                p.popup->close();
                p.popup.reset();
            }
        }
    }
}
