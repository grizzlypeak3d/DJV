// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/DisplaysModel.h>

#include <djv/Models/ColorModel.h>
#include <djv/Models/OCIOModel.h>
#include <djv/Models/SettingsKeys.h>

#include <ftk/UI/Settings.h>

#include <nlohmann/json.hpp>

#include <algorithm>

namespace djv
{
    namespace models
    {
        DisplayColor validateDisplayColor(
            const DisplayColor& chosen,
            const std::vector<std::string>& displays,
            const std::vector<std::string>& views)
        {
            DisplayColor out;
            if (!chosen.display.empty() &&
                std::find(displays.begin(), displays.end(), chosen.display) != displays.end())
            {
                out.display = chosen.display;
                out.view =
                    std::find(views.begin(), views.end(), chosen.view) != views.end() ?
                    chosen.view :
                    (!views.empty() ? views.front() : std::string());
            }
            return out;
        }

        tl::OCIOOptions applyDisplayColor(
            const tl::OCIOOptions& value,
            const DisplayColor& color)
        {
            tl::OCIOOptions out = value;
            if (!color.display.empty())
            {
                out.display = color.display;
                out.view = color.view;
                out.enabled = true;
            }
            return out;
        }

        void to_json(nlohmann::json& json, const DisplayColor& value)
        {
            json["Display"] = value.display;
            json["View"] = value.view;
        }

        void from_json(const nlohmann::json& json, DisplayColor& value)
        {
            value.display = json.value("Display", std::string());
            value.view = json.value("View", std::string());
        }

        void to_json(nlohmann::json& json, const SecondaryDisplay& value)
        {
            json["Color"] = value.color;
            json["Monitor"] = value.monitor;
            json["Open"] = value.open;
        }

        void from_json(const nlohmann::json& json, SecondaryDisplay& value)
        {
            if (json.contains("Color"))
            {
                json.at("Color").get_to(value.color);
            }
            value.monitor = json.value("Monitor", std::string());
            value.open = json.value("Open", false);
        }

        struct DisplaysModel::Private
        {
            std::shared_ptr<ftk::Settings> settings;
            std::shared_ptr<ColorModel> colorModel;

            //! A model of its own to read the configuration with: which
            //! displays it has, and which views the secondary window's
            //! display has, as the color tool's does for the viewport.
            std::shared_ptr<OCIOModel> ocioModel;

            std::shared_ptr<ftk::Observable<SecondaryDisplay> > secondary;
            std::shared_ptr<ftk::Observable<DisplayColor> > secondaryColor;
            std::shared_ptr<ftk::Observable<DisplayColorLists> > secondaryLists;
            std::shared_ptr<ftk::Observable<std::optional<tl::HDR_EOTF> > > secondaryHDRTransfer;

            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
        };

        void DisplaysModel::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings,
            const std::shared_ptr<ColorModel>& colorModel)
        {
            FTK_P();
            p.settings = settings;
            p.colorModel = colorModel;
            p.ocioModel = OCIOModel::create(context);

            SecondaryDisplay secondary;
            if (p.settings)
            {
                p.settings->getT(settingsKeys::displaysSecondary, secondary);
            }
            p.secondary = ftk::Observable<SecondaryDisplay>::create(secondary);
            p.secondaryColor = ftk::Observable<DisplayColor>::create();
            p.secondaryLists = ftk::Observable<DisplayColorLists>::create();
            p.secondaryHDRTransfer = ftk::Observable<std::optional<tl::HDR_EOTF> >::create();

            if (p.colorModel)
            {
                p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                    p.colorModel->observeResolvedOCIOOptions(),
                    [this](const tl::OCIOOptions&)
                    {
                        _secondaryUpdate();
                    });
            }
            _secondaryUpdate();
        }

        DisplaysModel::DisplaysModel() :
            _p(new Private)
        {}

        DisplaysModel::~DisplaysModel()
        {}

        std::shared_ptr<DisplaysModel> DisplaysModel::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings,
            const std::shared_ptr<ColorModel>& colorModel)
        {
            auto out = std::shared_ptr<DisplaysModel>(new DisplaysModel);
            out->_init(context, settings, colorModel);
            return out;
        }

        void DisplaysModel::save()
        {
            FTK_P();
            if (p.settings)
            {
                p.settings->setT(settingsKeys::displaysSecondary, p.secondary->get());
            }
        }

        const SecondaryDisplay& DisplaysModel::getSecondary() const
        {
            return _p->secondary->get();
        }

        std::shared_ptr<ftk::IObservable<SecondaryDisplay> > DisplaysModel::observeSecondary() const
        {
            return _p->secondary;
        }

        void DisplaysModel::setSecondaryColor(const DisplayColor& value)
        {
            FTK_P();
            auto secondary = p.secondary->get();
            secondary.color = value;
            if (p.secondary->setIfChanged(secondary))
            {
                _secondaryUpdate();
            }
        }

        void DisplaysModel::setSecondaryMonitor(const std::string& value)
        {
            FTK_P();
            auto secondary = p.secondary->get();
            secondary.monitor = value;
            p.secondary->setIfChanged(secondary);
        }

        void DisplaysModel::setSecondaryOpen(bool value)
        {
            FTK_P();
            auto secondary = p.secondary->get();
            secondary.open = value;
            p.secondary->setIfChanged(secondary);
        }

        std::shared_ptr<ftk::IObservable<DisplayColor> > DisplaysModel::observeSecondaryColor() const
        {
            return _p->secondaryColor;
        }

        std::shared_ptr<ftk::IObservable<std::optional<tl::HDR_EOTF> > > DisplaysModel::observeSecondaryHDRTransfer() const
        {
            return _p->secondaryHDRTransfer;
        }

        std::shared_ptr<ftk::IObservable<DisplayColorLists> > DisplaysModel::observeSecondaryLists() const
        {
            return _p->secondaryLists;
        }

        void DisplaysModel::_secondaryUpdate()
        {
            FTK_P();
            // The viewport's configuration, asked with the secondary
            // window's display: its views are what that display has. On
            // whether or not the viewport color manages, since choosing a
            // display is how the window is color managed on its own.
            const DisplayColor& chosen = p.secondary->get().color;
            tl::OCIOOptions options = p.colorModel ?
                p.colorModel->observeResolvedOCIOOptions()->get() :
                tl::OCIOOptions();
            if (!chosen.display.empty())
            {
                options.display = chosen.display;
                options.view = chosen.view;
            }
            options.enabled = true;
            p.ocioModel->setOptions(options);
            const OCIOModelData& data = p.ocioModel->observeData()->get();
            p.secondaryLists->setIfChanged({ data.displays, data.views });
            const DisplayColor color = validateDisplayColor(chosen, data.displays, data.views);
            p.secondaryColor->setIfChanged(color);
            // The data's view is the one in use, as the validated color's.
            std::optional<tl::HDR_EOTF> hdrTransfer;
            if (!color.display.empty())
            {
                hdrTransfer = data.hdr ? tl::HDR_EOTF::ST2084 : tl::HDR_EOTF::SDR;
            }
            p.secondaryHDRTransfer->setIfChanged(hdrTransfer);
        }
    }
}
