// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/ColorModel.h>

#include <ftk/UI/Settings.h>
#include <ftk/Core/Path.h>
#include <ftk/Core/String.h>

#if defined(TLRENDER_OCIO)
#include <OpenColorIO/OpenColorIO.h>
#endif // TLRENDER_OCIO

#if defined(TLRENDER_OCIO)
namespace OCIO = OCIO_NAMESPACE;
#endif // TLRENDER_OCIO

namespace djv
{
    namespace models
    {
        struct ColorModel::Private
        {
            std::shared_ptr<ftk::Settings> settings;
            std::shared_ptr<ftk::Observable<tl::OCIOOptions> > ocioOptions;
            std::shared_ptr<ftk::Observable<tl::OCIOOptions> > resolvedOCIOOptions;
            std::shared_ptr<ftk::Observable<std::string> > resolvedInput;
            std::shared_ptr<ftk::Observable<tl::LUTOptions> > lutOptions;
            std::shared_ptr<ftk::Observable<std::map<std::string, std::string> > > extColorSpaces;
            std::string activeFile;
            ftk::ImageTags activeTags;
            // Set beside the resolved options: the input color space and
            // where it came from, for display.
            std::string resolvedInputLabel;
#if defined(TLRENDER_OCIO)
            OCIO_NAMESPACE::ConstConfigRcPtr ocioConfig;
#endif // TLRENDER_OCIO
        };

        void ColorModel::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            FTK_P();

            p.settings = settings;

            tl::OCIOOptions ocioOptions;
            p.settings->getT("/Color/OCIO", ocioOptions);
            p.ocioOptions = ftk::Observable<tl::OCIOOptions>::create(ocioOptions);
            _ocioConfigUpdate(ocioOptions);
            std::map<std::string, std::string> extColorSpaces;
            p.settings->getT("/Color/OCIOExtColorSpaces", extColorSpaces);
            p.extColorSpaces = ftk::Observable<std::map<std::string, std::string> >::create(extColorSpaces);
            p.resolvedOCIOOptions = ftk::Observable<tl::OCIOOptions>::create(
                _resolvedOCIOOptions());
            p.resolvedInput = ftk::Observable<std::string>::create(
                p.resolvedInputLabel);

            tl::LUTOptions lutOptions;
            p.settings->getT("/Color/LUT", lutOptions);
            p.lutOptions = ftk::Observable<tl::LUTOptions>::create(lutOptions);
        }

        ColorModel::ColorModel() :
            _p(new Private)
        {}

        ColorModel::~ColorModel()
        {
            FTK_P();
            p.settings->setT("/Color/OCIO", p.ocioOptions->get());
            p.settings->setT("/Color/OCIOExtColorSpaces", p.extColorSpaces->get());
            p.settings->setT("/Color/LUT", p.lutOptions->get());
        }

        std::shared_ptr<ColorModel> ColorModel::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            auto out = std::shared_ptr<ColorModel>(new ColorModel);
            out->_init(context, settings);
            return out;
        }

        const tl::OCIOOptions& ColorModel::getOCIOOptions() const
        {
            return _p->ocioOptions->get();
        }

        std::shared_ptr<ftk::IObservable<tl::OCIOOptions> > ColorModel::observeOCIOOptions() const
        {
            return _p->ocioOptions;
        }

        void ColorModel::setOCIOOptions(const tl::OCIOOptions& value)
        {
            FTK_P();
            const bool configChanged =
                value.config != p.ocioOptions->get().config ||
                value.fileName != p.ocioOptions->get().fileName;
            p.ocioOptions->setIfChanged(value);
            if (configChanged)
            {
                _ocioConfigUpdate(value);
            }
            _resolvedUpdate();
        }

        std::shared_ptr<ftk::IObservable<tl::OCIOOptions> > ColorModel::observeResolvedOCIOOptions() const
        {
            return _p->resolvedOCIOOptions;
        }

        void ColorModel::setActiveFile(const std::string& value, const ftk::ImageTags& tags)
        {
            FTK_P();
            if (value != p.activeFile || tags != p.activeTags)
            {
                p.activeFile = value;
                p.activeTags = tags;
                _resolvedUpdate();
            }
        }

        const std::map<std::string, std::string>& ColorModel::getExtColorSpaces() const
        {
            return _p->extColorSpaces->get();
        }

        std::shared_ptr<ftk::IObservable<std::map<std::string, std::string> > > ColorModel::observeExtColorSpaces() const
        {
            return _p->extColorSpaces;
        }

        void ColorModel::setExtColorSpaces(const std::map<std::string, std::string>& value)
        {
            FTK_P();
            if (p.extColorSpaces->setIfChanged(value))
            {
                _resolvedUpdate();
            }
        }

        const tl::LUTOptions& ColorModel::getLUTOptions() const
        {
            return _p->lutOptions->get();
        }

        std::shared_ptr<ftk::IObservable<tl::LUTOptions> > ColorModel::observeLUTOptions() const
        {
            return _p->lutOptions;
        }

        void ColorModel::setLUTOptions(const tl::LUTOptions& value)
        {
            _p->lutOptions->setIfChanged(value);
        }

        std::shared_ptr<ftk::IObservable<std::string> > ColorModel::observeResolvedInput() const
        {
            return _p->resolvedInput;
        }

        void ColorModel::_resolvedUpdate()
        {
            FTK_P();
            p.resolvedOCIOOptions->setIfChanged(_resolvedOCIOOptions());
            p.resolvedInput->setIfChanged(p.resolvedInputLabel);
        }

        tl::OCIOOptions ColorModel::_resolvedOCIOOptions()
        {
            FTK_P();
            p.resolvedInputLabel = std::string();
            tl::OCIOOptions out = p.ocioOptions->get();
#if defined(TLRENDER_OCIO)
            // An empty input color space means automatic: it comes from the
            // configuration's file rules for the active file. Only a rule
            // the configuration author wrote is taken; every path matches
            // the default rule, so taking that too would replace "no input
            // transform" with the default rule's space for everyone,
            // whether their configuration has rules or not.
            if (out.enabled &&
                out.input.empty() &&
                !p.activeFile.empty())
            {
                // The user's own extension assignments come before the
                // configuration's rules: they are set from inside DJV, so
                // they are the more deliberate of the two.
                const std::string ext = ftk::toLower(ftk::Path(p.activeFile).getExt());
                const auto& extColorSpaces = p.extColorSpaces->get();
                const auto i = extColorSpaces.find(ext);
                if (i != extColorSpaces.end() && !i->second.empty())
                {
                    out.input = i->second;
                    p.resolvedInputLabel = out.input + " (extension)";
                }
                else if (const std::string declared = _declaredColorSpace();
                    !declared.empty())
                {
                    out.input = declared;
                    p.resolvedInputLabel = out.input + " (file)";
                }
                else if (p.ocioConfig)
                {
                    try
                    {
                        const char* colorSpace =
                            p.ocioConfig->getColorSpaceFromFilepath(p.activeFile.c_str());
                        if (colorSpace &&
                            colorSpace[0] &&
                            !p.ocioConfig->filepathOnlyMatchesDefaultRule(p.activeFile.c_str()))
                        {
                            out.input = colorSpace;
                            p.resolvedInputLabel = out.input + " (file rules)";
                        }
                    }
                    catch (const std::exception&)
                    {}
                }
            }
#endif // TLRENDER_OCIO
            return out;
        }

        std::string ColorModel::_declaredColorSpace() const
        {
            FTK_P();
            std::string out;
#if defined(TLRENDER_OCIO)
            // What the file was flagged with, matched against the color
            // spaces the configuration has. The names tried are the
            // canonical names and aliases the OpenColorIO configurations
            // use, so a configuration that renamed everything and carries
            // no aliases simply does not match, and resolution falls
            // through to the file rules. Only the common video encodings
            // are recognized; camera log material is almost never flagged.
            if (p.ocioConfig)
            {
                std::string primaries;
                std::string transfer;
                if (const auto i = p.activeTags.find("Color Primaries");
                    i != p.activeTags.end())
                {
                    primaries = i->second;
                }
                if (const auto i = p.activeTags.find("Color Transfer");
                    i != p.activeTags.end())
                {
                    transfer = i->second;
                }
                std::vector<std::string> candidates;
                if ("bt709" == primaries && "iec61966-2-1" == transfer)
                {
                    candidates = { "srgb_tx", "sRGB - Texture", "sRGB" };
                }
                else if ("bt709" == primaries && "bt709" == transfer)
                {
                    candidates =
                    {
                        "rec1886_rec709_display",
                        "Rec.1886 Rec.709 - Display",
                        "Rec.709"
                    };
                }
                else if ("bt2020" == primaries && "smpte2084" == transfer)
                {
                    candidates = { "rec2100_pq_display", "Rec.2100-PQ - Display" };
                }
                else if ("bt2020" == primaries && "arib-std-b67" == transfer)
                {
                    candidates = { "rec2100_hlg_display", "Rec.2100-HLG - Display" };
                }
                for (const auto& candidate : candidates)
                {
                    try
                    {
                        if (const auto colorSpace =
                            p.ocioConfig->getColorSpace(candidate.c_str()))
                        {
                            out = colorSpace->getName();
                            break;
                        }
                    }
                    catch (const std::exception&)
                    {}
                }
            }
#endif // TLRENDER_OCIO
            return out;
        }

        void ColorModel::_ocioConfigUpdate(const tl::OCIOOptions& options)
        {
            FTK_P();
#if defined(TLRENDER_OCIO)
            try
            {
                p.ocioConfig.reset();
                switch (options.config)
                {
                case tl::OCIOConfig::BuiltIn:
                    p.ocioConfig = OCIO::Config::CreateFromFile("ocio://default");
                    break;
                case tl::OCIOConfig::EnvVar:
                    p.ocioConfig = OCIO::Config::CreateFromEnv();
                    break;
                case tl::OCIOConfig::File:
                    if (!options.fileName.empty())
                    {
                        p.ocioConfig = OCIO::Config::CreateFromFile(options.fileName.c_str());
                    }
                    break;
                default: break;
                }
            }
            catch (const std::exception&)
            {}
#endif // TLRENDER_OCIO
        }
    }
}
