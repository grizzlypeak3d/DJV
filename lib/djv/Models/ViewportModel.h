// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <tlRender/Timeline/BackgroundOptions.h>
#include <tlRender/Timeline/DisplayOptions.h>
#include <tlRender/Timeline/ForegroundOptions.h>
#include <tlRender/Core/HDR.h>

#include <ftk/Core/Image.h>
#include <ftk/Core/Observable.h>

namespace ftk
{
    class Context;
    class Settings;
}

namespace djv
{
    namespace models
    {
        //! Aspect ratio options.
        struct DJV_MODELS_API_TYPE AspectRatioOptions
        {
            //! The entry in use: zero is Default, the file's own aspect ratio,
            //! and the others are the custom entries.
            int index = 0;

            //! The entries: Default, which leaves the file's own aspect ratio,
            //! then the custom ones, which the View tool edits.
            std::vector<tl::AspectRatioOptions> options =
            {
                tl::AspectRatioOptions(),
                tl::AspectRatioOptions(
                    tl::AspectRatio(16.F, 9.F),
                    tl::AspectRatioType::Display),
                tl::AspectRatioOptions(
                    tl::AspectRatio(1.85F),
                    tl::AspectRatioType::Display),
                tl::AspectRatioOptions(
                    tl::AspectRatio(2.39F),
                    tl::AspectRatioType::Display)
            };

            bool operator == (const AspectRatioOptions&) const = default;
        };

        //! HUD items.
        enum class DJV_MODELS_API_TYPE HUDItem
        {
            //! The options map is serialized with the enumeration index as the
            //! key, so changing this order requires a new settings version.
            FileName,
            Info,
            Cache,
            Time,
            ViewZoom,
            ColorPicker,
            Render,
            LUT,

            Count,
            First = FileName
        };
        FTK_ENUM(DJV_MODELS_API, HUDItem);

        //! Get the HUD items in the order they are listed and stacked, which
        //! the enumeration cannot change without a new settings version.
        DJV_MODELS_API std::vector<HUDItem> getHUDItemOrder();

        //! HUD positions.
        enum class DJV_MODELS_API_TYPE HUDPos
        {
            None,
            TopLeft,
            TopRight,
            BottomLeft,
            BottomRight,

            Count,
            First = None
        };
        FTK_ENUM(DJV_MODELS_API, HUDPos);

        //! HUD options.
        struct DJV_MODELS_API_TYPE HUDOptions
        {
            //! Whether the HUD is shown: View > HUD / Information Display.
            bool enabled = false;
            //! The corner each item is shown in. An item that is not in the
            //! map, or is at None, is not shown.
            std::map<HUDItem, HUDPos> items;

            bool operator == (const HUDOptions&) const = default;
        };

        //! Viewport model.
        class DJV_MODELS_API_TYPE ViewportModel : public std::enable_shared_from_this<ViewportModel>
        {
            FTK_NON_COPYABLE(ViewportModel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            ViewportModel();

        public:
            DJV_MODELS_API ~ViewportModel();

            //! Create a new model.
            DJV_MODELS_API static std::shared_ptr<ViewportModel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            //! Save the settings.
            DJV_MODELS_API void save();

            //! \name Image Options
            ///@{

            //! Get the image options: the minify and magnify filters, the video
            //! levels and the alpha blend the picture is drawn with.
            DJV_MODELS_API const ftk::ImageOptions& getImageOptions() const;
            //! Observe the image options.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<ftk::ImageOptions> > observeImageOptions() const;
            //! Set the image options.
            DJV_MODELS_API void setImageOptions(const ftk::ImageOptions&);

            ///@}

            //! \name Display Options
            ///@{

            //! Get the display options: the channels, the mirroring, the
            //! negative and the color adjustments.
            DJV_MODELS_API const tl::DisplayOptions& getDisplayOptions() const;
            //! Observe the display options.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::DisplayOptions> > observeDisplayOptions() const;
            //! Set the display options. Their aspect ratio is replaced by the
            //! one the aspect ratio options have chosen.
            DJV_MODELS_API void setDisplayOptions(const tl::DisplayOptions&);

            //! Whether any of the color adjustments is on: the color
            //! values, levels, exposure or soft clip. The channels, the
            //! mirroring and the negative are how the picture is viewed,
            //! not adjustments.
            DJV_MODELS_API bool hasColorAdjustments() const;

            //! Reset the color adjustments to their defaults: the color
            //! values, exposure and soft clip with "color", the levels with
            //! "levels". The view options beside them are left as they are.
            DJV_MODELS_API void resetColorAdjustments(bool color = true, bool levels = true);

            ///@}

            //! \name Aspect Ratio Options
            ///@{

            //! Get the aspect ratio options.
            DJV_MODELS_API const AspectRatioOptions& getAspectRatioOptions() const;
            //! Observe the aspect ratio options.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<AspectRatioOptions> > observeAspectRatioOptions() const;
            //! Set the aspect ratio options. The entry chosen becomes the
            //! display options' aspect ratio.
            DJV_MODELS_API void setAspectRatioOptions(const AspectRatioOptions&);
            ///@}

            //! \name Background Options
            ///@{

            //! Get the background options: what is drawn behind the picture,
            //! and the outline around it.
            DJV_MODELS_API const tl::BackgroundOptions& getBackgroundOptions() const;
            //! Observe the background options.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::BackgroundOptions> > observeBackgroundOptions() const;
            //! Set the background options.
            DJV_MODELS_API void setBackgroundOptions(const tl::BackgroundOptions&);

            ///@}

            //! \name Foreground Options
            ///@{

            //! Get the foreground options: the grid, the center marker, the
            //! missing frame indicator, the clipping warning, the frame guide,
            //! the safe areas and the data window outline.
            DJV_MODELS_API const tl::ForegroundOptions& getForegroundOptions() const;
            //! Observe the foreground options.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::ForegroundOptions> > observeForegroundOptions() const;
            //! Set the foreground options.
            DJV_MODELS_API void setForegroundOptions(const tl::ForegroundOptions&);

            ///@}

            //! \name Color Buffer
            ///@{

            //! Get the type of the view's color buffer, which is its bit depth.
            DJV_MODELS_API ftk::ImageType getColorBuffer() const;
            //! Observe the type of the view's color buffer.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<ftk::ImageType> > observeColorBuffer() const;
            //! Set the type of the view's color buffer.
            DJV_MODELS_API void setColorBuffer(ftk::ImageType);

            ///@}

            //! \name HDR
            //! What the picture is display encoded for, which nothing says
            //! but the person who chose the display it is transformed to:
            //! SDR, or PQ. And the luminance the window's white stands for
            //! when a PQ picture is drawn into it, where the system does
            //! not say. A window is HDR only with the GPU renderer.
            ///@{

            //! Get what the picture is display encoded for, SDR by default.
            DJV_MODELS_API tl::HDR_EOTF getHDRTransfer() const;
            //! Observe what the picture is display encoded for.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::HDR_EOTF> > observeHDRTransfer() const;
            //! Set what the picture is display encoded for.
            DJV_MODELS_API void setHDRTransfer(tl::HDR_EOTF);

            //! Get the luminance the window's white stands for, in nits; 203 by
            //! default. It is used where the system does not say.
            DJV_MODELS_API float getHDRWhite() const;
            //! Observe the luminance the window's white stands for.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<float> > observeHDRWhite() const;
            //! Set the luminance the window's white stands for, in nits.
            DJV_MODELS_API void setHDRWhite(float);

            ///@}

            //! \name Dither
            //! Dithering of the picture written to an eight bit window, so
            //! that gradients do not band. Only what is shown; the color
            //! picker, the magnifier and the exports read the picture before
            //! it. Off by default, since it changes what is shown.
            ///@{

            //! Get whether the picture is dithered.
            DJV_MODELS_API bool getDither() const;
            //! Observe whether the picture is dithered.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<bool> > observeDither() const;
            //! Set whether the picture is dithered.
            DJV_MODELS_API void setDither(bool);

            ///@}

            //! \name HUD
            ///@{

            //! Get the HUD options.
            DJV_MODELS_API const HUDOptions& getHUDOptions() const;
            //! Observe the HUD options.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<HUDOptions> > observeHUDOptions() const;
            //! Set the HUD options.
            DJV_MODELS_API void setHUDOptions(const HUDOptions&);

            ///@}

        private:
            FTK_PRIVATE();
        };

        //! \name Serialize
        ///@{

        DJV_MODELS_API void to_json(nlohmann::json&, const AspectRatioOptions&);
        DJV_MODELS_API void to_json(nlohmann::json&, const HUDOptions&);

        DJV_MODELS_API void from_json(const nlohmann::json&, AspectRatioOptions&);
        DJV_MODELS_API void from_json(const nlohmann::json&, HUDOptions&);

        ///@}
    }
}
