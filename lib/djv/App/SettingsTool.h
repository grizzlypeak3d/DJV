// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/App/IToolWidget.h>
#include <djv/Models/Export.h>

namespace djv
{
    namespace app
    {
        //! Settings tool.
        class DJV_APP_API_TYPE SettingsTool : public IToolWidget
        {
            FTK_NON_COPYABLE(SettingsTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent);

            SettingsTool();

        public:
            DJV_APP_API virtual ~SettingsTool();

            //! Create a new tool.
            DJV_APP_API static std::shared_ptr<SettingsTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Scroll to the given section.
            DJV_APP_API void scrollTo(const std::string&);

        private:
            FTK_PRIVATE();
        };
    }
}
