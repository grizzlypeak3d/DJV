// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <ftk/UI/IContainer.h>

namespace ftk
{
    class Settings;
}

namespace tl
{
    class Player;
}

namespace djv
{
    namespace ui
    {
        //! Information widget: the I/O information for the current file,
        //! in searchable sections.
        class DJV_UI_API_TYPE InfoWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(InfoWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::shared_ptr<IWidget>& parent);

            InfoWidget();

        public:
            DJV_UI_API virtual ~InfoWidget();

            DJV_UI_API static std::shared_ptr<InfoWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the player.
            DJV_UI_API void setPlayer(const std::shared_ptr<tl::Player>&);

        private:
            void _saveSettings();
            void _widgetUpdate();

            FTK_PRIVATE();
        };
    }
}
