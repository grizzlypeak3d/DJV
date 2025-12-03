// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/SettingsToolPrivate.h>

#include <djvApp/App.h>

#include <ftk/UI/GroupBox.h>
#include <ftk/UI/DrawUtil.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ToolButton.h>

namespace djv
{
    namespace app
    {
        struct ShortcutEdit::Private
        {
            Shortcut shortcut;
            bool collision = false;

            std::shared_ptr<ftk::Label> label;

            std::function<void(const Shortcut&)> callback;

            struct SizeData
            {
                std::optional<float> displayScale;
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

        void ShortcutEdit::setShortcut(const Shortcut& value)
        {
            FTK_P();
            if (value == p.shortcut)
                return;
            p.shortcut = value;
            _widgetUpdate();
        }

        void ShortcutEdit::setCallback(const std::function<void(const Shortcut&)>& value)
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
                p.size.border = event.style->getSizeRole(ftk::SizeRole::Border, event.displayScale);
                p.draw.reset();
            }

            setSizeHint(_p->label->getSizeHint() + p.size.border * 2);
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
            case ftk::Key::Tab: break;
            case ftk::Key::CapsLock: break;
            case ftk::Key::ScrollLock: break;
            case ftk::Key::NumLock: break;
            default:
                if (hasKeyFocus())
                {
                    event.accept = true;
                    p.shortcut.key = event.key;
                    p.shortcut.modifiers = event.modifiers;
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
            p.label->setText(ftk::getShortcutLabel(p.shortcut.key, p.shortcut.modifiers));
        }

        struct ShortcutWidget::Private
        {
            Shortcut shortcut;
            std::shared_ptr<ShortcutEdit> edit;
            std::shared_ptr<ftk::ToolButton> clearButton;
            std::shared_ptr<ftk::HorizontalLayout> layout;
            std::function<void(const Shortcut&)> callback;
        };

        void ShortcutWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(context, "djv::app::ShortcutWidget", parent);
            FTK_P();

            setHStretch(ftk::Stretch::Expanding);

            p.edit = ShortcutEdit::create(context);

            p.clearButton = ftk::ToolButton::create(context);
            p.clearButton->setIcon("Clear");

            p.layout = ftk::HorizontalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::SpacingTool);
            p.edit->setParent(p.layout);
            p.clearButton->setParent(p.layout);

            p.clearButton->setClickedCallback(
                [this]
                {
                    FTK_P();
                    p.shortcut.key = ftk::Key::Unknown;
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

        void ShortcutWidget::setShortcut(const Shortcut& value)
        {
            FTK_P();
            p.shortcut = value;
            p.edit->setShortcut(value);
        }

        void ShortcutWidget::setCallback(const std::function<void(const Shortcut&)>& value)
        {
            FTK_P();
            p.callback = value;
            p.edit->setCallback(value);
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

            std::map<std::string, std::shared_ptr<ShortcutWidget> > widgets;
            std::vector<std::shared_ptr<ftk::GroupBox> > groupBoxes;
            std::shared_ptr<ftk::VerticalLayout> layout;

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

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);

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
            std::map<std::pair<ftk::Key, int>, std::vector<std::string> > collisions;
            for (const auto& shortcut : settings.shortcuts)
            {
                if (shortcut.key != ftk::Key::Unknown)
                {
                    collisions[std::make_pair(shortcut.key, shortcut.modifiers)].push_back(shortcut.name);
                }
            }

            if (groups != p.groups)
            {
                p.groups = groups;

                // Delete the old widgets.
                p.widgets.clear();
                for (auto groupBox : p.groupBoxes)
                {
                    groupBox->setParent(nullptr);
                }
                p.groupBoxes.clear();

                // Create the new widgets.
                if (auto context = getContext())
                {
                    for (const auto& group : p.groups)
                    {
                        auto groupBox = ftk::GroupBox::create(context, group.name, p.layout);
                        p.groupBoxes.push_back(groupBox);
                        auto formLayout = ftk::FormLayout::create(context, groupBox);
                        formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
                        for (const auto& shortcut : group.shortcuts)
                        {
                            auto widget = ShortcutWidget::create(context);
                            widget->setShortcut(shortcut);
                            p.widgets[shortcut.name] = widget;
                            formLayout->addRow(shortcut.text + ":", widget);
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
                        bool collision = false;
                        const auto k = collisions.find(std::make_pair(j->key, j->modifiers));
                        if (k != collisions.end())
                        {
                            collision = k->second.size() > 1;
                        }
                        i->second->setCollision(collision);
                    }
                }
            }
        }
    }
}
