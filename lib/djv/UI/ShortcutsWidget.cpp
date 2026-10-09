// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/SettingsWidgets.h>

#include <ftk/UI/DrawUtil.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/SearchBox.h>
#include <ftk/UI/TableWidget.h>
#include <ftk/UI/ToolButton.h>

namespace djv
{
    namespace ui
    {
        struct ShortcutEdit::Private
        {
            ftk::KeyShortcut shortcut;
            bool collision = false;

            std::shared_ptr<ftk::Label> label;
            std::shared_ptr<ftk::ToolButton> clearButton;

            std::function<void(const ftk::KeyShortcut&)> callback;

            struct SizeData
            {
                bool init = true;
                int minSize = 0;
                int border = 0;
                int keyFocus = 0;
                int pad = 0;
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
            IMouseWidget::_init(context, "djv::ui::ShortcutEdit", parent);
            FTK_P();
            
            setHStretch(ftk::Stretch::Expanding);
            setAcceptsKeyFocus(true);
            _setMouseHoverEnabled(true);
            _setMousePressEnabled(true);

            p.label = ftk::Label::create(context, shared_from_this());
            // The text is inset as the text of a table cell is, so that
            // it does not move when a cell is edited.
            p.label->setMarginRole(ftk::SizeRole::MarginInside);

            // The clear button is inside the field, so that the field is
            // the size of the table cell it is put over.
            p.clearButton = ftk::ToolButton::create(context, shared_from_this());
            p.clearButton->setIcon("ClearSmall");
            p.clearButton->setAcceptsKeyFocus(false);
            p.clearButton->setTooltip("Clear the shortcut");

            _widgetUpdate();

            p.clearButton->setClickedCallback(
                [this]
                {
                    FTK_P();
                    p.shortcut = ftk::KeyShortcut();
                    _widgetUpdate();
                    if (p.callback)
                    {
                        p.callback(p.shortcut);
                    }
                });
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
            if (value != getGeometry())
            {
                _p->draw.reset();
            }
            IMouseWidget::setGeometry(value);
            FTK_P();
            const ftk::Box2I g = ftk::margin(value, -p.size.keyFocus);
            const int w = std::min(p.clearButton->getSizeHint().w, g.w());
            p.label->setGeometry(ftk::Box2I(
                g.min.x + p.size.pad,
                g.min.y,
                g.w() - w - p.size.pad,
                g.h()));
            p.clearButton->setGeometry(ftk::Box2I(g.max.x + 1 - w, g.min.y, w, g.h()));
        }

        ftk::Box2I ShortcutEdit::getChildrenClipRect() const
        {
            return ftk::margin(getGeometry(), -_p->size.keyFocus);
        }

        ftk::Size2I ShortcutEdit::getSizeHint() const
        {
            FTK_P();
            ftk::Size2I out;
            out.w = std::max(p.label->getSizeHint().w, p.size.minSize) +
                p.size.pad +
                p.clearButton->getSizeHint().w;
            out.h = p.label->getSizeHint().h;
            return out + p.size.keyFocus * 2;
        }

        void ShortcutEdit::styleEvent(const ftk::StyleEvent& event)
        {
            IMouseWidget::styleEvent(event);
            FTK_P();
            if (event.hasChanges())
            {
                p.size.init = true;
                p.draw.reset();
            }
        }

        void ShortcutEdit::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            FTK_P();
            if (p.size.init)
            {
                p.size.init = false;
                p.size = Private::SizeData();
                p.size.minSize = event.style->getSizeRole(ftk::SizeRole::Icon, event.displayScale);
                p.size.border = event.style->getSizeRole(ftk::SizeRole::Border, event.displayScale);
                p.size.keyFocus = event.style->getSizeRole(ftk::SizeRole::KeyFocus, event.displayScale);
                p.size.pad = event.style->getSizeRole(ftk::SizeRole::LabelPad, event.displayScale);
                p.draw.reset();
            }
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
            case ftk::Key::Unknown:
            case ftk::Key::Return:
            case ftk::Key::CapsLock:
            case ftk::Key::ScrollLock:
            case ftk::Key::NumLock:
                break;
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

        struct ShortcutsSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;
            models::ShortcutsSettings shortcuts;
            std::string search;

            // The shortcut of each row of the table, empty for a heading.
            std::vector<std::string> rowNames;

            std::shared_ptr<ftk::SearchBox> searchBox;
            std::shared_ptr<ftk::TableWidget> table;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<models::ShortcutsSettings> > settingsObserver;
        };

        void ShortcutsSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::ShortcutsSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.searchBox = ftk::SearchBox::create(context);
            p.searchBox->setTooltip("Search the shortcuts");
            ftk::setScreenshotTag(p.searchBox, "Shortcuts.Search");

            // The shortcuts are a table of text, with an editor put over a
            // shortcut when it is clicked. A field and a clear button for
            // every shortcut made the list hard to read.
            p.table = ftk::TableWidget::create(context);
            p.table->setHeadingRole(ftk::ColorRole::None);
            p.table->setHeadingLine(true);
            p.table->setColumnLines(true);
            p.table->setTooltip(
                "Click a shortcut to change it. Each action has a\n"
                "primary and a secondary shortcut.");
            ftk::setScreenshotTag(p.table, "Shortcuts.Table");

            p.layout = ftk::VerticalLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::Spacing);
            p.searchBox->setParent(p.layout);
            p.table->setParent(p.layout);

            p.searchBox->setCallback(
                [this](const std::string& value)
                {
                    _searchUpdate(value);
                });

            p.table->setCallback(
                [this](const ftk::TableIndex& index)
                {
                    _edit(index);
                });

            p.settingsObserver = ftk::Observer<models::ShortcutsSettings>::create(
                settings->observeShortcuts(),
                [this](const models::ShortcutsSettings& value)
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
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ShortcutsSettingsWidget>(new ShortcutsSettingsWidget);
            out->_init(context, settings, parent);
            return out;
        }

        void ShortcutsSettingsWidget::_widgetUpdate(const models::ShortcutsSettings& settings)
        {
            FTK_P();
            p.shortcuts = settings;
            _tableUpdate();
        }

        void ShortcutsSettingsWidget::_searchUpdate(const std::string& value)
        {
            FTK_P();
            p.search = value;
            _tableUpdate();
        }

        void ShortcutsSettingsWidget::_tableUpdate()
        {
            FTK_P();

            // Create groups of shortcuts, sorted by name.
            struct Group
            {
                std::string name;
                std::vector<models::Shortcut> shortcuts;
            };
            std::vector<Group> groups;
            for (const auto& shortcut : p.shortcuts.shortcuts)
            {
                const auto s = ftk::split(shortcut.name, '/');
                if (!s.empty())
                {
                    const auto& name = s.front();
                    auto i = std::find_if(
                        groups.begin(),
                        groups.end(),
                        [name](const Group& value)
                        {
                            return name == value.name;
                        });
                    if (i == groups.end())
                    {
                        Group group;
                        group.name = name;
                        groups.push_back(group);
                        i = groups.end() - 1;
                    }
                    i->shortcuts.push_back(shortcut);
                }
            }
            std::sort(
                groups.begin(),
                groups.end(),
                [](const Group& a, const Group& b)
                {
                    return a.name < b.name;
                });

            // Find collisions.
            std::map<std::string, int> collisions;
            for (const auto& i : p.shortcuts.shortcuts)
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
            const auto cell = [&collisions](const ftk::KeyShortcut& value)
                {
                    ftk::TableCell out(
                        ftk::getShortcutLabel(value.key, value.modifiers),
                        true);
                    if (const auto i = collisions.find(to_string(value));
                        i != collisions.end() && i->second > 1)
                    {
                        out.colorRole = ftk::ColorRole::Red;
                    }
                    return out;
                };

            // Create the rows: a heading and the shortcuts for each
            // group. A group with nothing that matches the search is
            // hidden with its heading.
            std::vector<ftk::TableRow> rows;
            p.rowNames.clear();
            for (const auto& group : groups)
            {
                const size_t heading = rows.size();
                rows.push_back(ftk::TableRow({ ftk::TableCell(group.name) }, true));
                rows.back().visible = false;
                p.rowNames.push_back(std::string());
                for (const auto& shortcut : group.shortcuts)
                {
                    ftk::TableRow row(
                        {
                            ftk::TableCell(shortcut.text + ":"),
                            cell(shortcut.primary),
                            cell(shortcut.secondary)
                        });
                    row.visible =
                        p.search.empty() ||
                        ftk::contains(
                            group.name + " " + shortcut.text,
                            p.search,
                            ftk::CaseCompare::Insensitive);
                    rows[heading].visible = rows[heading].visible || row.visible;
                    rows.push_back(row);
                    p.rowNames.push_back(shortcut.name);
                }
            }
            p.table->setRows(rows);
            // The names are as wide as the longest of them and the two
            // shortcuts share the rest.
            p.table->setColumnStretch(1, true);
            p.table->setColumnStretch(2, true);
        }

        void ShortcutsSettingsWidget::_edit(const ftk::TableIndex& index)
        {
            FTK_P();
            if (index.row < 0 || index.row >= static_cast<int>(p.rowNames.size()))
                return;
            const std::string name = p.rowNames[index.row];
            const bool primary = 1 == index.column;
            const auto i = std::find_if(
                p.shortcuts.shortcuts.begin(),
                p.shortcuts.shortcuts.end(),
                [name](const models::Shortcut& value)
                {
                    return name == value.name;
                });
            if (i == p.shortcuts.shortcuts.end())
                return;
            if (auto context = getContext())
            {
                auto widget = ShortcutEdit::create(context);
                widget->setShortcut(primary ? i->primary : i->secondary);
                widget->setCallback(
                    [this, name, primary](const ftk::KeyShortcut& value)
                    {
                        FTK_P();
                        // Close the editor first: the settings come back
                        // through the observer and update the table.
                        p.table->closeEditor();
                        auto settings = p.settings->getShortcuts();
                        const auto i = std::find_if(
                            settings.shortcuts.begin(),
                            settings.shortcuts.end(),
                            [name](const models::Shortcut& other)
                            {
                                return name == other.name;
                            });
                        if (i != settings.shortcuts.end())
                        {
                            if (primary)
                            {
                                i->primary = value;
                            }
                            else
                            {
                                i->secondary = value;
                            }
                            p.settings->setShortcuts(settings);
                        }
                    });
                p.table->openEditor(index, widget);
                widget->takeKeyFocus();
            }
        }
    }
}
