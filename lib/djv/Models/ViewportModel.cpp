// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/ViewportModel.h>
#include <djv/Models/SettingsKeys.h>

#include <tlRender/UI/Viewport.h>

#include <ftk/UI/Settings.h>
#include <ftk/GL/Init.h>
#include <ftk/GL/OffscreenBuffer.h>
#include <ftk/Core/String.h>

namespace djv
{
    namespace models
    {
        FTK_ENUM_IMPL(
            HUDItem,
            "File Name",
            "Information",
            "Cache",
            "Time",
            "View Zoom",
            "Color Picker",
            "Render");

        FTK_ENUM_IMPL(
            HUDPos,
            "None",
            "Top Left",
            "Top Right",
            "Bottom Left",
            "Bottom Right");

        struct ViewportModel::Private
        {
            std::weak_ptr<ftk::Context> context;
            std::shared_ptr<ftk::Settings> settings;
            std::shared_ptr<ftk::Observable<ftk::ImageOptions> > imageOptions;
            std::shared_ptr<ftk::Observable<tl::DisplayOptions> > displayOptions;
            std::shared_ptr<ftk::Observable<AspectRatioOptions> > aspectRatioOptions;
            std::shared_ptr<ftk::Observable<tl::BackgroundOptions> > backgroundOptions;
            std::shared_ptr<ftk::Observable<tl::ForegroundOptions> > foregroundOptions;
            std::shared_ptr<ftk::Observable<ftk::ImageType> > colorBuffer;
            std::shared_ptr<ftk::Observable<tl::HDR_EOTF> > hdrTransfer;
            std::shared_ptr<ftk::Observable<float> > hdrWhite;
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
            // .1: the minify and magnify filters used to be written to the
            // display options and were never read from here, so a blob from
            // before that says nothing about what the view was set to.
            p.settings->getT(settingsKeys::viewportImage, imageOptions);
            p.imageOptions = ftk::Observable<ftk::ImageOptions>::create(imageOptions);

            tl::DisplayOptions displayOptions;
            p.settings->getT(settingsKeys::viewportDisplay, displayOptions);
            p.displayOptions = ftk::Observable<tl::DisplayOptions>::create(displayOptions);

            AspectRatioOptions aspectRatioOptions;
            p.settings->getT(settingsKeys::viewportAspectRatio, aspectRatioOptions);
            p.aspectRatioOptions = ftk::Observable<AspectRatioOptions>::create(aspectRatioOptions);

            tl::BackgroundOptions backgroundOptions;
            p.settings->getT(settingsKeys::viewportBackground, backgroundOptions);
            p.backgroundOptions = ftk::Observable<tl::BackgroundOptions>::create(
                backgroundOptions);

            tl::ForegroundOptions foregroundOptions;
            p.settings->getT(settingsKeys::viewportForeground, foregroundOptions);
            p.foregroundOptions = ftk::Observable<tl::ForegroundOptions>::create(
                foregroundOptions);

            // Half rather than full float. The viewport is display referred,
            // where half's eleven bits of mantissa are ample and its range
            // past 1.0 and below zero is what a fixed point buffer lacks --
            // and it halves what every buffer costs, 66 MB rather than 132
            // for one at 4K, with several of them live in a comparison.
            // Exporting keeps full float: see ExportWidget, where the
            // picture is written rather than shown.
            // The labels are the ones the renderer's texture type had, so a
            // settings file from before reads as it did.
            ftk::ImageType colorBuffer = tl::ui::getViewportColorBufferDefault();
            std::string s = ftk::to_string(colorBuffer);
            p.settings->get(settingsKeys::viewportColorBuffer, s);
            ftk::from_string(s, colorBuffer);
            p.colorBuffer = ftk::Observable<ftk::ImageType>::create(colorBuffer);

            tl::HDR_EOTF hdrTransfer = tl::HDR_EOTF::SDR;
            s = tl::to_string(hdrTransfer);
            p.settings->get(settingsKeys::viewportHDRTransfer, s);
            tl::from_string(s, hdrTransfer);
            p.hdrTransfer = ftk::Observable<tl::HDR_EOTF>::create(hdrTransfer);
            // 203 nits is the reference white for HDR: a picture's white is
            // then as bright as the user interface's.
            float hdrWhite = 203.F;
            p.settings->get(settingsKeys::viewportHDRWhite, hdrWhite);
            p.hdrWhite = ftk::Observable<float>::create(hdrWhite);

            HUDOptions hudOptions;
            hudOptions.items[HUDItem::FileName] = HUDPos::TopLeft;
            hudOptions.items[HUDItem::Cache] = HUDPos::BottomRight;
            hudOptions.items[HUDItem::Time] = HUDPos::TopRight;
            hudOptions.items[HUDItem::ColorPicker] = HUDPos::BottomLeft;
            p.settings->getT(settingsKeys::viewportHUD, hudOptions);
            p.hudOptions = ftk::Observable<HUDOptions>::create(hudOptions);
        }

        ViewportModel::ViewportModel() :
            _p(new Private)
        {}

        ViewportModel::~ViewportModel()
        {
            save();
        }

        void ViewportModel::save()
        {
            FTK_P();
            p.settings->setT(settingsKeys::viewportImage, p.imageOptions->get());
            p.settings->setT(settingsKeys::viewportDisplay, p.displayOptions->get());
            p.settings->setT(settingsKeys::viewportAspectRatio, p.aspectRatioOptions->get());
            p.settings->setT(settingsKeys::viewportBackground, p.backgroundOptions->get());
            p.settings->setT(settingsKeys::viewportForeground, p.foregroundOptions->get());
            p.settings->set(settingsKeys::viewportColorBuffer, ftk::to_string(p.colorBuffer->get()));
            p.settings->set(settingsKeys::viewportHDRTransfer, tl::to_string(p.hdrTransfer->get()));
            p.settings->set(settingsKeys::viewportHDRWhite, p.hdrWhite->get());
            p.settings->setT(settingsKeys::viewportHUD, p.hudOptions->get());
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
                aspectRatioOptions.index < static_cast<int>(aspectRatioOptions.options.size()) ?
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
                    value.index >= 0 && value.index < static_cast<int>(value.options.size()) ?
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
            _p->settings->setT(settingsKeys::viewportBackground, value);
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
            _p->settings->setT(settingsKeys::viewportForeground, value);
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

        tl::HDR_EOTF ViewportModel::getHDRTransfer() const
        {
            return _p->hdrTransfer->get();
        }

        std::shared_ptr<ftk::IObservable<tl::HDR_EOTF> > ViewportModel::observeHDRTransfer() const
        {
            return _p->hdrTransfer;
        }

        void ViewportModel::setHDRTransfer(tl::HDR_EOTF value)
        {
            _p->hdrTransfer->setIfChanged(value);
        }

        float ViewportModel::getHDRWhite() const
        {
            return _p->hdrWhite->get();
        }

        std::shared_ptr<ftk::IObservable<float> > ViewportModel::observeHDRWhite() const
        {
            return _p->hdrWhite;
        }

        void ViewportModel::setHDRWhite(float value)
        {
            _p->hdrWhite->setIfChanged(value);
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
