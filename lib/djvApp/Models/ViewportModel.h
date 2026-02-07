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
    namespace app
    {
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

            //! Get the image options.
            const ftk::ImageOptions& getImageOptions() const;

            //! Observe the image options.
            std::shared_ptr<ftk::IObservable<ftk::ImageOptions> > observeImageOptions() const;

            //! Set the image options.
            void setImageOptions(const ftk::ImageOptions&);

            //! Get the display options.
            const tl::DisplayOptions& getDisplayOptions() const;

            //! Observe the display options.
            std::shared_ptr<ftk::IObservable<tl::DisplayOptions> > observeDisplayOptions() const;

            //! Set the display options.
            void setDisplayOptions(const tl::DisplayOptions&);

            //! Get the background options.
            const tl::BackgroundOptions& getBackgroundOptions() const;

            //! Observe the background options.
            std::shared_ptr<ftk::IObservable<tl::BackgroundOptions> > observeBackgroundOptions() const;

            //! Set the background options.
            void setBackgroundOptions(const tl::BackgroundOptions&);

            //! Get the foreground options.
            const tl::ForegroundOptions& getForegroundOptions() const;

            //! Observe the foreground options.
            std::shared_ptr<ftk::IObservable<tl::ForegroundOptions> > observeForegroundOptions() const;

            //! Set the foreground options.
            void setForegroundOptions(const tl::ForegroundOptions&);

            //! Get the color buffer type.
            ftk::gl::TextureType getColorBuffer() const;

            //! Observe the color buffer type.
            std::shared_ptr<ftk::IObservable<ftk::gl::TextureType> > observeColorBuffer() const;

            //! Set the color buffer type.
            void setColorBuffer(ftk::gl::TextureType);

            //! Get whether the HUD is enabled.
            bool getHUD() const;

            //! Observe whether the HUD is enabled.
            std::shared_ptr<ftk::IObservable<bool> > observeHUD() const;

            //! Set whether the HUD is enabled.
            void setHUD(bool);

        private:
            FTK_PRIVATE();
        };
    }
}
