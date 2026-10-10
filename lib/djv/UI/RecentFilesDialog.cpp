// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/RecentFilesDialog.h>

#include <ftk/UI/CheckBox.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/DrawUtil.h>
#include <ftk/UI/FileBrowserWidgets.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/SearchBox.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace ui
    {
        namespace
        {
            class RecentFilesWidget : public ftk::IWidget
            {
            protected:
                void _init(
                    const std::shared_ptr<ftk::Context>& context,
                    const std::shared_ptr<ftk::FileBrowserModel>& model,
                    const std::vector<ftk::Path>& recent,
                    bool clearOnExit,
                    const std::shared_ptr<IWidget>& parent)
                {
                    IWidget::_init(context, "djv::ui::RecentFilesWidget", parent);

                    setStretch(ftk::Stretch::Expanding, ftk::Stretch::Expanding);

                    _searchBox = ftk::SearchBox::create(context);
                    _searchBox->setTooltip(
                        "Search the recent files, by name or directory.\n"
                        "\n"
                        "Return opens the first of what is found, and the "
                        "down arrow moves into the list.");
                    ftk::setScreenshotTag(_searchBox, "RecentFiles.Search");

                    _view = ftk::FileBrowserView::create(
                        context,
                        ftk::FileBrowserMode::Open,
                        model);
                    _view->setMultiple(true);
                    _view->setPaths(recent);
                    ftk::setScreenshotTag(_view, "RecentFiles.View");
                    _scrollWidget = ftk::ScrollWidget::create(context);
                    _scrollWidget->setWidget(_view);
                    _scrollWidget->setVStretch(ftk::Stretch::Expanding);

                    _countLabel = ftk::Label::create(context);
                    ftk::setScreenshotTag(_countLabel, "RecentFiles.Count");

                    _clearButton = ftk::PushButton::create(context, "Clear");
                    _clearButton->setTooltip(
                        "Forget the recent files, playlists, reviews and "
                        "directories.\n"
                        "\n"
                        "The names of the files that were opened stay in the "
                        "log files, which cover this session and the one "
                        "before it.");
                    ftk::setScreenshotTag(_clearButton, "RecentFiles.Clear");
                    _clearOnExitCheckBox = ftk::CheckBox::create(context, "Clear on exit");
                    _clearOnExitCheckBox->setChecked(clearOnExit);
                    _clearOnExitCheckBox->setTooltip(
                        "Forget the recent files, playlists, reviews and "
                        "directories each time the application exits.");
                    ftk::setScreenshotTag(_clearOnExitCheckBox, "RecentFiles.ClearOnExit");

                    _openButton = ftk::PushButton::create(context, "Open");
                    _cancelButton = ftk::PushButton::create(context, "Cancel");

                    _layout = ftk::VerticalLayout::create(context, shared_from_this());
                    _layout->setSpacingRole(ftk::SizeRole::None);
                    auto vLayout = ftk::VerticalLayout::create(context, _layout);
                    vLayout->setMarginRole(ftk::SizeRole::MarginSmall);
                    vLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
                    vLayout->setVStretch(ftk::Stretch::Expanding);
                    _searchBox->setParent(vLayout);
                    _scrollWidget->setParent(vLayout);
                    ftk::Divider::create(context, ftk::Orientation::Vertical, _layout);
                    auto hLayout = ftk::HorizontalLayout::create(context, _layout);
                    hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
                    hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
                    _clearButton->setParent(hLayout);
                    _clearOnExitCheckBox->setParent(hLayout);
                    hLayout->addSpacer(ftk::SizeRole::Spacing, ftk::Stretch::Expanding);
                    _countLabel->setParent(hLayout);
                    _openButton->setParent(hLayout);
                    _cancelButton->setParent(hLayout);

                    _widgetUpdate();

                    _searchBox->setCallback(
                        [this](const std::string& value)
                        {
                            _view->setSearch(value);
                        });

                    // Return in the search opens the first of what was
                    // found: type a few letters, press return.
                    _searchBox->setReturnCallback(
                        [this]
                        {
                            _view->setCurrent(0);
                            _open(_view->getSelection());
                        });

                    _view->setCallback(
                        [this](const std::vector<ftk::Path>& value)
                        {
                            _open(value);
                        });

                    _view->setSelectCallback(
                        [this](const std::vector<ftk::Path>&)
                        {
                            _widgetUpdate();
                        });

                    _view->setKeyFocusCallback(
                        [this](bool)
                        {
                            setDrawUpdate();
                        });

                    _clearButton->setClickedCallback(
                        [this]
                        {
                            if (_clearCallback)
                            {
                                _clearCallback();
                            }
                        });

                    _clearOnExitCheckBox->setCheckedCallback(
                        [this](bool value)
                        {
                            if (_clearOnExitCallback)
                            {
                                _clearOnExitCallback(value);
                            }
                        });

                    _openButton->setClickedCallback(
                        [this]
                        {
                            _open(_view->getSelection());
                        });

                    _cancelButton->setClickedCallback(
                        [this]
                        {
                            if (_cancelCallback)
                            {
                                _cancelCallback();
                            }
                        });

                    _itemCountObserver = ftk::Observer<size_t>::create(
                        _view->observeItemCount(),
                        [this](size_t)
                        {
                            _widgetUpdate();
                        });

                    _currentObserver = ftk::Observer<int>::create(
                        _view->observeCurrent(),
                        [this](int value)
                        {
                            if (value >= 0)
                            {
                                _scrollWidget->scrollTo(_view->getRect(value));
                            }
                        });
                }

                RecentFilesWidget() = default;

            public:
                static std::shared_ptr<RecentFilesWidget> create(
                    const std::shared_ptr<ftk::Context>& context,
                    const std::shared_ptr<ftk::FileBrowserModel>& model,
                    const std::vector<ftk::Path>& recent,
                    bool clearOnExit,
                    const std::shared_ptr<IWidget>& parent)
                {
                    auto out = std::shared_ptr<RecentFilesWidget>(new RecentFilesWidget);
                    out->_init(context, model, recent, clearOnExit, parent);
                    return out;
                }

                void setRecent(const std::vector<ftk::Path>& value)
                {
                    _view->setPaths(value);
                    _widgetUpdate();
                }

                void setCallback(const std::function<void(const std::vector<ftk::Path>&)>& value)
                {
                    _callback = value;
                }

                void setClearCallback(const std::function<void(void)>& value)
                {
                    _clearCallback = value;
                }

                void setClearOnExitCallback(const std::function<void(bool)>& value)
                {
                    _clearOnExitCallback = value;
                }

                void setCancelCallback(const std::function<void(void)>& value)
                {
                    _cancelCallback = value;
                }

                const std::shared_ptr<ftk::SearchBox>& getSearchBox() const
                {
                    return _searchBox;
                }

                ftk::Size2I getSizeHint() const override
                {
                    return _layout->getSizeHint();
                }

                void setGeometry(const ftk::Box2I& value) override
                {
                    IWidget::setGeometry(value);
                    _layout->setGeometry(value);
                }

                void sizeHintEvent(const ftk::SizeHintEvent& event) override
                {
                    IWidget::sizeHintEvent(event);
                    _keyFocus = event.style->getSizeRole(ftk::SizeRole::KeyFocus, event.displayScale);
                }

                void drawOverlayEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent& event) override
                {
                    IWidget::drawOverlayEvent(drawRect, event);
                    // The ring that says the list has the keyboard, as the
                    // file browser draws it.
                    if (_view->showKeyFocus())
                    {
                        event.render->drawMesh(
                            ftk::border(_scrollWidget->getGeometry(), _keyFocus),
                            event.style->getColorRole(ftk::ColorRole::KeyFocus));
                    }
                }

                void keyPressEvent(ftk::KeyEvent& event) override
                {
                    // The down arrow from the search box, which has no use
                    // for it, moves into the list.
                    if (0 == event.modifiers &&
                        ftk::Key::Down == event.key &&
                        !_view->hasKeyFocus())
                    {
                        event.accept = true;
                        _view->takeKeyFocus();
                        _view->setCurrent(0);
                    }
                    if (!event.accept)
                    {
                        IWidget::keyPressEvent(event);
                    }
                }

            private:
                void _open(const std::vector<ftk::Path>& value)
                {
                    if (!value.empty() && _callback)
                    {
                        _callback(value);
                    }
                }

                void _widgetUpdate()
                {
                    const size_t count = _view->getPaths().size();
                    const size_t shown = _view->observeItemCount()->get();
                    _countLabel->setText(
                        shown != count ?
                        ftk::Format("{0} of {1} files").arg(shown).arg(count).str() :
                        ftk::Format(1 == count ? "{0} file" : "{0} files").arg(count).str());
                    _clearButton->setEnabled(count > 0);
                    _openButton->setEnabled(!_view->getSelection().empty());
                }

                std::shared_ptr<ftk::SearchBox> _searchBox;
                std::shared_ptr<ftk::FileBrowserView> _view;
                std::shared_ptr<ftk::ScrollWidget> _scrollWidget;
                std::shared_ptr<ftk::Label> _countLabel;
                std::shared_ptr<ftk::PushButton> _clearButton;
                std::shared_ptr<ftk::CheckBox> _clearOnExitCheckBox;
                std::shared_ptr<ftk::PushButton> _openButton;
                std::shared_ptr<ftk::PushButton> _cancelButton;
                std::shared_ptr<ftk::VerticalLayout> _layout;
                std::function<void(const std::vector<ftk::Path>&)> _callback;
                std::function<void(void)> _clearCallback;
                std::function<void(bool)> _clearOnExitCallback;
                std::function<void(void)> _cancelCallback;
                std::shared_ptr<ftk::Observer<size_t> > _itemCountObserver;
                std::shared_ptr<ftk::Observer<int> > _currentObserver;
                int _keyFocus = 0;
            };
        }

        struct RecentFilesDialog::Private
        {
            std::shared_ptr<RecentFilesWidget> widget;
            std::function<void(const std::vector<ftk::Path>&)> callback;
        };

        void RecentFilesDialog::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::FileBrowserModel>& model,
            const std::vector<ftk::Path>& recent,
            bool clearOnExit,
            const std::shared_ptr<IWidget>& parent)
        {
            IDialog::_init(context, "djv::ui::RecentFilesDialog", parent);
            FTK_P();

            setTitle("Recent Files");

            p.widget = RecentFilesWidget::create(
                context,
                model,
                recent,
                clearOnExit,
                shared_from_this());

            p.widget->setCallback(
                [this](const std::vector<ftk::Path>& value)
                {
                    // Copied, since closing can release whoever set it.
                    auto callback = _p->callback;
                    close();
                    if (callback)
                    {
                        callback(value);
                    }
                });

            p.widget->setCancelCallback(
                [this]
                {
                    close();
                });
        }

        RecentFilesDialog::RecentFilesDialog() :
            _p(new Private)
        {}

        RecentFilesDialog::~RecentFilesDialog()
        {}

        std::shared_ptr<RecentFilesDialog> RecentFilesDialog::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::FileBrowserModel>& model,
            const std::vector<ftk::Path>& recent,
            bool clearOnExit,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<RecentFilesDialog>(new RecentFilesDialog);
            out->_init(context, model, recent, clearOnExit, parent);
            return out;
        }

        void RecentFilesDialog::setRecent(const std::vector<ftk::Path>& value)
        {
            _p->widget->setRecent(value);
        }

        void RecentFilesDialog::setCallback(
            const std::function<void(const std::vector<ftk::Path>&)>& value)
        {
            _p->callback = value;
        }

        void RecentFilesDialog::setClearCallback(const std::function<void(void)>& value)
        {
            _p->widget->setClearCallback(value);
        }

        void RecentFilesDialog::setClearOnExitCallback(const std::function<void(bool)>& value)
        {
            _p->widget->setClearOnExitCallback(value);
        }

        std::shared_ptr<ftk::IWidget> RecentFilesDialog::getKeyFocus() const
        {
            return _p->widget->getSearchBox();
        }
    }
}
