// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/SettingsToolPrivate.h>

#include <djvApp/App.h>

#include <ftk/UI/DrawUtil.h>
#include <ftk/UI/GridLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/Spacer.h>
#include <ftk/UI/ToolButton.h>

namespace djv
{
    namespace app
    {
        struct ShortcutEdit::Private
        {
            ftk::KeyShortcut shortcut;
            bool collision = false;

            std::shared_ptr<ftk::Label> label;

            std::function<void(const ftk::KeyShortcut&)> callback;

            struct SizeData
            {
                std::optional<float> displayScale;
                int minSize = 0;
                int border = 0;
            };
            SizeData size;

            struct DrawData
            {
                ftk::Box2I g;
                ftk::Box2I g2;
                ftk::TriMesh2F border;
            };
            std::optional<DrawData> draw;
        };

        void ShortcutEdit::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            IMouseWidget::_init(context, "djv::app::ShortcutEdit", parent);
            FTK_P();
            
            setHStretch(ftk::Stretch::Expanding);
            setAcceptsKeyFocus(true);
            _setMouseHoverEnabled(true);
            _setMousePressEnabled(true);

            p.label = ftk::Label::create(context, shared_from_this());
            p.label->setMarginRole(ftk::SizeRole::MarginInside);

            _widgetUpdate();
        }

        ShortcutEdit::ShortcutEdit() :
            _p(new Private)
        {}

        ShortcutEdit::~ShortcutEdit()
        {}

        std::shared_ptr<ShortcutEdit> ShortcutEdit::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ShortcutEdit>(new ShortcutEdit);
            out->_init(context, parent);
            return out;
        }

        void ShortcutEdit::setShortcut(const ftk::KeyShortcut& value)
        {
            FTK_P();
            if (value == p.shortcut)
                return;
            p.shortcut = value;
            _widgetUpdate();
        }

        void ShortcutEdit::setCallback(const std::function<void(const ftk::KeyShortcut&)>& value)
        {
            _p->callback = value;
        }

        void ShortcutEdit::setCollision(bool value)
        {
            FTK_P();
            if (value == p.collision)
                return;
            p.collision = value;
            _widgetUpdate();
        }

        void ShortcutEdit::setGeometry(const ftk::Box2I& value)
        {
            bool changed = value != getGeometry();
            IMouseWidget::setGeometry(value);
            FTK_P();

            const ftk::Box2I g = ftk::margin(value, -p.size.border);
            p.label->setGeometry(g);

            if (changed)
            {
                p.draw.reset();
            }
        }

        ftk::Box2I ShortcutEdit::getChildrenClipRect() const
        {
            return ftk::margin(getGeometry(), -_p->size.border);
        }

        void ShortcutEdit::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            FTK_P();

            if (!p.size.displayScale.has_value() ||
                (p.size.displayScale.has_value() && p.size.displayScale.value() != event.displayScale))
            {
                p.size.displayScale = event.displayScale;
                p.size.minSize = event.style->getSizeRole(ftk::SizeRole::ScrollAreaSmall, event.displayScale);
                p.size.border = event.style->getSizeRole(ftk::SizeRole::Border, event.displayScale);
                p.draw.reset();
            }

            ftk::Size2I sizeHint;
            sizeHint.w = std::max(_p->label->getSizeHint().w, p.size.minSize);
            sizeHint.h = _p->label->getSizeHint().h;
            setSizeHint(sizeHint + p.size.border * 2);
        }

        void ShortcutEdit::drawEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent& event)
        {
            IMouseWidget::drawEvent(drawRect, event);
            FTK_P();

            if (!p.draw.has_value())
            {
                p.draw = Private::DrawData();
                p.draw->g = getGeometry();
                p.draw->g2 = ftk::margin(p.draw->g, -p.size.border);
                p.draw->border = ftk::border(p.draw->g, p.size.border);
            }

            event.render->drawMesh(
                p.draw->border,
                event.style->getColorRole(hasKeyFocus() ? ftk::ColorRole::KeyFocus : ftk::ColorRole::Border));

            event.render->drawRect(
                p.draw->g2,
                event.style->getColorRole(p.collision ? ftk::ColorRole::Red : ftk::ColorRole::Base));

            if (_isMouseInside())
            {
                event.render->drawRect(
                    p.draw->g,
                    event.style->getColorRole(ftk::ColorRole::Hover));
            }
        }

        void ShortcutEdit::mouseEnterEvent(ftk::MouseEnterEvent& event)
        {
            IMouseWidget::mouseEnterEvent(event);
            setDrawUpdate();
        }

        void ShortcutEdit::mouseLeaveEvent()
        {
            IMouseWidget::mouseLeaveEvent();
            setDrawUpdate();
        }

        void ShortcutEdit::mousePressEvent(ftk::MouseClickEvent& event)
        {
            IMouseWidget::mousePressEvent(event);
            takeKeyFocus();
            setDrawUpdate();
        }

        void ShortcutEdit::keyFocusEvent(bool value)
        {
            IMouseWidget::keyFocusEvent(value);
            setDrawUpdate();
        }

        void ShortcutEdit::keyPressEvent(ftk::KeyEvent& event)
        {
            IMouseWidget::keyPressEvent(event);
            FTK_P();
            switch (event.key)
            {
            case ftk::Key::Unknown: break;
            case ftk::Key::Escape:
                event.accept = true;
                releaseKeyFocus();
                break;
            case ftk::Key::Return: break;
            case ftk::Key::CapsLock: break;
            case ftk::Key::ScrollLock: break;
            case ftk::Key::NumLock: break;
            default:
                if (hasKeyFocus())
                {
                    event.accept = true;
                    p.shortcut = ftk::KeyShortcut(event.key, event.modifiers);
                    if (p.callback)
                    {
                        p.callback(p.shortcut);
                    }
                    _widgetUpdate();
                }
                break;
            }
        }

        void ShortcutEdit::keyReleaseEvent(ftk::KeyEvent& event)
        {
            IMouseWidget::keyReleaseEvent(event);
            event.accept = true;
        }

        void ShortcutEdit::_widgetUpdate()
        {
            FTK_P();
            p.label->setText(ftk::getShortcutLabel(
                p.shortcut.key,
                p.shortcut.modifiers));
        }

        struct ShortcutWidget::Private
        {
            Shortcut shortcut;
            std::shared_ptr<ShortcutEdit> primaryEdit;
            std::shared_ptr<ftk::ToolButton> primaryClearButton;
            std::shared_ptr<ShortcutEdit> secondaryEdit;
            std::shared_ptr<ftk::ToolButton> secondaryClearButton;
            std::shared_ptr<ftk::HorizontalLayout> layout;
            std::function<void(const Shortcut&)> callback;
        };

        void ShortcutWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(context, "djv::app::ShortcutWidget", parent);
            FTK_P();

            p.primaryEdit = ShortcutEdit::create(context);
            p.primaryEdit->setTooltip("Primary shortcut");

            p.primaryClearButton = ftk::ToolButton::create(context);
            p.primaryClearButton->setIcon("ClearSmall");
            p.primaryClearButton->setTooltip("Clear the primary shortcut");

            p.secondaryEdit = ShortcutEdit::create(context);
            p.secondaryEdit->setTooltip("Secondary shortcut");

            p.secondaryClearButton = ftk::ToolButton::create(context);
            p.secondaryClearButton->setIcon("ClearSmall");
            p.secondaryClearButton->setTooltip("Clear the secondary shortcut");

            p.layout = ftk::HorizontalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::SpacingTool);
            p.primaryEdit->setParent(p.layout);
            p.primaryClearButton->setParent(p.layout);
            p.secondaryEdit->setParent(p.layout);
            p.secondaryClearButton->setParent(p.layout);

            p.primaryEdit->setCallback(
                [this](const ftk::KeyShortcut& value)
                {
                    FTK_P();
                    p.shortcut.primary = value;
                    if (p.callback)
                    {
                        p.callback(p.shortcut);
                    }
                });

            p.primaryClearButton->setClickedCallback(
                [this]
                {
                    FTK_P();
                    p.shortcut.primary = ftk::KeyShortcut();
                    p.primaryEdit->setShortcut(p.shortcut.primary);
                    if (p.callback)
                    {
                        p.callback(p.shortcut);
                    }
                });

            p.secondaryEdit->setCallback(
                [this](const ftk::KeyShortcut& value)
                {
                    FTK_P();
                    p.shortcut.secondary = value;
                    if (p.callback)
                    {
                        p.callback(p.shortcut);
                    }
                });

            p.secondaryClearButton->setClickedCallback(
                [this]
                {
                    FTK_P();
                    p.shortcut.secondary = ftk::KeyShortcut();
                    p.secondaryEdit->setShortcut(p.shortcut.secondary);
                    if (p.callback)
                    {
                        p.callback(p.shortcut);
                    }
                });
        }

        ShortcutWidget::ShortcutWidget() :
            _p(new Private)
        {}

        ShortcutWidget::~ShortcutWidget()
        {}

        std::shared_ptr<ShortcutWidget> ShortcutWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ShortcutWidget>(new ShortcutWidget);
            out->_init(context, parent);
            return out;
        }

        void ShortcutWidget::setShortcut(const Shortcut& value)
        {
            FTK_P();
            if (value == p.shortcut)
                return;
            p.shortcut = value;
            p.primaryEdit->setShortcut(value.primary);
            p.secondaryEdit->setShortcut(value.secondary);
        }

        void ShortcutWidget::setCallback(const std::function<void(const Shortcut&)>& value)
        {
            FTK_P();
            p.callback = value;
        }

        void ShortcutWidget::setCollision(bool primary, bool secondary)
        {
            _p->primaryEdit->setCollision(primary);
            _p->secondaryEdit->setCollision(secondary);
        }

        void ShortcutWidget::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void ShortcutWidget::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            setSizeHint(_p->layout->getSizeHint());
        }

        struct ShortcutsSettingsWidget::Private
        {
            std::shared_ptr<SettingsModel> model;
            struct Group
            {
                std::string name;
                std::vector<Shortcut> shortcuts;

                bool operator == (const Group& other) const
                {
                    bool out =
                        name == other.name &&
                        shortcuts.size() == other.shortcuts.size();
                    for (size_t i = 0; out && i < shortcuts.size(); ++i)
                    {
                        out &= shortcuts[i].name == other.shortcuts[i].name;
                    }
                    return out;
                }

                bool operator != (const Group& other) const
                {
                    return !(*this == other);
                }
            };
            std::vector<Group> groups;

            std::map<std::string, std::shared_ptr<ftk::Label> > groupLabels;
            std::map<std::string, std::shared_ptr<ftk::Spacer> > groupSpacers;
            std::map<std::string, std::shared_ptr<ShortcutWidget> > widgets;
            std::shared_ptr<ftk::GridLayout> layout;

            std::shared_ptr<ftk::Observer<ShortcutsSettings> > settingsObserver;
        };

        void ShortcutsSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::app::ShortcutsSettingsWidget", parent);
            FTK_P();

            p.model = app->getSettingsModel();

            p.layout = ftk::GridLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.settingsObserver = ftk::Observer<ShortcutsSettings>::create(
                p.model->observeShortcuts(),
                [this](const ShortcutsSettings& value)
                {
                    _widgetUpdate(value);
                });
        }

        ShortcutsSettingsWidget::ShortcutsSettingsWidget() :
            _p(new Private)
        {}

        ShortcutsSettingsWidget::~ShortcutsSettingsWidget()
        {}

        std::shared_ptr<ShortcutsSettingsWidget> ShortcutsSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ShortcutsSettingsWidget>(new ShortcutsSettingsWidget);
            out->_init(context, app, parent);
            return out;
        }

        void ShortcutsSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void ShortcutsSettingsWidget::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            ISettingsWidget::sizeHintEvent(event);
            setSizeHint(_p->layout->getSizeHint());
        }

        void ShortcutsSettingsWidget::_widgetUpdate(const ShortcutsSettings& settings)
        {
            FTK_P();

            // Create groups of shortcuts.
            std::vector<Private::Group> groups;
            for (const auto& shortcut : settings.shortcuts)
            {
                const auto s = ftk::split(shortcut.name, '/');
                if ((!s.empty() && !groups.empty() && s.front() != groups.back().name) ||
                    (!s.empty() && groups.empty()))
                {
                    Private::Group group;
                    group.name = s.front();
                    groups.push_back(group);
                }
                if (s.size() > 1 && !groups.empty())
                {
                    groups.back().shortcuts.push_back(shortcut);
                }
            }

            // Find collisions.
            std::map<std::string, int> collisions;
            for (const auto& i : settings.shortcuts)
            {
                if (i.primary.key != ftk::Key::Unknown)
                {
                    collisions[to_string(i.primary)]++;
                }
                if (i.secondary.key != ftk::Key::Unknown)
                {
                    collisions[to_string(i.secondary)]++;
                }
            }

            if (groups != p.groups)
            {
                p.groups = groups;

                // Delete the old widgets.
                p.groupLabels.clear();
                p.groupSpacers.clear();
                p.widgets.clear();
                p.layout->clear();

                // Create the new widgets.
                if (auto context = getContext())
                {
                    int column = 0;
                    for (int i = 0; i < p.groups.size(); ++i)
                    {
                        const auto& group = p.groups[i];

                        auto groupLabel = ftk::Label::create(context, ftk::toUpper(group.name), p.layout);
                        groupLabel->setMarginRole(ftk::SizeRole::MarginInside);
                        ftk::FontInfo fontInfo;
                        fontInfo.family = ftk::getFont(ftk::Font::Bold);
                        groupLabel->setFontInfo(fontInfo);
                        p.groupLabels[group.name] = groupLabel;
                        p.layout->setGridPos(groupLabel, column, 0);
                        ++column;

                        for (const auto& shortcut : group.shortcuts)
                        {
                            auto label = ftk::Label::create(context, shortcut.text + ":", p.layout);
                            label->setMarginRole(ftk::SizeRole::MarginInside);
                            label->setHStretch(ftk::Stretch::Expanding);
                            p.layout->setGridPos(label, column, 0);

                            auto widget = ShortcutWidget::create(context, p.layout);
                            widget->setShortcut(shortcut);
                            widget->setTooltip("Primary shorcut");
                            p.widgets[shortcut.name] = widget;
                            p.layout->setGridPos(widget, column, 1);
                            widget->setCallback(
                                [this](const Shortcut& value)
                                {
                                    FTK_P();
                                    auto settings = p.model->getShortcuts();
                                    const auto shortcut = value;
                                    const auto i = std::find_if(
                                        settings.shortcuts.begin(),
                                        settings.shortcuts.end(),
                                        [shortcut](const Shortcut& other)
                                        {
                                            return shortcut.name == other.name;
                                        });
                                    if (i != settings.shortcuts.end())
                                    {
                                        *i = value;
                                        p.model->setShortcuts(settings);
                                    }
                                });

                            ++column;
                        }

                        if (i < p.groups.size() - 1)
                        {
                            auto spacer = ftk::Spacer::create(context, ftk::Orientation::Vertical, p.layout);
                            p.groupSpacers[group.name] = spacer;
                            p.layout->setGridPos(spacer, column, 0);
                            ++column;
                        }
                    }
                }
            }

            // Update the values.
            for (const auto& group : p.groups)
            {
                for (const auto& shortcut : group.shortcuts)
                {
                    const auto i = p.widgets.find(shortcut.name);
                    const auto j = std::find_if(
                        settings.shortcuts.begin(),
                        settings.shortcuts.end(),
                        [shortcut](const Shortcut& value)
                        {
                            return shortcut.name == value.name;
                        });
                    if (i != p.widgets.end() && j != settings.shortcuts.end())
                    {
                        i->second->setShortcut(*j);
                        bool primaryCollision = false;
                        bool secondaryCollision = false;
                        auto k = collisions.find(to_string(j->primary));
                        if (k != collisions.end())
                        {
                            primaryCollision = k->second > 1;
                        }
                        k = collisions.find(to_string(j->secondary));
                        if (k != collisions.end())
                        {
                            secondaryCollision = k->second > 1;
                        }
                        i->second->setCollision(primaryCollision, secondaryCollision);
                    }
                }
            }
        }
    }
}
