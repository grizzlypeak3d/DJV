// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Models/ViewportModel.h>

#include <ftk/UI/Settings.h>

namespace djv
{
    namespace app
    {
        struct ViewportModel::Private
        {
            std::weak_ptr<ftk::Context> context;
            std::shared_ptr<ftk::Settings> settings;
            std::shared_ptr<ftk::Observable<ftk::ImageOptions> > imageOptions;
            std::shared_ptr<ftk::Observable<tl::DisplayOptions> > displayOptions;
            std::shared_ptr<ftk::Observable<tl::BackgroundOptions> > backgroundOptions;
            std::shared_ptr<ftk::Observable<tl::ForegroundOptions> > foregroundOptions;
            std::shared_ptr<ftk::Observable<ftk::ImageType> > colorBuffer;
            std::shared_ptr<ftk::Observable<bool> > hud;
        };

        void ViewportModel::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            FTK_P();

            p.context = context;
            p.settings = settings;

            ftk::ImageOptions imageOptions;
            p.settings->getT("/Viewport/Image", imageOptions);
            p.imageOptions = ftk::Observable<ftk::ImageOptions>::create(imageOptions);

            tl::DisplayOptions displayOptions;
            p.settings->getT("/Viewport/Display", displayOptions);
            p.displayOptions = ftk::Observable<tl::DisplayOptions>::create(displayOptions);

            tl::BackgroundOptions backgroundOptions;
            p.settings->getT("/Viewport/Background", backgroundOptions);
            p.backgroundOptions = ftk::Observable<tl::BackgroundOptions>::create(
                backgroundOptions);

            tl::ForegroundOptions foregroundOptions;
            p.settings->getT("/Viewport/Foreground", foregroundOptions);
            p.foregroundOptions = ftk::Observable<tl::ForegroundOptions>::create(
                foregroundOptions);

            ftk::ImageType colorBuffer =
#if defined(FTK_API_GL_4_1)
                ftk::ImageType::RGBA_F32;
#elif defined(FTK_API_GLES_2)
                ftk::ImageType::RGBA_U8;
#endif // FTK_API_GL_4_1
            std::string s = ftk::to_string(colorBuffer);
            p.settings->get("/Viewport/ColorBuffer", s);
            ftk::from_string(s, colorBuffer);
            p.colorBuffer = ftk::Observable<ftk::ImageType>::create(colorBuffer);

            bool hud = false;
            p.settings->get("/Viewport/HUD/Enabled", hud);
            p.hud = ftk::Observable<bool>::create(hud);
        }

        ViewportModel::ViewportModel() :
            _p(new Private)
        {}

        ViewportModel::~ViewportModel()
        {
            FTK_P();
            p.settings->setT("/Viewport/Image", p.imageOptions->get());
            p.settings->setT("/Viewport/Display", p.displayOptions->get());
            p.settings->setT("/Viewport/Background", p.backgroundOptions->get());
            p.settings->setT("/Viewport/Foreground", p.foregroundOptions->get());
            p.settings->set("/Viewport/ColorBuffer", ftk::to_string(p.colorBuffer->get()));
            p.settings->set("/Viewport/HUD/Enabled", p.hud->get());
        }

        std::shared_ptr<ViewportModel> ViewportModel::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            auto out = std::shared_ptr<ViewportModel>(new ViewportModel);
            out->_init(context, settings);
            return out;
        }

        const ftk::ImageOptions& ViewportModel::getImageOptions() const
        {
            return _p->imageOptions->get();
        }

        std::shared_ptr<ftk::IObservable<ftk::ImageOptions> > ViewportModel::observeImageOptions() const
        {
            return _p->imageOptions;
        }

        void ViewportModel::setImageOptions(const ftk::ImageOptions& value)
        {
            _p->imageOptions->setIfChanged(value);
        }

        const tl::DisplayOptions& ViewportModel::getDisplayOptions() const
        {
            return _p->displayOptions->get();
        }

        std::shared_ptr<ftk::IObservable<tl::DisplayOptions> > ViewportModel::observeDisplayOptions() const
        {
            return _p->displayOptions;
        }

        void ViewportModel::setDisplayOptions(const tl::DisplayOptions& value)
        {
            _p->displayOptions->setIfChanged(value);
        }

        const tl::BackgroundOptions& ViewportModel::getBackgroundOptions() const
        {
            return _p->backgroundOptions->get();
        }

        std::shared_ptr<ftk::IObservable<tl::BackgroundOptions> > ViewportModel::observeBackgroundOptions() const
        {
            return _p->backgroundOptions;
        }

        void ViewportModel::setBackgroundOptions(const tl::BackgroundOptions& value)
        {
            _p->settings->setT("/Viewport/Background", value);
            _p->backgroundOptions->setIfChanged(value);
        }

        const tl::ForegroundOptions& ViewportModel::getForegroundOptions() const
        {
            return _p->foregroundOptions->get();
        }

        std::shared_ptr<ftk::IObservable<tl::ForegroundOptions> > ViewportModel::observeForegroundOptions() const
        {
            return _p->foregroundOptions;
        }

        void ViewportModel::setForegroundOptions(const tl::ForegroundOptions& value)
        {
            _p->settings->setT("/Viewport/Foreground", value);
            _p->foregroundOptions->setIfChanged(value);
        }

        ftk::ImageType ViewportModel::getColorBuffer() const
        {
            return _p->colorBuffer->get();
        }

        std::shared_ptr<ftk::IObservable<ftk::ImageType> > ViewportModel::observeColorBuffer() const
        {
            return _p->colorBuffer;
        }

        void ViewportModel::setColorBuffer(ftk::ImageType value)
        {
            _p->colorBuffer->setIfChanged(value);
        }

        bool ViewportModel::getHUD() const
        {
            return _p->hud->get();
        }

        std::shared_ptr<ftk::IObservable<bool> > ViewportModel::observeHUD() const
        {
            return _p->hud;
        }

        void ViewportModel::setHUD(bool value)
        {
            _p->hud->setIfChanged(value);
        }
    }
}
