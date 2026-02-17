// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IDialog.h>

namespace djv
{
    namespace models
    {
        class SettingsModel;
        class TimeUnitsModel;
    }

    namespace ui
    {
        //! Setup start widget.
        class SetupStartWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(SetupStartWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            SetupStartWidget();

        public:
            virtual ~SetupStartWidget();

            static std::shared_ptr<SetupStartWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

            FTK_PRIVATE();
        };

        //! Setup dialog.
        class SetupDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(SetupDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<IWidget>& parent);

            SetupDialog();

        public:
            virtual ~SetupDialog();

            static std::shared_ptr<SetupDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
