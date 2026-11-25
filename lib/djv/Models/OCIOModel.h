// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <tlRender/Timeline/ColorOptions.h>

#include <ftk/Core/Observable.h>

namespace ftk
{
    class Context;
}

namespace djv
{
    namespace models
    {
        //! OpenColorIO model data.
        struct DJV_MODELS_API_TYPE OCIOModelData
        {
            //! Whether OpenColorIO is enabled.
            bool enabled = false;
            //! Which configuration is used.
            tl::OCIOConfig config = tl::OCIOConfig::First;
            //! The configuration file, for File.
            std::string fileName;
            //! The configuration's name, or where it has none the file's, or
            //! for a "config.ocio" its directory's.
            std::string name;
            //! The input color spaces. The first is "Automatic", which resolves
            //! each file's from the extension assignments, what the file
            //! declares, and the configuration's file rules.
            std::vector<std::string> inputs;
            //! The input color space in use, an index into the inputs.
            size_t inputIndex = 0;
            //! The configuration's displays.
            std::vector<std::string> displays;
            //! The display in use, an index into the displays.
            size_t displayIndex = 0;
            //! The views of the display in use.
            std::vector<std::string> views;
            //! The view in use, an index into the views.
            size_t viewIndex = 0;
            //! The configuration's looks. The first is "None".
            std::vector<std::string> looks;
            //! The look in use, an index into the looks.
            size_t lookIndex = 0;

            //! Whether the display and view give HDR video: the display's
            //! color space is encoded "hdr-video", as a PQ display's is. The
            //! view is the one in use, the first where the one asked for is
            //! not the display's.
            bool hdr = false;

            bool operator == (const OCIOModelData&) const = default;
        };

        //! Get the OpenColorIO version, or an empty string when OpenColorIO
        //! is not built in.
        DJV_MODELS_API std::string getOCIOVersion();

        //! OpenColorIO model: a configuration's color spaces, displays, views
        //! and looks, and the options chosen from them. The application's color
        //! options are the color model's; a color widget keeps a model of its
        //! own in step with them.
        class DJV_MODELS_API_TYPE OCIOModel : public std::enable_shared_from_this<OCIOModel>
        {
            FTK_NON_COPYABLE(OCIOModel);

        protected:
            void _init(const std::shared_ptr<ftk::Context>&);

            OCIOModel();

        public:
            DJV_MODELS_API ~OCIOModel();

            //! Create a new model.
            DJV_MODELS_API static std::shared_ptr<OCIOModel> create(const std::shared_ptr<ftk::Context>&);

            //! Observe the options.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::OCIOOptions> > observeOptions() const;

            //! Set the options.
            DJV_MODELS_API void setOptions(const tl::OCIOOptions&);

            //! Observe the model data.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<OCIOModelData> > observeData() const;

            //! Set whether the color configuration is enabled.
            DJV_MODELS_API void setEnabled(bool);

            //! Set the color configuration.
            DJV_MODELS_API void setConfig(tl::OCIOConfig);

            //! Set the color configuration file.
            DJV_MODELS_API void setFileName(const std::string& fileName);

            //! Set the input index.
            DJV_MODELS_API void setInputIndex(size_t);

            //! Set the display index.
            DJV_MODELS_API void setDisplayIndex(size_t);

            //! Set the view index.
            DJV_MODELS_API void setViewIndex(size_t);

            //! Set the look index.
            DJV_MODELS_API void setLookIndex(size_t);

        private:
            OCIOModelData _getData(const tl::OCIOOptions&) const;

            void _configUpdate(tl::OCIOOptions&);

            FTK_PRIVATE();
        };
    }
}
