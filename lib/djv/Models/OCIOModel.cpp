// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/OCIOModel.h>

#include <ftk/Core/Context.h>
#include <ftk/Core/OS.h>

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
        bool OCIOModelData::operator == (const OCIOModelData& other) const
        {
            return
                enabled == other.enabled &&
                config == other.config &&
                fileName == other.fileName &&
                inputs == other.inputs &&
                inputIndex == other.inputIndex &&
                displays == other.displays &&
                displayIndex == other.displayIndex &&
                views == other.views &&
                viewIndex == other.viewIndex &&
                looks == other.looks &&
                lookIndex == other.lookIndex;
        }

        bool OCIOModelData::operator != (const OCIOModelData& other) const
        {
            return !(*this == other);
        }

        struct OCIOModel::Private
        {
            std::weak_ptr<ftk::Context> context;
#if defined(TLRENDER_OCIO)
            OCIO_NAMESPACE::ConstConfigRcPtr ocioConfig;
#endif // TLRENDER_OCIO
            std::shared_ptr<ftk::Observable<tl::OCIOOptions> > options;
            std::shared_ptr<ftk::Observable<OCIOModelData> > data;
        };

        void OCIOModel::_init(const std::shared_ptr<ftk::Context>& context)
        {
            FTK_P();

            p.context = context;

            tl::OCIOOptions options;
            _configUpdate(options);
#if defined(TLRENDER_OCIO)
            if (p.ocioConfig)
            {
                const char* display = p.ocioConfig->getDefaultDisplay();
                options.display = display;
                options.view = p.ocioConfig->getDefaultView(display);
            }
#endif // TLRENDER_OCIO
            p.options = ftk::Observable<tl::OCIOOptions>::create(options);

            p.data = ftk::Observable<OCIOModelData>::create(_getData(options));
        }

        OCIOModel::OCIOModel() :
            _p(new Private)
        {}

        OCIOModel::~OCIOModel()
        {}

        std::shared_ptr<OCIOModel> OCIOModel::create(const std::shared_ptr<ftk::Context>& context)
        {
            auto out = std::shared_ptr<OCIOModel>(new OCIOModel);
            out->_init(context);
            return out;
        }

        std::shared_ptr<ftk::IObservable<tl::OCIOOptions> > OCIOModel::observeOptions() const
        {
            return _p->options;
        }

        void OCIOModel::setOptions(const tl::OCIOOptions& value)
        {
            FTK_P();
            const bool configChanged = value.config != p.options->get().config;
            const bool fileNameChanged = value.fileName != p.options->get().fileName;
            auto options = value;
            if (configChanged || fileNameChanged)
            {
                _configUpdate(options);
            }
            p.options->setIfChanged(options);
            p.data->setIfChanged(_getData(options));
        }

        void OCIOModel::setEnabled(bool value)
        {
            FTK_P();
            auto options = p.options->get();
            options.enabled = value;
            p.options->setIfChanged(options);
            p.data->setIfChanged(_getData(options));
        }

        void OCIOModel::setConfig(tl::OCIOConfig value)
        {
            FTK_P();
            const bool changed = value != p.options->get().config;
            tl::OCIOOptions options;
            options.enabled = true;
            options.config = value;
            options.fileName = p.options->get().fileName;
            if (changed)
            {
                _configUpdate(options);
#if defined(TLRENDER_OCIO)
                if (p.ocioConfig)
                {
                    const char* display = p.ocioConfig->getDefaultDisplay();
                    options.display = display;
                    options.view = p.ocioConfig->getDefaultView(display);
                }
#endif // TLRENDER_OCIO
            }
            p.options->setIfChanged(options);
            p.data->setIfChanged(_getData(options));
        }

        void OCIOModel::setFileName(const std::string& fileName)
        {
            FTK_P();
            const bool changed = fileName != p.options->get().fileName;
            auto options = p.options->get();
            options.enabled = true;
            options.fileName = fileName;
            if (changed)
            {
                _configUpdate(options);
            }
            p.options->setIfChanged(options);
            p.data->setIfChanged(_getData(options));
        }

        std::shared_ptr<ftk::IObservable<OCIOModelData> > OCIOModel::observeData() const
        {
            return _p->data;
        }

        void OCIOModel::setInputIndex(size_t value)
        {
            FTK_P();
            auto data = p.data->get();
            if (value >= 0 && value < data.inputs.size())
            {
                auto options = p.options->get();
                options.enabled = true;
                options.input = value > 0 ? data.inputs[value] : std::string();
                data = _getData(options);
                if (data.displays[data.displayIndex] != options.display)
                {
                    options.display = std::string();
                }
                if (data.views[data.viewIndex] != options.view)
                {
                    options.view = std::string();
                }
                if (data.looks[data.lookIndex] != options.look)
                {
                    options.look = std::string();
                }
                p.options->setIfChanged(options);
            }
        }

        void OCIOModel::setDisplayIndex(size_t value)
        {
            FTK_P();
            auto data = p.data->get();
            if (value >= 0 && value < data.displays.size())
            {
                auto options = p.options->get();
                options.enabled = true;
                options.display = value > 0 ? data.displays[value] : std::string();
                data = _getData(options);
                if (data.views[data.viewIndex] != options.view)
                {
                    options.view = std::string();
                }
                if (data.looks[data.lookIndex] != options.look)
                {
                    options.look = std::string();
                }
                p.options->setIfChanged(options);
            }
        }

        void OCIOModel::setViewIndex(size_t value)
        {
            FTK_P();
            auto data = p.data->get();
            if (value >= 0 && value < data.views.size())
            {
                auto options = p.options->get();
                options.enabled = true;
                options.view = value > 0 ? data.views[value] : std::string();
                data = _getData(options);
                if (data.looks[data.lookIndex] != options.look)
                {
                    options.look = std::string();
                }
                p.options->setIfChanged(options);
            }
        }

        void OCIOModel::setLookIndex(size_t value)
        {
            FTK_P();
            const auto& looks = p.data->get().looks;
            if (value >= 0 && value < looks.size())
            {
                auto options = p.options->get();
                options.enabled = true;
                options.look = value > 0 ? looks[value] : std::string();
                p.options->setIfChanged(options);
            }
        }

        OCIOModelData OCIOModel::_getData(const tl::OCIOOptions& options) const
        {
            FTK_P();
            OCIOModelData out;
            out.enabled = options.enabled;
            out.config = options.config;
            out.fileName = options.fileName;
#if defined(TLRENDER_OCIO)
            if (p.ocioConfig)
            {
                out.name = p.ocioConfig->getName();
                if (out.name.empty())
                {
                    const std::filesystem::path path = std::filesystem::u8path(options.fileName);
                    out.name = path.filename().u8string();
                    if ("config.ocio" == out.name)
                    {
                        out.name = path.parent_path().filename().u8string();
                    }
                }
                out.inputs.push_back("None");
                for (int i = 0; i < p.ocioConfig->getNumColorSpaces(); ++i)
                {
                    out.inputs.push_back(p.ocioConfig->getColorSpaceNameByIndex(i));
                }
                auto j = std::find(out.inputs.begin(), out.inputs.end(), options.input);
                if (j != out.inputs.end())
                {
                    out.inputIndex = j - out.inputs.begin();
                }

                out.displays.push_back("None");
                for (int i = 0; i < p.ocioConfig->getNumDisplays(); ++i)
                {
                    out.displays.push_back(p.ocioConfig->getDisplay(i));
                }
                j = std::find(out.displays.begin(), out.displays.end(), options.display);
                if (j != out.displays.end())
                {
                    out.displayIndex = j - out.displays.begin();
                }

                out.views.push_back("None");
                const std::string display = options.display;
                for (int i = 0; i < p.ocioConfig->getNumViews(display.c_str()); ++i)
                {
                    out.views.push_back(p.ocioConfig->getView(display.c_str(), i));
                }
                j = std::find(out.views.begin(), out.views.end(), options.view);
                if (j != out.views.end())
                {
                    out.viewIndex = j - out.views.begin();
                }

                out.looks.push_back("None");
                for (int i = 0; i < p.ocioConfig->getNumLooks(); ++i)
                {
                    out.looks.push_back(p.ocioConfig->getLookNameByIndex(i));
                }
                j = std::find(out.looks.begin(), out.looks.end(), options.look);
                if (j != out.looks.end())
                {
                    out.lookIndex = j - out.looks.begin();
                }
            }
#endif // TLRENDER_OCIO
            return out;
        }

        void OCIOModel::_configUpdate(tl::OCIOOptions& options)
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
                    p.ocioConfig = OCIO::Config::CreateFromFile(options.fileName.c_str());
                    break;
                default: break;
                }
            }
            catch (const std::exception& e)
            {}
#endif // TLRENDER_OCIO
        }
    }
}
