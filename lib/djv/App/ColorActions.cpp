// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ColorActions.h>

#include <djv/App/App.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/ViewportModel.h>

#include <djv/App/MainWindow.h>

#include <ftk/Core/Context.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        namespace
        {
            // Which of the color settings are turned on, each with a
            // check box of its own in the color tool.
            struct Enables
            {
                bool ocio     = false;
                bool lut      = false;
                bool color    = false;
                bool levels   = false;
                bool exposure = false;
                bool softClip = false;

                bool any() const
                {
                    return ocio || lut || color || levels || exposure || softClip;
                }
            };

            // A command given no arguments has none to look in.
            bool getBool(const nlohmann::json& args, const std::string& key, bool defaultValue)
            {
                return args.is_object() ? args.value(key, defaultValue) : defaultValue;
            }

            Enables getEnables(const std::shared_ptr<App>& app)
            {
                Enables out;
                out.ocio = app->getColorModel()->getOCIOOptions().enabled;
                out.lut = app->getColorModel()->getLUTOptions().enabled;
                const tl::DisplayOptions& display = app->getViewportModel()->getDisplayOptions();
                out.color = display.color.enabled;
                out.levels = display.levels.enabled;
                out.exposure = display.exposure.enabled;
                out.softClip = display.softClip.enabled;
                return out;
            }

            void setEnables(const std::shared_ptr<App>& app, const Enables& value)
            {
                auto ocio = app->getColorModel()->getOCIOOptions();
                ocio.enabled = value.ocio;
                app->getColorModel()->setOCIOOptions(ocio);
                auto lut = app->getColorModel()->getLUTOptions();
                lut.enabled = value.lut;
                app->getColorModel()->setLUTOptions(lut);
                auto display = app->getViewportModel()->getDisplayOptions();
                display.color.enabled = value.color;
                display.levels.enabled = value.levels;
                display.exposure.enabled = value.exposure;
                display.softClip.enabled = value.softClip;
                app->getViewportModel()->setDisplayOptions(display);
            }
        }

        struct ColorActions::Private
        {
            // What "Color/Enabled" turned off, to turn back on; cleared
            // when anything is turned on some other way, since the
            // settings are no longer the ones that were set aside.
            std::optional<Enables> bypassed;
            bool switching = false;


            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayObserver;
        };

        void ColorActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            IActions::_init(context, app, "Color");
            FTK_P();

            auto appWeak = std::weak_ptr<App>(app);

            // Register the commands.
            // The options can be given with the command, the same as on
            // the command line; this is how a script sets them after
            // "Color/Reset", which the command line options run before.
            // Giving any of them turns OCIO on unless "value" says not.
            _addCheckCommand(
                "OCIO",
                "Toggle whether OCIO is enabled. Also takes \"config\" "
                "(\"Built In\", \"Environment Variable\", or \"File\"), "
                "\"fileName\", \"input\", \"display\", \"view\", and \"look\".",
                [appWeak](const nlohmann::json& args)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getColorModel()->getOCIOOptions();
                        if (args.contains("config"))
                        {
                            const std::string s = args.at("config").get<std::string>();
                            if (!tl::from_string(s, options.config))
                            {
                                throw std::invalid_argument(ftk::Format(
                                    "Unknown OCIO config \"{0}\"; use \"Built In\", "
                                    "\"Environment Variable\", or \"File\"").arg(s));
                            }
                        }
                        if (args.contains("fileName"))
                        {
                            options.config = tl::OCIOConfig::File;
                            options.fileName = args.at("fileName").get<std::string>();
                        }
                        if (args.contains("input"))
                        {
                            options.input = args.at("input").get<std::string>();
                        }
                        if (args.contains("display"))
                        {
                            options.display = args.at("display").get<std::string>();
                        }
                        if (args.contains("view"))
                        {
                            options.view = args.at("view").get<std::string>();
                        }
                        if (args.contains("look"))
                        {
                            options.look = args.at("look").get<std::string>();
                        }
                        options.enabled = getBool(args, "value", true);
                        app->getColorModel()->setOCIOOptions(options);
                    }
                });

            _addCheckCommand(
                "LUT",
                "Toggle whether the LUT is enabled. Also takes \"fileName\" "
                "and \"order\" (\"Post-Config\" or \"Pre-Config\").",
                [appWeak](const nlohmann::json& args)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getColorModel()->getLUTOptions();
                        if (args.contains("fileName"))
                        {
                            options.fileName = args.at("fileName").get<std::string>();
                        }
                        if (args.contains("order"))
                        {
                            const std::string s = args.at("order").get<std::string>();
                            if (!tl::from_string(s, options.order))
                            {
                                throw std::invalid_argument(ftk::Format(
                                    "Unknown LUT order \"{0}\"; use \"Post-Config\" "
                                    "or \"Pre-Config\"").arg(s));
                            }
                        }
                        options.enabled = getBool(args, "value", true);
                        app->getColorModel()->setLUTOptions(options);
                    }
                });

            _addCheckCommand(
                "Enabled",
                "Turn all of the color settings off, to see the image unaltered, "
                "and back on again as they were.",
                [this, appWeak](const nlohmann::json& args)
                {
                    FTK_P();
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        const Enables enables = getEnables(app);
                        // The settings change one at a time, and the
                        // observers must not take the ones part way through
                        // for a change made some other way.
                        if (!value && enables.any())
                        {
                            p.switching = true;
                            setEnables(app, Enables());
                            p.switching = false;
                            p.bypassed = enables;
                        }
                        else if (value && !enables.any() && p.bypassed.has_value())
                        {
                            p.switching = true;
                            setEnables(app, *p.bypassed);
                            p.switching = false;
                            p.bypassed.reset();
                        }
                        _enabledUpdate(app);
                    }
                });

            _addCommand(
                "Reset",
                "Reset the color settings to their defaults. Takes \"ocio\", \"lut\", "
                "\"color\", and \"levels\" to leave a section as it is with false; "
                "e.g., { \"lut\": false }.",
                [this, appWeak](const nlohmann::json& args)
                {
                    FTK_P();
                    if (auto app = appWeak.lock())
                    {
                        const bool ocio = getBool(args, "ocio", true);
                        const bool lut = getBool(args, "lut", true);
                        const bool color = getBool(args, "color", true);
                        const bool levels = getBool(args, "levels", true);
                        p.bypassed.reset();
                        if (ocio)
                        {
                            app->getColorModel()->setOCIOOptions(models::ColorModel::getDefaultOCIOOptions());
                            app->getColorModel()->setExtColorSpaces({});
                        }
                        if (lut)
                        {
                            app->getColorModel()->setLUTOptions(tl::LUTOptions());
                        }
                        // The view options beside them, the channels and
                        // the mirroring, belong to the view tool.
                        auto display = app->getViewportModel()->getDisplayOptions();
                        const tl::DisplayOptions defaults;
                        if (color)
                        {
                            display.color = defaults.color;
                            display.exposure = defaults.exposure;
                            display.softClip = defaults.softClip;
                        }
                        if (levels)
                        {
                            display.levels = defaults.levels;
                        }
                        app->getViewportModel()->setDisplayOptions(display);
                        _enabledUpdate(app);
                    }
                });

            // Create the actions.
            _actions["OCIO"] = ftk::Action::create(
                "Enable OCIO",
                _checkCommand("OCIO"));
            _actions["LUT"] = ftk::Action::create(
                "Enable LUT",
                _checkCommand("LUT"));
            _actions["Enabled"] = ftk::Action::create(
                "Enable Color",
                _checkCommand("Enabled"));
            // Asked from the menu, where it is one click from losing the
            // settings, with the sections to reset; not from the command,
            // which a script runs with nobody there to answer.
            _actions["Reset"] = ftk::Action::create(
                "Reset Color",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        // The dialog belongs to the application: the color
                        // tool offers it as well, which is where somebody
                        // looks for it first (DJV #687).
                        app->colorResetDialog();
                    }
                });

            // Register the shortcuts.
            _addShortcut("OCIO", ftk::KeyShortcut(ftk::Key::N, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("LUT", ftk::KeyShortcut(ftk::Key::K, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("Enabled");
            _addShortcut("Reset");

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

#if !defined(TLRENDER_OCIO)
            _actions["OCIO"]->setEnabled(false);
            _actions["LUT"]->setEnabled(false);
#endif // TLRENDER_OCIO

            p.ocioObserver = ftk::Observer<tl::OCIOOptions>::create(
                app->getColorModel()->observeOCIOOptions(),
                [this, appWeak](const tl::OCIOOptions& value)
                {
                    _actions["OCIO"]->setChecked(value.enabled);
                    if (auto app = appWeak.lock())
                    {
                        _enabledUpdate(app);
                    }
                });

            p.lutObserver = ftk::Observer<tl::LUTOptions>::create(
                app->getColorModel()->observeLUTOptions(),
                [this, appWeak](const tl::LUTOptions& value)
                {
                    _actions["LUT"]->setChecked(value.enabled);
                    if (auto app = appWeak.lock())
                    {
                        _enabledUpdate(app);
                    }
                });

            p.displayObserver = ftk::Observer<tl::DisplayOptions>::create(
                app->getViewportModel()->observeDisplayOptions(),
                [this, appWeak](const tl::DisplayOptions&)
                {
                    if (auto app = appWeak.lock())
                    {
                        _enabledUpdate(app);
                    }
                });
        }

        void ColorActions::_enabledUpdate(const std::shared_ptr<App>& app)
        {
            FTK_P();
            // The observers run while the model is still being set up.
            if (!app->getColorModel() || !app->getViewportModel())
            {
                return;
            }
            const bool any = getEnables(app).any();
            if (p.switching)
            {
                return;
            }
            if (any)
            {
                p.bypassed.reset();
            }
            auto i = _actions.find("Enabled");
            if (i != _actions.end())
            {
                i->second->setChecked(any);
                // With nothing on and nothing set aside there is nothing
                // for it to turn on.
                i->second->setEnabled(any || p.bypassed.has_value());
            }
        }

        ColorActions::ColorActions() :
            _p(new Private)
        {}

        ColorActions::~ColorActions()
        {}

        std::shared_ptr<ColorActions> ColorActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            auto out = std::shared_ptr<ColorActions>(new ColorActions);
            out->_init(context, app);
            return out;
        }
    }
}
