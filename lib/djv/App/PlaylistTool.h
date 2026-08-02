// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/IToolWidget.h>

namespace djv
{
    namespace app
    {
        class App;

        //! OTIO media playlist tool.
        class PlaylistTool : public IToolWidget
        {
            FTK_NON_COPYABLE(PlaylistTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent);

            PlaylistTool();

        public:
            ~PlaylistTool();

            static std::shared_ptr<PlaylistTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void dragEnterEvent(ftk::DragDropEvent&) override;
            void dropEvent(ftk::DragDropEvent&) override;

        private:
            void _update();
            void _setSelected(int);

            FTK_PRIVATE();
        };
    }
}
