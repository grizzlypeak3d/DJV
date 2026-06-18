// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

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
        struct AspectRatioOptions
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

            bool operator == (const AspectRatioOptions&) const;
            bool operator != (const AspectRatioOptions&) const;
        };

        //! Viewport model.
        class ViewportModel : public std::enable_shared_from_this<ViewportModel>
        {
            FTK_NON_COPYABLE(ViewportModel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            ViewportModel();

        public:
            ~ViewportModel();

            //! Create a new model.
            static std::shared_ptr<ViewportModel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            //! \name Image Options
            ///@{

            const ftk::ImageOptions& getImageOptions() const;
            std::shared_ptr<ftk::IObservable<ftk::ImageOptions> > observeImageOptions() const;
            void setImageOptions(const ftk::ImageOptions&);

            ///@}

            //! \name Display Options
            ///@{

            const tl::DisplayOptions& getDisplayOptions() const;
            std::shared_ptr<ftk::IObservable<tl::DisplayOptions> > observeDisplayOptions() const;
            void setDisplayOptions(const tl::DisplayOptions&);

            ///@}

            //! \name Aspect Ratio Options
            ///@{

            const AspectRatioOptions& getAspectRatioOptions() const;
            std::shared_ptr<ftk::IObservable<AspectRatioOptions> > observeAspectRatioOptions() const;
            void setAspectRatioOptions(const AspectRatioOptions&);
            ///@}

            //! \name Background Options
            ///@{

            const tl::BackgroundOptions& getBackgroundOptions() const;
            std::shared_ptr<ftk::IObservable<tl::BackgroundOptions> > observeBackgroundOptions() const;
            void setBackgroundOptions(const tl::BackgroundOptions&);

            ///@}

            //! \name Foreground Options
            ///@{

            const tl::ForegroundOptions& getForegroundOptions() const;
            std::shared_ptr<ftk::IObservable<tl::ForegroundOptions> > observeForegroundOptions() const;
            void setForegroundOptions(const tl::ForegroundOptions&);

            ///@}

            //! \name Color Buffer
            ///@{

            ftk::gl::TextureType getColorBuffer() const;
            std::shared_ptr<ftk::IObservable<ftk::gl::TextureType> > observeColorBuffer() const;
            void setColorBuffer(ftk::gl::TextureType);

            ///@}

            //! \name HUD
            ///@{

            bool getHUD() const;
            std::shared_ptr<ftk::IObservable<bool> > observeHUD() const;
            void setHUD(bool);

            ///@}

        private:
            FTK_PRIVATE();
        };

        //! \name Serialize
        ///@{

        void to_json(nlohmann::json&, const AspectRatioOptions&);

        void from_json(const nlohmann::json&, AspectRatioOptions&);

        ///@}
    }
}
