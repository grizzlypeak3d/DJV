// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/ColorModel.h>

#include <ftk/UI/Settings.h>

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
            std::shared_ptr<ftk::Observable<tl::LUTOptions> > lutOptions;
            std::string activeFile;
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
            p.resolvedOCIOOptions = ftk::Observable<tl::OCIOOptions>::create(
                _resolvedOCIOOptions());

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
            p.resolvedOCIOOptions->setIfChanged(_resolvedOCIOOptions());
        }

        std::shared_ptr<ftk::IObservable<tl::OCIOOptions> > ColorModel::observeResolvedOCIOOptions() const
        {
            return _p->resolvedOCIOOptions;
        }

        void ColorModel::setActiveFile(const std::string& value)
        {
            FTK_P();
            if (value != p.activeFile)
            {
                p.activeFile = value;
                p.resolvedOCIOOptions->setIfChanged(_resolvedOCIOOptions());
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

        tl::OCIOOptions ColorModel::_resolvedOCIOOptions() const
        {
            FTK_P();
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
                p.ocioConfig &&
                !p.activeFile.empty())
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
                    }
                }
                catch (const std::exception&)
                {}
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
