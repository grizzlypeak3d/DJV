// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/ViewportModel.h>

#include <ftk/UI/Settings.h>
#include <ftk/GL/OffscreenBuffer.h>
#include <ftk/Core/String.h>

namespace djv
{
    namespace models
    {
        TL_ENUM_IMPL(
            HUDItem,
            "File Name",
            "Cache",
            "Time",
            "View Zoom",
            "Color Picker");

        TL_ENUM_IMPL(
            HUDPos,
            "None",
            "Top Left",
            "Top Right",
            "Bottom Left",
            "Bottom Right");

        bool AspectRatioOptions::operator == (const AspectRatioOptions& other) const
        {
            return
                index == other.index &&
                options == other.options;
        }

        bool AspectRatioOptions::operator != (const AspectRatioOptions& other) const
        {
            return !(*this == other);
        }

        bool HUDOptions::operator == (const HUDOptions& other) const
        {
            return
                enabled == other.enabled &&
                items == other.items;
        }

        bool HUDOptions::operator != (const HUDOptions& other) const
        {
            return !(*this == other);
        }

        struct ViewportModel::Private
        {
            std::weak_ptr<ftk::Context> context;
            std::shared_ptr<ftk::Settings> settings;
            std::shared_ptr<ftk::Observable<ftk::ImageOptions> > imageOptions;
            std::shared_ptr<ftk::Observable<tl::DisplayOptions> > displayOptions;
            std::shared_ptr<ftk::Observable<AspectRatioOptions> > aspectRatioOptions;
            std::shared_ptr<ftk::Observable<tl::BackgroundOptions> > backgroundOptions;
            std::shared_ptr<ftk::Observable<tl::ForegroundOptions> > foregroundOptions;
            std::shared_ptr<ftk::Observable<ftk::gl::TextureType> > colorBuffer;
            std::shared_ptr<ftk::Observable<HUDOptions> > hudOptions;
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

            AspectRatioOptions aspectRatioOptions;
            p.settings->getT("/Viewport/AspectRatio.1", aspectRatioOptions);
            p.aspectRatioOptions = ftk::Observable<AspectRatioOptions>::create(aspectRatioOptions);

            tl::BackgroundOptions backgroundOptions;
            p.settings->getT("/Viewport/Background", backgroundOptions);
            p.backgroundOptions = ftk::Observable<tl::BackgroundOptions>::create(
                backgroundOptions);

            tl::ForegroundOptions foregroundOptions;
            p.settings->getT("/Viewport/Foreground.1", foregroundOptions);
            p.foregroundOptions = ftk::Observable<tl::ForegroundOptions>::create(
                foregroundOptions);

            ftk::gl::TextureType colorBuffer = ftk::gl::offscreenColorDefault;
            std::string s = ftk::gl::to_string(colorBuffer);
            p.settings->get("/Viewport/ColorBuffer", s);
            ftk::gl::from_string(s, colorBuffer);
            p.colorBuffer = ftk::Observable<ftk::gl::TextureType>::create(colorBuffer);

            HUDOptions hudOptions;
            hudOptions.items[HUDItem::FileName] = HUDPos::TopLeft;
            hudOptions.items[HUDItem::Cache] = HUDPos::BottomRight;
            hudOptions.items[HUDItem::Time] = HUDPos::TopRight;
            hudOptions.items[HUDItem::ColorPicker] = HUDPos::BottomLeft;
            p.settings->getT("/Viewport/HUD.1", hudOptions);
            p.hudOptions = ftk::Observable<HUDOptions>::create(hudOptions);
        }

        ViewportModel::ViewportModel() :
            _p(new Private)
        {}

        ViewportModel::~ViewportModel()
        {
            FTK_P();
            p.settings->setT("/Viewport/Image", p.imageOptions->get());
            p.settings->setT("/Viewport/Display", p.displayOptions->get());
            p.settings->setT("/Viewport/AspectRatio.1", p.aspectRatioOptions->get());
            p.settings->setT("/Viewport/Background", p.backgroundOptions->get());
            p.settings->setT("/Viewport/Foreground.1", p.foregroundOptions->get());
            p.settings->set("/Viewport/ColorBuffer", ftk::gl::to_string(p.colorBuffer->get()));
            p.settings->setT("/Viewport/HUD.1", p.hudOptions->get());
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
            FTK_P();
            const auto& aspectRatioOptions = p.aspectRatioOptions->get();
            tl::DisplayOptions tmp = value;
            tmp.aspectRatio =
                aspectRatioOptions.index >= 0 &&
                aspectRatioOptions.index < aspectRatioOptions.options.size() ?
                aspectRatioOptions.options[aspectRatioOptions.index] :
                tl::AspectRatioOptions();
            p.displayOptions->setIfChanged(tmp);
        }

        const AspectRatioOptions& ViewportModel::getAspectRatioOptions() const
        {
            return _p->aspectRatioOptions->get();
        }

        std::shared_ptr<ftk::IObservable<AspectRatioOptions> > ViewportModel::observeAspectRatioOptions() const
        {
            return _p->aspectRatioOptions;
        }

        void ViewportModel::setAspectRatioOptions(const AspectRatioOptions& value)
        {
            FTK_P();
            if (p.aspectRatioOptions->setIfChanged(value))
            {
                auto displayOptions = p.displayOptions->get();
                displayOptions.aspectRatio =
                    value.index >= 0 && value.index < value.options.size() ?
                    value.options[value.index] :
                    tl::AspectRatioOptions();
                p.displayOptions->setIfChanged(displayOptions);
            }
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

        ftk::gl::TextureType ViewportModel::getColorBuffer() const
        {
            return _p->colorBuffer->get();
        }

        std::shared_ptr<ftk::IObservable<ftk::gl::TextureType> > ViewportModel::observeColorBuffer() const
        {
            return _p->colorBuffer;
        }

        void ViewportModel::setColorBuffer(ftk::gl::TextureType value)
        {
            _p->colorBuffer->setIfChanged(value);
        }

        const HUDOptions& ViewportModel::getHUDOptions() const
        {
            return _p->hudOptions->get();
        }

        std::shared_ptr<ftk::IObservable<HUDOptions> > ViewportModel::observeHUDOptions() const
        {
            return _p->hudOptions;
        }

        void ViewportModel::setHUDOptions(const HUDOptions& value)
        {
            _p->hudOptions->setIfChanged(value);
        }

        void to_json(nlohmann::json& json, const AspectRatioOptions& in)
        {
            json["Index"] = in.index;
            json["Options"] = in.options;
        }

        void to_json(nlohmann::json& json, const HUDOptions& in)
        {
            json["Enabled"] = in.enabled;
            json["Items"] = in.items;
        }

        void from_json(const nlohmann::json& json, AspectRatioOptions& out)
        {
            json.at("Index").get_to(out.index);
            json.at("Options").get_to(out.options);
        }

        void from_json(const nlohmann::json& json, HUDOptions& out)
        {
            json.at("Enabled").get_to(out.enabled);
            json.at("Items").get_to(out.items);
        }
    }
}
