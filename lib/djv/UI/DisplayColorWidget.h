// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <djv/Models/DisplaysModel.h>

#include <ftk/UI/IContainer.h>

#include <functional>

namespace djv
{
    namespace ui
    {
        //! The color a place the picture goes is given: the viewport's
        //! ("Same as viewport"), or an OpenColorIO display and view of its
        //! own. The secondary window's row of the Displays tool, and DJV
        //! Studio's Blackmagic output, choose with this.
        class DJV_UI_API_TYPE DisplayColorWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(DisplayColorWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::string& what,
                const std::shared_ptr<IWidget>& parent);

            DisplayColorWidget();

        public:
            DJV_UI_API virtual ~DisplayColorWidget();

            //! Create a new widget. What it is for, e.g. "the output" or
            //! "the window", is said in its tooltips and its note.
            DJV_UI_API static std::shared_ptr<DisplayColorWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::string& what,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the displays the configuration has, and the views of the
            //! display chosen.
            DJV_UI_API void setLists(const models::DisplayColorLists&);

            //! Set the color chosen.
            DJV_UI_API void setColor(const models::DisplayColor&);

            //! Set the input color space it renders from, and where it came
            //! from; shown while it is color managed at all.
            DJV_UI_API void setResolvedInput(const std::string&);

            //! Set whether the viewport color manages: when it does not, a
            //! place following it does not either, which is said.
            DJV_UI_API void setOCIOEnabled(bool);

            //! Set the callback for a color chosen.
            DJV_UI_API void setCallback(const std::function<void(const models::DisplayColor&)>&);

        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };
    }
}
