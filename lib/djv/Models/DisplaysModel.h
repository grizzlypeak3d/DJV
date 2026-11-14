// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <tlRender/Timeline/ColorOptions.h>
#include <tlRender/Core/HDR.h>

#include <ftk/Core/Observable.h>

#include <nlohmann/json_fwd.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ftk
{
    class Context;
    class Settings;
}

namespace djv
{
    namespace models
    {
        class ColorModel;

        //! The color a place the picture goes is given, beside the viewport:
        //! an OpenColorIO display and view of its own, or, with no display,
        //! the viewport's. An HDR television as a second monitor wants a PQ
        //! display when the desktop does not.
        struct DJV_MODELS_API_TYPE DisplayColor
        {
            std::string display;
            std::string view;

            bool operator == (const DisplayColor&) const = default;
        };

        //! The display and view an output is given, from what was chosen
        //! for it and what the configuration has: the display when the
        //! configuration has it, and the view when that display has it, or
        //! else its first. Nothing, which follows the viewport, when the
        //! display is not chosen or the configuration does not have it.
        DJV_MODELS_API DisplayColor validateDisplayColor(
            const DisplayColor& chosen,
            const std::vector<std::string>& displays,
            const std::vector<std::string>& views);

        //! The viewport's options with an output's display and view in
        //! place of its own. Choosing a display for an output is choosing to
        //! color manage it, whether or not the viewport is, so the options
        //! are turned on; the input is left as it is, to resolve per clip
        //! when none is given.
        DJV_MODELS_API tl::OCIOOptions applyDisplayColor(
            const tl::OCIOOptions&,
            const DisplayColor&);

        //! What the secondary window shows and where.
        struct DJV_MODELS_API_TYPE SecondaryDisplay
        {
            //! Its color, as chosen.
            DisplayColor color;

            //! The monitor it fills, by name, or empty for a window of its
            //! own. A monitor is known by name: the order the system lists
            //! them in changes when one is plugged in.
            std::string monitor;

            //! Whether it was open when the application quit, to open again
            //! at the next start, as the Blackmagic output turns on again.
            bool open = false;

            bool operator == (const SecondaryDisplay&) const = default;
        };

        DJV_MODELS_API void to_json(nlohmann::json&, const DisplayColor&);
        DJV_MODELS_API void from_json(const nlohmann::json&, DisplayColor&);
        DJV_MODELS_API void to_json(nlohmann::json&, const SecondaryDisplay&);
        DJV_MODELS_API void from_json(const nlohmann::json&, SecondaryDisplay&);

        //! The lists an output's color is chosen from: the configuration's
        //! displays, and the views of the display the output has.
        struct DJV_MODELS_API_TYPE DisplayColorLists
        {
            std::vector<std::string> displays;
            std::vector<std::string> views;

            bool operator == (const DisplayColorLists&) const = default;
        };

        //! The places the picture goes beside the main viewport, and the
        //! color each is given. DJV has the secondary window; DJV Studio's
        //! Blackmagic output keeps its own choice in its devices model and
        //! resolves it with the same functions.
        class DJV_MODELS_API_TYPE DisplaysModel : public std::enable_shared_from_this<DisplaysModel>
        {
            FTK_NON_COPYABLE(DisplaysModel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::shared_ptr<ColorModel>&);

            DisplaysModel();

        public:
            DJV_MODELS_API ~DisplaysModel();

            //! Create a new model.
            DJV_MODELS_API static std::shared_ptr<DisplaysModel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::shared_ptr<ColorModel>&);

            //! Save the settings.
            DJV_MODELS_API void save();

            //! Get the secondary window's settings.
            DJV_MODELS_API const SecondaryDisplay& getSecondary() const;

            //! Observe the secondary window's settings.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<SecondaryDisplay> > observeSecondary() const;

            //! Set the secondary window's color.
            DJV_MODELS_API void setSecondaryColor(const DisplayColor&);

            //! Set the monitor the secondary window fills, or empty for a
            //! window.
            DJV_MODELS_API void setSecondaryMonitor(const std::string&);

            //! Set whether the secondary window is open, as the user left
            //! it: opened or closed, not closed by the application quitting.
            DJV_MODELS_API void setSecondaryOpen(bool);

            //! Observe the secondary window's color as the configuration
            //! has it: what the viewport is given (see
            //! validateDisplayColor()).
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<DisplayColor> > observeSecondaryColor() const;

            //! Observe whether the secondary window shows the picture as HDR:
            //! nothing while it follows the viewport, which then follows the
            //! View tool's setting, and with a display of its own, PQ for an
            //! HDR display and SDR otherwise. The secondary window is the
            //! one place an HDR television beside an SDR desktop wants
            //! otherwise.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<std::optional<tl::HDR_EOTF> > > observeSecondaryHDRTransfer() const;

            //! Observe the lists the secondary window's color is chosen
            //! from.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<DisplayColorLists> > observeSecondaryLists() const;

        private:
            void _secondaryUpdate();

            FTK_PRIVATE();
        };
    }
}
