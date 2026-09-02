// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/ColorModel.h>

#include <ftk/UI/Settings.h>
#include <ftk/Core/Path.h>
#include <ftk/Core/String.h>

#include <cmath>
#include <sstream>

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
            std::vector<std::pair<std::string, ftk::ImageTags> > activeFiles;
            std::shared_ptr<ftk::Observable<std::vector<std::string> > > resolvedInputs;
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
            p.resolvedInputs = ftk::Observable<std::vector<std::string> >::create(
                std::vector<std::string>());

            tl::LUTOptions lutOptions;
            p.settings->getT("/Color/LUT", lutOptions);
            p.lutOptions = ftk::Observable<tl::LUTOptions>::create(lutOptions);
        }

        ColorModel::ColorModel() :
            _p(new Private)
        {}

        ColorModel::~ColorModel()
        {
            save();
        }

        void ColorModel::save()
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

        void ColorModel::setActiveFiles(
            const std::vector<std::pair<std::string, ftk::ImageTags> >& value)
        {
            FTK_P();
            if (value != p.activeFiles)
            {
                p.activeFiles = value;
                _resolvedUpdate();
            }
        }

        std::string ColorModel::resolveInput(
            const std::string& path,
            const ftk::ImageTags& tags) const
        {
            return _resolveInput(path, tags, nullptr);
        }

        std::shared_ptr<ftk::IObservable<std::vector<std::string> > > ColorModel::observeResolvedInputs() const
        {
            return _p->resolvedInputs;
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

            // One resolution per active file, for the per item display
            // options; all empty when the user chose an input themselves.
            const tl::OCIOOptions& options = p.ocioOptions->get();
            std::vector<std::string> inputs(p.activeFiles.size());
            if (options.enabled && options.input.empty())
            {
                for (size_t i = 0; i < p.activeFiles.size(); ++i)
                {
                    inputs[i] = _resolveInput(
                        p.activeFiles[i].first,
                        p.activeFiles[i].second,
                        nullptr);
                }
            }
            p.resolvedInputs->setIfChanged(inputs);
        }

        tl::OCIOOptions ColorModel::_resolvedOCIOOptions()
        {
            FTK_P();
            p.resolvedInputLabel = std::string();
            tl::OCIOOptions out = p.ocioOptions->get();
            // An empty input color space means automatic: it is resolved
            // for the active file.
            if (out.enabled &&
                out.input.empty() &&
                !p.activeFiles.empty() &&
                !p.activeFiles[0].first.empty())
            {
                std::string label;
                const std::string input = _resolveInput(
                    p.activeFiles[0].first,
                    p.activeFiles[0].second,
                    &label);
                if (!input.empty())
                {
                    out.input = input;
                    p.resolvedInputLabel = label;
                }
            }
            return out;
        }

        std::string ColorModel::_resolveInput(
            const std::string& path,
            const ftk::ImageTags& tags,
            std::string* label) const
        {
            FTK_P();
            std::string out;
#if defined(TLRENDER_OCIO)
            std::string source;

            // A timeline is a container of media in whatever color spaces
            // they each are, so it has no input color space of its own; the
            // clips resolve individually in the render. Resolving the
            // container would paint every clip with the first clip's space,
            // since that is where a timeline's metadata is borrowed from.
            const std::string ext = ftk::toLower(ftk::Path(path).getExt());
            if (".otio" == ext || ".otioz" == ext)
            {
                return out;
            }

            // The user's own extension assignments come before what the
            // file says and before the configuration's rules: they are set
            // from inside DJV, so they are the most deliberate of the
            // three.
            const auto& extColorSpaces = p.extColorSpaces->get();
            const auto i = extColorSpaces.find(ext);
            if (i != extColorSpaces.end() && !i->second.empty())
            {
                out = i->second;
                source = "extension";
            }
            else if (bool declaredUnmatched = false; true)
            {
                const std::string declared =
                    _declaredColorSpace(tags, declaredUnmatched);
                if (!declared.empty())
                {
                    out = declared;
                    source = "file";
                }
                else if (declaredUnmatched)
                {
                    // The file said what it is and the configuration cannot
                    // name it. Guessing from the extension rules here would
                    // dress a wrong answer as a resolution -- an XYZ or
                    // E-Gamut EXR shown as ACES2065-1 -- so nothing resolves
                    // and the image is shown unmanaged.
                    return out;
                }
                else if (p.ocioConfig)
                {
                // Only a rule the configuration author wrote is taken;
                // every path matches the default rule, so taking that too
                // would replace "no input transform" with the default
                // rule's space for everyone, whether their configuration
                // has rules or not.
                try
                {
                    const char* colorSpace =
                        p.ocioConfig->getColorSpaceFromFilepath(path.c_str());
                    if (colorSpace &&
                        colorSpace[0] &&
                        !p.ocioConfig->filepathOnlyMatchesDefaultRule(path.c_str()))
                    {
                        out = colorSpace;
                        source = "file rules";
                    }
                }
                catch (const std::exception&)
                {}
                }
            }
            if (label && !out.empty())
            {
                *label = out + " (" + source + ")";
            }
#endif // TLRENDER_OCIO
            return out;
        }

        std::string ColorModel::_declaredColorSpace(
            const ftk::ImageTags& tags,
            bool& declaredUnmatched) const
        {
            FTK_P();
            std::string out;
            declaredUnmatched = false;
#if defined(TLRENDER_OCIO)
            // What the file was flagged with, matched against the color
            // spaces the configuration has. The names tried are the
            // canonical names and aliases the OpenColorIO configurations
            // use, so a configuration that renamed everything and carries
            // no aliases simply does not match. Only the common video
            // encodings are recognized; camera log material is almost never
            // flagged.
            if (p.ocioConfig)
            {
                std::vector<std::string> candidates;
                bool declared = false;

                // The OpenEXR 3.4 colorInteropID attribute is the file
                // saying its color space by the color interop forum's
                // names, which the configurations carry as aliases -- the
                // most direct declaration there is, so it comes first.
                if (const auto i = tags.find("colorInteropID");
                    i != tags.end() && !i->second.empty())
                {
                    candidates.push_back(i->second);
                    declared = true;
                }

                // EXR files carry their primaries in the header, exact
                // for the standard sets and measured for everything else.
                // The tolerance is generous: primaries near a standard set
                // are that set within a small error, while falling through
                // to a configuration rule written for the extension risks
                // a gross one -- a camera characterization near Rec.709
                // shown as ACES2065-1 is unrecognizable. The standard sets
                // are about 0.04 apart at their closest, so the tolerance
                // cannot confuse two of them.
                if (const auto i = tags.find("Chromaticities");
                    i != tags.end() && candidates.empty())
                {
                    declared = true;
                    float c[8] = { 0.F };
                    std::stringstream ss(i->second);
                    for (size_t j = 0; j < 8; ++j)
                    {
                        ss >> c[j];
                    }
                    struct Primaries
                    {
                        float c[8];
                        std::vector<std::string> candidates;
                    };
                    const std::vector<Primaries> known =
                    {
                        { { .64F, .33F, .3F, .6F, .15F, .06F, .3127F, .329F },
                            { "lin_rec709", "lin_srgb", "Linear Rec.709 (sRGB)",
                              "Linear Rec.709" } },
                        { { .68F, .32F, .265F, .69F, .15F, .06F, .3127F, .329F },
                            { "lin_p3d65", "Linear P3-D65" } },
                        { { .708F, .292F, .17F, .797F, .131F, .046F, .3127F, .329F },
                            { "lin_rec2020", "Linear Rec.2020" } },
                        { { .7347F, .2653F, 0.F, 1.F, .0001F, -.077F, .32168F, .33767F },
                            { "aces2065_1", "ACES2065-1" } },
                        { { .713F, .293F, .165F, .83F, .128F, .044F, .32168F, .33767F },
                            { "acescg", "ACEScg" } }
                    };
                    for (const auto& k : known)
                    {
                        bool match = true;
                        for (size_t j = 0; j < 8 && match; ++j)
                        {
                            match = std::abs(c[j] - k.c[j]) < .02F;
                        }
                        if (match)
                        {
                            candidates = k.candidates;
                            break;
                        }
                    }
                }

                std::string primaries;
                std::string transfer;
                if (const auto i = tags.find("Color Primaries");
                    i != tags.end())
                {
                    primaries = i->second;
                }
                if (const auto i = tags.find("Color Transfer");
                    i != tags.end())
                {
                    transfer = i->second;
                }
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
                declaredUnmatched = declared && out.empty();
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
