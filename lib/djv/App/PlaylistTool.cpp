// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/PlaylistTool.h>

#include <djv/App/App.h>
#include <djv/Models/PlaylistModel.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/Spacer.h>

#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>

#include <cmath>

namespace djv
{
    namespace app
    {
        namespace
        {
            class PlaylistDragData : public ftk::IDragDropData
            {
            public:
                explicit PlaylistDragData(size_t value) :
                    index(value)
                {}

                ~PlaylistDragData() override = default;

                size_t index = 0;
            };

            class PlaylistRow : public ftk::PushButton
            {
            protected:
                void _init(
                    const std::shared_ptr<ftk::Context>& context,
                    size_t index,
                    const std::string& text,
                    const std::string& tooltip,
                    bool editable,
                    const std::function<void(size_t, size_t)>& moveCallback,
                    const std::function<void(const std::vector<ftk::Path>&)>& addCallback,
                    const std::shared_ptr<IWidget>& parent)
                {
                    PushButton::_init(context, parent);
                    setText(text);
                    setTooltip(tooltip);
                    setCheckable(true);
                    setHStretch(ftk::Stretch::Expanding);
                    _index = index;
                    _editable = editable;
                    _moveCallback = moveCallback;
                    _addCallback = addCallback;
                }

                PlaylistRow() = default;

            public:
                ~PlaylistRow() override = default;

                static std::shared_ptr<PlaylistRow> create(
                    const std::shared_ptr<ftk::Context>& context,
                    size_t index,
                    const std::string& text,
                    const std::string& tooltip,
                    bool editable,
                    const std::function<void(size_t, size_t)>& moveCallback,
                    const std::function<void(const std::vector<ftk::Path>&)>& addCallback,
                    const std::shared_ptr<IWidget>& parent = nullptr)
                {
                    auto out = std::shared_ptr<PlaylistRow>(new PlaylistRow);
                    out->_init(
                        context,
                        index,
                        text,
                        tooltip,
                        editable,
                        moveCallback,
                        addCallback,
                        parent);
                    return out;
                }

                void sizeHintEvent(const ftk::SizeHintEvent& event) override
                {
                    PushButton::sizeHintEvent(event);
                    _dragLength = event.style->getSizeRole(
                        ftk::SizeRole::DragLength,
                        event.displayScale);
                }

                void mouseMoveEvent(ftk::MouseMoveEvent& event) override
                {
                    PushButton::mouseMoveEvent(event);
                    if (_editable && _isMousePressed())
                    {
                        const float length = ftk::length(event.pos - _getMousePressPos());
                        if (length > _dragLength)
                        {
                            event.dragDropData =
                                std::make_shared<PlaylistDragData>(_index);
                        }
                    }
                }

                void dragEnterEvent(ftk::DragDropEvent& event) override
                {
                    if (!_editable)
                    {
                        return;
                    }
                    event.accept =
                        static_cast<bool>(
                            std::dynamic_pointer_cast<PlaylistDragData>(event.data)) ||
                        static_cast<bool>(
                            std::dynamic_pointer_cast<ftk::DragDropTextData>(event.data));
                }

                void dropEvent(ftk::DragDropEvent& event) override
                {
                    if (!_editable)
                    {
                        return;
                    }
                    if (auto data =
                        std::dynamic_pointer_cast<PlaylistDragData>(event.data))
                    {
                        event.accept = true;
                        if (_moveCallback)
                        {
                            _moveCallback(data->index, _index);
                        }
                    }
                    else if (auto data =
                        std::dynamic_pointer_cast<ftk::DragDropTextData>(event.data))
                    {
                        event.accept = true;
                        std::vector<ftk::Path> paths;
                        for (const auto& value : data->getText())
                        {
                            paths.push_back(ftk::Path(value));
                        }
                        if (_addCallback)
                        {
                            _addCallback(paths);
                        }
                    }
                }

            private:
                size_t _index = 0;
                bool _editable = false;
                int _dragLength = 0;
                std::function<void(size_t, size_t)> _moveCallback;
                std::function<void(const std::vector<ftk::Path>&)> _addCallback;
            };

            std::string getDurationText(const OTIO_NS::RationalTime& value)
            {
                const int64_t frames = static_cast<int64_t>(std::llround(value.value()));
                return ftk::Format("{0}f").arg(frames).str();
            }
        }

        struct PlaylistTool::Private
        {
            std::weak_ptr<App> app;
            int selected = -1;
            std::shared_ptr<ftk::Label> pathLabel;
            std::shared_ptr<ftk::Label> statusLabel;
            std::shared_ptr<ftk::VerticalLayout> listLayout;
            std::vector<std::shared_ptr<PlaylistRow> > rows;
            std::shared_ptr<ftk::PushButton> addButton;
            std::shared_ptr<ftk::PushButton> addFolderButton;
            std::shared_ptr<ftk::PushButton> removeButton;
            std::shared_ptr<ftk::PushButton> saveButton;
            std::shared_ptr<ftk::PushButton> saveAsButton;
            std::shared_ptr<ftk::Observer<int> > revisionObserver;
        };

        void PlaylistTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                mainWindow,
                "OTIO Playlist",
                "Files",
                "djv::app::PlaylistTool",
                parent);
            FTK_P();
            p.app = app;

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);

            auto headerLayout = ftk::HorizontalLayout::create(context, layout);
            headerLayout->setMarginRole(ftk::SizeRole::Margin);
            headerLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto titleLabel = ftk::Label::create(context, "OTIO Playlist", headerLayout);
            titleLabel->setFont(ftk::FontType::Bold);
            titleLabel->setHStretch(ftk::Stretch::Expanding);

            p.pathLabel = ftk::Label::create(context, layout);
            p.pathLabel->setMarginRole(ftk::SizeRole::MarginSmall);
            p.pathLabel->setTextRole(ftk::ColorRole::TextDisabled);

            p.statusLabel = ftk::Label::create(context, layout);
            p.statusLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);

            p.listLayout = ftk::VerticalLayout::create(context);
            p.listLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.listLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            auto scrollWidget = ftk::ScrollWidget::create(
                context,
                ftk::ScrollType::Vertical,
                layout);
            scrollWidget->setBorder(false);
            scrollWidget->setVStretch(ftk::Stretch::Expanding);
            scrollWidget->setWidget(p.listLayout);

            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);

            auto actionsLayout = ftk::HorizontalLayout::create(context, layout);
            actionsLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            actionsLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.addButton = ftk::PushButton::create(context, "Add", actionsLayout);
            p.addButton->setTooltip("Add an image or video to the end of the playlist.");
            p.addFolderButton = ftk::PushButton::create(
                context,
                "Add Folder",
                actionsLayout);
            p.addFolderButton->setTooltip(
                "Add matching media from a folder and its subfolders.");
            p.removeButton = ftk::PushButton::create(context, "Remove", actionsLayout);
            p.removeButton->setTooltip("Remove the selected playlist item.");
            p.saveButton = ftk::PushButton::create(context, "Save", actionsLayout);
            p.saveButton->setTooltip("Save the active OTIO playlist.");
            p.saveAsButton = ftk::PushButton::create(context, "Save As", actionsLayout);
            p.saveAsButton->setTooltip("Save the active timeline as an .otio file.");

            _setWidget(layout);

            std::weak_ptr<App> appWeak(app);
            p.addButton->setClickedCallback(
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->openPlaylistMediaDialog();
                    }
                });
            p.addFolderButton->setClickedCallback(
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->openPlaylistFolderDialog(false);
                    }
                });
            p.removeButton->setClickedCallback(
                [this, appWeak]
                {
                    if (_p->selected >= 0)
                    {
                        if (auto app = appWeak.lock())
                        {
                            app->removePlaylistMedia(
                                static_cast<size_t>(_p->selected));
                        }
                    }
                });
            p.saveButton->setClickedCallback(
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->savePlaylist();
                    }
                });
            p.saveAsButton->setClickedCallback(
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->savePlaylistAsDialog();
                    }
                });

            p.revisionObserver = ftk::Observer<int>::create(
                app->getPlaylistModel()->observeRevision(),
                [this](int)
                {
                    _update();
                });
        }

        PlaylistTool::PlaylistTool() :
            _p(new Private)
        {}

        PlaylistTool::~PlaylistTool()
        {}

        std::shared_ptr<PlaylistTool> PlaylistTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<PlaylistTool>(new PlaylistTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }

        void PlaylistTool::dragEnterEvent(ftk::DragDropEvent& event)
        {
            FTK_P();
            if (auto app = p.app.lock())
            {
                if (app->getPlaylistModel()->isEditable())
                {
                    event.accept =
                        static_cast<bool>(
                            std::dynamic_pointer_cast<PlaylistDragData>(event.data)) ||
                        static_cast<bool>(
                            std::dynamic_pointer_cast<ftk::DragDropTextData>(event.data));
                }
            }
        }

        void PlaylistTool::dropEvent(ftk::DragDropEvent& event)
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app || !app->getPlaylistModel()->isEditable())
            {
                return;
            }
            if (auto data = std::dynamic_pointer_cast<PlaylistDragData>(event.data))
            {
                event.accept = true;
                const auto& items = app->getPlaylistModel()->getItems();
                if (!items.empty())
                {
                    app->movePlaylistMedia(data->index, items.size() - 1);
                }
            }
            else if (auto data =
                std::dynamic_pointer_cast<ftk::DragDropTextData>(event.data))
            {
                event.accept = true;
                std::vector<ftk::Path> paths;
                for (const auto& value : data->getText())
                {
                    paths.push_back(ftk::Path(value));
                }
                app->addPlaylistMedia(paths);
            }
        }

        void PlaylistTool::_update()
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
            {
                return;
            }
            auto model = app->getPlaylistModel();
            const auto& items = model->getItems();
            if (p.selected >= static_cast<int>(items.size()))
            {
                p.selected = items.empty() ? -1 : static_cast<int>(items.size()) - 1;
            }

            p.pathLabel->setText(
                model->isScratch() ?
                "Untitled playlist" :
                ftk::elide(model->getPath().getFileName()));
            p.pathLabel->setTooltip(model->getPath().get());

            if (!model->getLastError().empty())
            {
                p.statusLabel->setText(model->getLastError());
                p.statusLabel->setTextRole(ftk::ColorRole::Red);
            }
            else if (model->isEditable())
            {
                p.statusLabel->setText(
                    model->isDirty() ?
                    "Editable - unsaved changes" :
                    "Editable - saved");
                p.statusLabel->setTextRole(
                    model->isDirty() ?
                    ftk::ColorRole::Yellow :
                    ftk::ColorRole::Green);
            }
            else
            {
                p.statusLabel->setText(
                    model->getReadOnlyReason().empty() ?
                    "Read only" :
                    model->getReadOnlyReason());
                p.statusLabel->setTextRole(ftk::ColorRole::Yellow);
            }

            p.listLayout->clear();
            p.rows.clear();
            std::weak_ptr<App> appWeak(app);
            for (size_t i = 0; i < items.size(); ++i)
            {
                const auto& item = items[i];
                const std::string text = ftk::Format(
                    "{0}.  {1}  [{2}{3}]").
                    arg(i + 1).
                    arg(item.name).
                    arg(getDurationText(item.duration)).
                    arg(item.hasAudio ? ", audio" : "").
                    str();
                std::string tooltip = item.path.get();
                if (item.missing)
                {
                    tooltip += "\nMissing media";
                }
                auto row = PlaylistRow::create(
                    getContext(),
                    i,
                    text,
                    tooltip,
                    model->isEditable(),
                    [appWeak](size_t from, size_t to)
                    {
                        if (auto app = appWeak.lock())
                        {
                            app->movePlaylistMedia(from, to);
                        }
                    },
                    [appWeak](const std::vector<ftk::Path>& paths)
                    {
                        if (auto app = appWeak.lock())
                        {
                            app->addPlaylistMedia(paths);
                        }
                    },
                    p.listLayout);
                row->setChecked(static_cast<int>(i) == p.selected);
                row->setClickedCallback(
                    [this, i]
                    {
                        _setSelected(static_cast<int>(i));
                    });
                p.rows.push_back(row);
            }
            if (items.empty())
            {
                auto label = ftk::Label::create(
                    getContext(),
                    "Drop images or videos here",
                    p.listLayout);
                label->setMarginRole(ftk::SizeRole::Margin);
                label->setHAlign(ftk::HAlign::Center);
            }
            auto spacer = ftk::Spacer::create(
                getContext(),
                ftk::Orientation::Vertical,
                p.listLayout);
            spacer->setVStretch(ftk::Stretch::Expanding);

            p.addButton->setEnabled(model->isEditable());
            p.addFolderButton->setEnabled(model->isEditable());
            p.removeButton->setEnabled(
                model->isEditable() &&
                p.selected >= 0 &&
                items.size() > 1);
            p.saveButton->setEnabled(
                model->isEditable() &&
                model->isDirty());
            p.saveAsButton->setEnabled(model->isAvailable());
        }

        void PlaylistTool::_setSelected(int value)
        {
            FTK_P();
            p.selected = value;
            for (size_t i = 0; i < p.rows.size(); ++i)
            {
                p.rows[i]->setChecked(static_cast<int>(i) == value);
            }
            if (auto app = p.app.lock())
            {
                p.removeButton->setEnabled(
                    app->getPlaylistModel()->isEditable() &&
                    value >= 0 &&
                    app->getPlaylistModel()->getItems().size() > 1);
            }
        }
    }
}
