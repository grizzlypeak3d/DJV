// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/SettingsToolPrivate.h>

#include <djvApp/App.h>

#include <ftk/UI/DrawUtil.h>
#include <ftk/UI/GridLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/SearchBox.h>
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
                int keyFocus = 0;
            };
            SizeData size;

            struct DrawData
            {
                ftk::Box2I g;
                ftk::Box2I g2;
                ftk::TriMesh2F border;
                ftk::TriMesh2F keyFocus;
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
            p.label->setMarginRole(ftk::SizeRole::MarginSmall, ftk::SizeRole::MarginInside);

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

            const ftk::Box2I g = ftk::margin(value, -p.size.keyFocus);
            p.label->setGeometry(g);

            if (changed)
            {
                p.draw.reset();
            }
        }

        ftk::Box2I ShortcutEdit::getChildrenClipRect() const
        {
            return ftk::margin(getGeometry(), -_p->size.keyFocus);
        }

        void ShortcutEdit::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            FTK_P();

            if (!p.size.displayScale.has_value() ||
                (p.size.displayScale.has_value() && p.size.displayScale.value() != event.displayScale))
            {
                p.size.displayScale = event.displayScale;
                p.size.minSize = event.style->getSizeRole(ftk::SizeRole::Icon, event.displayScale);
                p.size.border = event.style->getSizeRole(ftk::SizeRole::Border, event.displayScale);
                p.size.keyFocus = event.style->getSizeRole(ftk::SizeRole::KeyFocus, event.displayScale);
                p.draw.reset();
            }

            ftk::Size2I sizeHint;
            sizeHint.w = std::max(_p->label->getSizeHint().w, p.size.minSize);
            sizeHint.h = _p->label->getSizeHint().h;
            setSizeHint(sizeHint + p.size.keyFocus * 2);
        }

        void ShortcutEdit::drawEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent& event)
        {
            IMouseWidget::drawEvent(drawRect, event);
            FTK_P();

            if (!p.draw.has_value())
            {
                p.draw = Private::DrawData();
                p.draw->g = getGeometry();
                p.draw->g2 = ftk::margin(p.draw->g, -p.size.keyFocus);
                p.draw->border = ftk::border(margin(p.draw->g2, p.size.border), p.size.border);
                p.draw->keyFocus = ftk::border(p.draw->g, p.size.keyFocus);
            }

            const bool keyFocus = hasKeyFocus();
            event.render->drawMesh(
                keyFocus ? p.draw->keyFocus : p.draw->border,
                event.style->getColorRole(keyFocus ? ftk::ColorRole::KeyFocus : ftk::ColorRole::Border));

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
            ftk::KeyShortcut shortcut;
            std::shared_ptr<ShortcutEdit> edit;
            std::shared_ptr<ftk::ToolButton> clearButton;
            std::shared_ptr<ftk::HorizontalLayout> layout;
            std::function<void(const ftk::KeyShortcut&)> callback;
        };

        void ShortcutWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(context, "djv::app::ShortcutWidget", parent);
            FTK_P();

            p.edit = ShortcutEdit::create(context);

            p.clearButton = ftk::ToolButton::create(context);
            p.clearButton->setIcon("ClearSmall");
            p.clearButton->setTooltip("Clear the shortcut");

            p.layout = ftk::HorizontalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::SpacingTool);
            p.edit->setParent(p.layout);
            p.clearButton->setParent(p.layout);

            p.edit->setCallback(
                [this](const ftk::KeyShortcut& value)
                {
                    FTK_P();
                    p.shortcut = value;
                    if (p.callback)
                    {
                        p.callback(p.shortcut);
                    }
                });

            p.clearButton->setClickedCallback(
                [this]
                {
                    FTK_P();
                    p.shortcut = ftk::KeyShortcut();
                    p.edit->setShortcut(p.shortcut);
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

        void ShortcutWidget::setShortcut(const ftk::KeyShortcut& value)
        {
            FTK_P();
            if (value == p.shortcut)
                return;
            p.shortcut = value;
            p.edit->setShortcut(value);
        }

        void ShortcutWidget::setCallback(const std::function<void(const ftk::KeyShortcut&)>& value)
        {
            FTK_P();
            p.callback = value;
        }

        void ShortcutWidget::setCollision(bool value)
        {
            _p->edit->setCollision(value);
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

            std::shared_ptr<ftk::SearchBox> searchBox;
            std::map<std::string, std::shared_ptr<ftk::Spacer> > groupSpacers;
            std::map<std::string, std::shared_ptr<ftk::Label> > groupLabels;
            std::map<std::string, std::shared_ptr<ftk::Label> > labels;
            std::map<std::string, std::shared_ptr<ShortcutWidget> > primaryWidgets;
            std::map<std::string, std::shared_ptr<ShortcutWidget> > secondaryWidgets;
            std::shared_ptr<ftk::VerticalLayout> layout;
            std::shared_ptr<ftk::GridLayout> shortcutsLayout;

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

            p.searchBox = ftk::SearchBox::create(context);
            p.searchBox->setTooltip("Search the shortcuts");

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::None);
            p.searchBox->setParent(p.layout);
            p.shortcutsLayout = ftk::GridLayout::create(context, p.layout);
            p.shortcutsLayout->setSpacingRole(ftk::SizeRole::SpacingTool);

            p.searchBox->setCallback(
                [this](const std::string& value)
                {
                    _searchUpdate(value);
                });

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
                p.groupSpacers.clear();
                p.groupLabels.clear();
                p.primaryWidgets.clear();
                p.secondaryWidgets.clear();
                p.shortcutsLayout->clear();

                // Create the new widgets.
                if (auto context = getContext())
                {
                    int column = 0;
                    for (int i = 0; i < p.groups.size(); ++i)
                    {
                        const auto& group = p.groups[i];

                        auto spacer = ftk::Spacer::create(context, ftk::Orientation::Vertical, p.shortcutsLayout);
                        spacer->setSpacingRole(ftk::SizeRole::SpacingSmall);
                        p.groupSpacers[group.name] = spacer;
                        p.shortcutsLayout->setGridPos(spacer, column, 0);
                        ++column;

                        auto groupLabel = ftk::Label::create(context, ftk::toUpper(group.name), p.shortcutsLayout);
                        groupLabel->setMarginRole(ftk::SizeRole::MarginInside);
                        ftk::FontInfo fontInfo;
                        fontInfo.family = ftk::getFont(ftk::Font::Bold);
                        groupLabel->setFontInfo(fontInfo);
                        p.groupLabels[group.name] = groupLabel;
                        p.shortcutsLayout->setGridPos(groupLabel, column, 0);
                        ++column;

                        for (const auto& shortcut : group.shortcuts)
                        {
                            auto label = ftk::Label::create(context, shortcut.text + ":", p.shortcutsLayout);
                            label->setMarginRole(ftk::SizeRole::MarginInside);
                            label->setHStretch(ftk::Stretch::Expanding);
                            p.labels[shortcut.name] = label;
                            p.shortcutsLayout->setGridPos(label, column, 0);

                            auto primaryWidget = ShortcutWidget::create(context, p.shortcutsLayout);
                            primaryWidget->setShortcut(shortcut.primary);
                            primaryWidget->setTooltip("Primary shortcut");
                            p.primaryWidgets[shortcut.name] = primaryWidget;
                            p.shortcutsLayout->setGridPos(primaryWidget, column, 1);
                            primaryWidget->setCallback(
                                [this, shortcut](const ftk::KeyShortcut& value)
                                {
                                    FTK_P();
                                    auto settings = p.model->getShortcuts();
                                    const auto i = std::find_if(
                                        settings.shortcuts.begin(),
                                        settings.shortcuts.end(),
                                        [shortcut](const Shortcut& other)
                                        {
                                            return shortcut.name == other.name;
                                        });
                                    if (i != settings.shortcuts.end())
                                    {
                                        i->primary = value;
                                        p.model->setShortcuts(settings);
                                    }
                                });

                            auto secondaryWidget = ShortcutWidget::create(context, p.shortcutsLayout);
                            secondaryWidget->setShortcut(shortcut.secondary);
                            secondaryWidget->setTooltip("Secondary shortcut");
                            p.secondaryWidgets[shortcut.name] = secondaryWidget;
                            p.shortcutsLayout->setGridPos(secondaryWidget, column, 2);
                            secondaryWidget->setCallback(
                                [this, shortcut](const ftk::KeyShortcut& value)
                                {
                                    FTK_P();
                                    auto settings = p.model->getShortcuts();
                                    const auto i = std::find_if(
                                        settings.shortcuts.begin(),
                                        settings.shortcuts.end(),
                                        [shortcut](const Shortcut& other)
                                        {
                                            return shortcut.name == other.name;
                                        });
                                    if (i != settings.shortcuts.end())
                                    {
                                        i->secondary = value;
                                        p.model->setShortcuts(settings);
                                    }
                                });

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
                    auto i = p.primaryWidgets.find(shortcut.name);
                    auto j = std::find_if(
                        settings.shortcuts.begin(),
                        settings.shortcuts.end(),
                        [shortcut](const Shortcut& value)
                        {
                            return shortcut.name == value.name;
                        });
                    if (i != p.primaryWidgets.end() && j != settings.shortcuts.end())
                    {
                        i->second->setShortcut(j->primary);
                        bool collision = false;
                        auto k = collisions.find(to_string(j->primary));
                        if (k != collisions.end())
                        {
                            collision = k->second > 1;
                        }
                        i->second->setCollision(collision);
                    }

                    i = p.secondaryWidgets.find(shortcut.name);
                    j = std::find_if(
                        settings.shortcuts.begin(),
                        settings.shortcuts.end(),
                        [shortcut](const Shortcut& value)
                        {
                            return shortcut.name == value.name;
                        });
                    if (i != p.secondaryWidgets.end() && j != settings.shortcuts.end())
                    {
                        i->second->setShortcut(j->secondary);
                        bool collision = false;
                        auto k = collisions.find(to_string(j->secondary));
                        if (k != collisions.end())
                        {
                            collision = k->second > 1;
                        }
                        i->second->setCollision(collision);
                    }
                }
            }
        }

        void ShortcutsSettingsWidget::_searchUpdate(const std::string& value)
        {
            FTK_P();

            std::map<std::string, bool> visible;
            for (const auto& i : p.primaryWidgets)
            {
                const bool v =
                    !value.empty() ?
                    ftk::contains(i.first, value, ftk::CaseCompare::Insensitive) :
                    true;
                visible[i.first] = v;
                i.second->setVisible(v);
            }

            for (const auto& i : p.secondaryWidgets)
            {
                const auto j = visible.find(i.first);
                if (j != visible.end())
                {
                    i.second->setVisible(j->second);
                }
            }

            for (const auto& i : p.labels)
            {
                const auto j = visible.find(i.first);
                if (j != visible.end())
                {
                    i.second->setVisible(j->second);
                }
            }

            for (const auto& i : p.groups)
            {
                bool v = false;
                for (const auto& j : i.shortcuts)
                {
                    const auto k = visible.find(j.name);
                    if (k != visible.end())
                    {
                        v |= k->second;
                    }
                }
                auto j = p.groupSpacers.find(i.name);
                if (j != p.groupSpacers.end())
                {
                    j->second->setVisible(v);
                }
                auto k = p.groupLabels.find(i.name);
                if (k != p.groupLabels.end())
                {
                    k->second->setVisible(v);
                }
            }
        }
    }
}
