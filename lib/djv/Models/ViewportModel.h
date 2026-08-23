// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <tlRender/Timeline/BackgroundOptions.h>
#include <tlRender/Timeline/DisplayOptions.h>
#include <tlRender/Timeline/ForegroundOptions.h>

#include <ftk/GL/Texture.h>
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
            int index = 0;

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

            DJV_MODELS_API bool operator == (const AspectRatioOptions&) const;
            DJV_MODELS_API bool operator != (const AspectRatioOptions&) const;
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

            Count,
            First = FileName
        };
        TL_ENUM(HUDItem);

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
        TL_ENUM(HUDPos);

        //! HUD options.
        struct DJV_MODELS_API_TYPE HUDOptions
        {
            bool enabled = false;
            std::map<HUDItem, HUDPos> items;

            DJV_MODELS_API bool operator == (const HUDOptions&) const;
            DJV_MODELS_API bool operator != (const HUDOptions&) const;
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

            //! \name Image Options
            ///@{

            DJV_MODELS_API const ftk::ImageOptions& getImageOptions() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<ftk::ImageOptions> > observeImageOptions() const;
            DJV_MODELS_API void setImageOptions(const ftk::ImageOptions&);

            ///@}

            //! \name Display Options
            ///@{

            DJV_MODELS_API const tl::DisplayOptions& getDisplayOptions() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::DisplayOptions> > observeDisplayOptions() const;
            DJV_MODELS_API void setDisplayOptions(const tl::DisplayOptions&);

            ///@}

            //! \name Aspect Ratio Options
            ///@{

            DJV_MODELS_API const AspectRatioOptions& getAspectRatioOptions() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<AspectRatioOptions> > observeAspectRatioOptions() const;
            DJV_MODELS_API void setAspectRatioOptions(const AspectRatioOptions&);
            ///@}

            //! \name Background Options
            ///@{

            DJV_MODELS_API const tl::BackgroundOptions& getBackgroundOptions() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::BackgroundOptions> > observeBackgroundOptions() const;
            DJV_MODELS_API void setBackgroundOptions(const tl::BackgroundOptions&);

            ///@}

            //! \name Foreground Options
            ///@{

            DJV_MODELS_API const tl::ForegroundOptions& getForegroundOptions() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::ForegroundOptions> > observeForegroundOptions() const;
            DJV_MODELS_API void setForegroundOptions(const tl::ForegroundOptions&);

            ///@}

            //! \name Color Buffer
            ///@{

            DJV_MODELS_API ftk::gl::TextureType getColorBuffer() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<ftk::gl::TextureType> > observeColorBuffer() const;
            DJV_MODELS_API void setColorBuffer(ftk::gl::TextureType);

            ///@}

            //! \name HUD
            ///@{

            DJV_MODELS_API const HUDOptions& getHUDOptions() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<HUDOptions> > observeHUDOptions() const;
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
