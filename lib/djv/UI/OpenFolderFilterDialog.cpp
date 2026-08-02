// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/OpenFolderFilterDialog.h>

#include <ftk/UI/ComboBox.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/FileBrowser.h>
#include <ftk/UI/FileEdit.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/LineEdit.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/Spacer.h>

namespace djv
{
    namespace ui
    {
        struct OpenFolderFilterWidget::Private
        {
            std::vector<std::string> recentFilters;
            std::vector<std::string> filterPresets;

            std::shared_ptr<ftk::FileEdit> folderEdit;
            std::shared_ptr<ftk::LineEdit> filterEdit;
            std::shared_ptr<ftk::ComboBox> recentComboBox;
            std::shared_ptr<ftk::ComboBox> presetComboBox;
            std::shared_ptr<ftk::Label> statusLabel;
            std::shared_ptr<ftk::PushButton> okButton;
            std::shared_ptr<ftk::PushButton> cancelButton;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::function<void(const ftk::Path&, const std::string&)> callback;
            std::function<void(void)> cancelCallback;
        };

        void OpenFolderFilterWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            IMouseWidget::_init(
                context,
                "djv::ui::OpenFolderFilterWidget",
                parent);
            FTK_P();

            _setMouseHoverEnabled(true);
            _setMousePressEnabled(true);

            p.folderEdit = ftk::FileEdit::create(context, ftk::FileBrowserMode::Dir);
            p.folderEdit->setTooltip("Folder to open recursively.");

            p.filterEdit = ftk::LineEdit::create(context);
            p.filterEdit->setHStretch(ftk::Stretch::Expanding);
            p.filterEdit->setFormat(std::string(40, '#'));
            p.filterEdit->setTooltip(
                "Filter loaded files with case-insensitive regex tokens.\n"
                "\n"
                "Examples:\n"
                "* plate render\n"
                "* name:exr$\n"
                "* dir:(shot010|shot020)\n"
                "* -dir:(cache|tmp)\n"
                "* -name:proxy");

            p.recentComboBox = ftk::ComboBox::create(context);
            p.recentComboBox->setHStretch(ftk::Stretch::Expanding);
            p.recentComboBox->setTooltip("Recent folder filters.");

            p.presetComboBox = ftk::ComboBox::create(context);
            p.presetComboBox->setHStretch(ftk::Stretch::Expanding);
            p.presetComboBox->setTooltip("Preset folder filters.");

            p.okButton = ftk::PushButton::create(context, "OK");
            p.cancelButton = ftk::PushButton::create(context, "Cancel");

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::None);
            auto label = ftk::Label::create(context, "Open Folder With Filter", p.layout);
            label->setFontSize(14);
            label->setMarginRole(ftk::SizeRole::Margin);
            label->setBackgroundRole(ftk::ColorRole::Button);
            auto formLayout = ftk::FormLayout::create(context, p.layout);
            formLayout->setVStretch(ftk::Stretch::Expanding);
            formLayout->setMarginRole(ftk::SizeRole::Margin);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Folder:", p.folderEdit);
            formLayout->addRow("Filter:", p.filterEdit);
            formLayout->addRow("Recent:", p.recentComboBox);
            formLayout->addRow("Preset:", p.presetComboBox);
            p.statusLabel = ftk::Label::create(context, std::string(), p.layout);
            p.statusLabel->setMarginRole(ftk::SizeRole::MarginSmall);
            ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto spacer = ftk::Spacer::create(context, ftk::Orientation::Horizontal, hLayout);
            spacer->setSpacingRole(ftk::SizeRole::None);
            spacer->setHStretch(ftk::Stretch::Expanding);
            p.okButton->setParent(hLayout);
            p.cancelButton->setParent(hLayout);

            p.recentComboBox->setIndexCallback(
                [this](int value)
                {
                    if (value >= 0 && value < static_cast<int>(_p->recentFilters.size()))
                    {
                        _p->filterEdit->setText(_p->recentFilters[value]);
                    }
                });

            p.presetComboBox->setIndexCallback(
                [this](int value)
                {
                    if (value >= 0 && value < static_cast<int>(_p->filterPresets.size()))
                    {
                        _p->filterEdit->setText(_p->filterPresets[value]);
                    }
                });

            p.okButton->setClickedCallback(
                [this]
                {
                    if (_p->callback)
                    {
                        _p->callback(
                            ftk::Path(_p->folderEdit->getPath()),
                            _p->filterEdit->getText());
                    }
                });

            p.cancelButton->setClickedCallback(
                [this]
                {
                    if (_p->cancelCallback)
                    {
                        _p->cancelCallback();
                    }
                });

            _recentFiltersUpdate();
            _filterPresetsUpdate();
        }

        OpenFolderFilterWidget::OpenFolderFilterWidget() :
            _p(new Private)
        {}

        OpenFolderFilterWidget::~OpenFolderFilterWidget()
        {}

        std::shared_ptr<OpenFolderFilterWidget> OpenFolderFilterWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<OpenFolderFilterWidget>(new OpenFolderFilterWidget);
            out->_init(context, parent);
            return out;
        }

        void OpenFolderFilterWidget::setRecentFilters(const std::vector<std::string>& value)
        {
            _p->recentFilters = value;
            _recentFiltersUpdate();
        }

        void OpenFolderFilterWidget::setFilterPresets(const std::vector<std::string>& value)
        {
            _p->filterPresets = value;
            _filterPresetsUpdate();
        }

        void OpenFolderFilterWidget::setCallback(const std::function<void(
            const ftk::Path&,
            const std::string&)>& value)
        {
            _p->callback = value;
        }

        void OpenFolderFilterWidget::setCancelCallback(const std::function<void(void)>& value)
        {
            _p->cancelCallback = value;
        }

        void OpenFolderFilterWidget::setBusy(
            bool value,
            const std::string& status)
        {
            FTK_P();
            p.folderEdit->setEnabled(!value);
            p.filterEdit->setEnabled(!value);
            p.recentComboBox->setEnabled(
                !value && !p.recentFilters.empty());
            p.presetComboBox->setEnabled(
                !value && !p.filterPresets.empty());
            p.okButton->setEnabled(!value);
            if (!status.empty())
            {
                p.statusLabel->setText(status);
            }
        }

        void OpenFolderFilterWidget::setStatus(const std::string& value)
        {
            _p->statusLabel->setText(value);
        }

        ftk::Size2I OpenFolderFilterWidget::getSizeHint() const
        {
            ftk::Size2I out = _p->layout->getSizeHint();
            out.w *= 2;
            return out;
        }

        void OpenFolderFilterWidget::setGeometry(const ftk::Box2I& value)
        {
            IMouseWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void OpenFolderFilterWidget::_recentFiltersUpdate()
        {
            FTK_P();
            p.recentComboBox->setItems(p.recentFilters);
            p.recentComboBox->setEnabled(!p.recentFilters.empty());
            p.recentComboBox->setCurrentIndex(-1);
        }

        void OpenFolderFilterWidget::_filterPresetsUpdate()
        {
            FTK_P();
            p.presetComboBox->setItems(p.filterPresets);
            p.presetComboBox->setEnabled(!p.filterPresets.empty());
            p.presetComboBox->setCurrentIndex(-1);
        }

        struct OpenFolderFilterDialog::Private
        {
            std::shared_ptr<OpenFolderFilterWidget> widget;
        };

        void OpenFolderFilterDialog::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            IDialog::_init(
                context,
                "djv::ui::OpenFolderFilterDialog",
                parent);
            FTK_P();

            p.widget = OpenFolderFilterWidget::create(
                context,
                shared_from_this());

            p.widget->setCancelCallback(
                [this]
                {
                    close();
                });
        }

        OpenFolderFilterDialog::OpenFolderFilterDialog() :
            _p(new Private)
        {}

        OpenFolderFilterDialog::~OpenFolderFilterDialog()
        {}

        std::shared_ptr<OpenFolderFilterDialog> OpenFolderFilterDialog::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<OpenFolderFilterDialog>(new OpenFolderFilterDialog);
            out->_init(context, parent);
            return out;
        }

        void OpenFolderFilterDialog::setRecentFilters(const std::vector<std::string>& value)
        {
            _p->widget->setRecentFilters(value);
        }

        void OpenFolderFilterDialog::setFilterPresets(const std::vector<std::string>& value)
        {
            _p->widget->setFilterPresets(value);
        }

        void OpenFolderFilterDialog::setCallback(const std::function<void(
            const ftk::Path&,
            const std::string&)>& value)
        {
            _p->widget->setCallback(value);
        }

        void OpenFolderFilterDialog::setBusy(
            bool value,
            const std::string& status)
        {
            _p->widget->setBusy(value, status);
        }

        void OpenFolderFilterDialog::setStatus(const std::string& value)
        {
            _p->widget->setStatus(value);
        }
    }
}
