// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>
#include <djv/Models/Export.h>
#include <djv/Models/OCIOModel.h>

#include <ftk/UI/IContainer.h>
#include <ftk/UI/IWidget.h>

namespace ftk
{
    class CheckBox;
    class Settings;
}

namespace djv
{
    namespace models
    {
        class ColorModel;
        class ViewportModel;
    }

    namespace ui
    {
        //! The OCIO section of the Color tool: the configuration, the input
        //! color space, the display, the view and the look.
        class DJV_UI_API_TYPE OCIOWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(OCIOWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<IWidget>& parent);

            OCIOWidget();

        public:
            DJV_UI_API virtual ~OCIOWidget();

            DJV_UI_API static std::shared_ptr<OCIOWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Get the check box that turns OpenColorIO on and off, which the
            //! Color tool puts in the section's title.
            DJV_UI_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            void _extUpdate();

            FTK_PRIVATE();
        };

        //! The LUT section of the Color tool: the LUT file, and the order and
        //! direction it is applied in.
        class DJV_UI_API_TYPE LUTWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(LUTWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<IWidget>& parent);

            LUTWidget();

        public:
            DJV_UI_API virtual ~LUTWidget();

            DJV_UI_API static std::shared_ptr<LUTWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Get the check box that turns the LUT on and off, which the Color
            //! tool puts in the section's title.
            DJV_UI_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            FTK_PRIVATE();
        };

        //! The Color section of the Color tool: the color values, exposure and
        //! soft clip.
        class DJV_UI_API_TYPE ColorWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ColorWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ColorWidget();

        public:
            DJV_UI_API virtual ~ColorWidget();

            DJV_UI_API static std::shared_ptr<ColorWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Get the check box that turns the color values, exposure and soft
            //! clip on and off together, which the Color tool puts in the
            //! section's title.
            DJV_UI_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            FTK_PRIVATE();
        };

        //! The Levels section of the Color tool.
        class DJV_UI_API_TYPE LevelsWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(LevelsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            LevelsWidget();

        public:
            DJV_UI_API virtual ~LevelsWidget();

            DJV_UI_API static std::shared_ptr<LevelsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Get the check box that turns the levels on and off, which the
            //! Color tool puts in the section's title.
            DJV_UI_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            FTK_PRIVATE();
        };
    }
}
