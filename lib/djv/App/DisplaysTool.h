// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/App/IToolWidget.h>

namespace djv
{
    namespace app
    {
        //! The places the picture goes, and the color each is given: the
        //! viewport, with its color from the Color tool, and the secondary
        //! window, with a monitor to fill and a color of its own. An
        //! application with outputs of its own adds them with addSection()
        //! (DJV Studio's Blackmagic output).
        class DJV_APP_API_TYPE DisplaysTool : public IToolWidget
        {
            FTK_NON_COPYABLE(DisplaysTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent);

            DisplaysTool();

        public:
            DJV_APP_API virtual ~DisplaysTool();

            DJV_APP_API static std::shared_ptr<DisplaysTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Add a section after the built in ones: a title, what it
            //! shows, and a widget for its header, such as a check box that
            //! turns it on.
            DJV_APP_API void addSection(
                const std::string& title,
                const std::shared_ptr<IWidget>&,
                const std::shared_ptr<IWidget>& headerWidget = nullptr);

        private:
            void _monitorsUpdate();

            FTK_PRIVATE();
        };
    }
}
