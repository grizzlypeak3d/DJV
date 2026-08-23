// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>
#include <djv/Models/Export.h>

#include <ftk/UI/IContainer.h>
#include <ftk/UI/IDialog.h>

namespace djv
{
    namespace models
    {
        class AppInfoModel;
        class SettingsModel;
        class TimeUnitsModel;
    }

    namespace ui
    {
        //! Setup start widget.
        class DJV_UI_API_TYPE SetupStartWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(SetupStartWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::AppInfoModel>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            SetupStartWidget();

        public:
            DJV_UI_API virtual ~SetupStartWidget();

            DJV_UI_API static std::shared_ptr<SetupStartWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::AppInfoModel>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

            FTK_PRIVATE();
        };

        //! Setup dialog.
        class DJV_UI_API_TYPE SetupDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(SetupDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::AppInfoModel>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<IWidget>& parent);

            SetupDialog();

        public:
            DJV_UI_API virtual ~SetupDialog();

            DJV_UI_API static std::shared_ptr<SetupDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::AppInfoModel>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_UI_API std::shared_ptr<ftk::IWidget> getKeyFocus() const override;

        private:
            FTK_PRIVATE();
        };
    }
}
