// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IDialog.h>
#include <ftk/Core/Path.h>

#include <functional>
#include <string>
#include <vector>

namespace djv
{
    namespace ui
    {
        //! Open folder with filter widget.
        class OpenFolderFilterWidget : public ftk::IMouseWidget
        {
            FTK_NON_COPYABLE(OpenFolderFilterWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            OpenFolderFilterWidget();

        public:
            virtual ~OpenFolderFilterWidget();

            static std::shared_ptr<OpenFolderFilterWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setRecentFilters(const std::vector<std::string>&);
            void setFilterPresets(const std::vector<std::string>&);

            void setCallback(const std::function<void(
                const ftk::Path&,
                const std::string&)>&);

            void setCancelCallback(const std::function<void(void)>&);

            //! Keep cancellation available while an asynchronous scan owns
            //! the form inputs.
            void setBusy(bool, const std::string& status = std::string());

            //! Set a validation or scan status message.
            void setStatus(const std::string&);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _recentFiltersUpdate();
            void _filterPresetsUpdate();

            FTK_PRIVATE();
        };

        //! Open folder with filter dialog.
        class OpenFolderFilterDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(OpenFolderFilterDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            OpenFolderFilterDialog();

        public:
            virtual ~OpenFolderFilterDialog();

            static std::shared_ptr<OpenFolderFilterDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setRecentFilters(const std::vector<std::string>&);
            void setFilterPresets(const std::vector<std::string>&);

            //! Set the callback.
            void setCallback(const std::function<void(
                const ftk::Path&,
                const std::string&)>&);

            void setBusy(bool, const std::string& status = std::string());
            void setStatus(const std::string&);

        private:
            FTK_PRIVATE();
        };
    }
}
