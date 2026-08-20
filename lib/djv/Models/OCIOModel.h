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
        struct DJV_API_TYPE OCIOModelData
        {
            bool enabled = false;
            tl::OCIOConfig config = tl::OCIOConfig::First;
            std::string fileName;
            std::string name;
            std::vector<std::string> inputs;
            size_t inputIndex = 0;
            std::vector<std::string> displays;
            size_t displayIndex = 0;
            std::vector<std::string> views;
            size_t viewIndex = 0;
            std::vector<std::string> looks;
            size_t lookIndex = 0;

            DJV_API bool operator == (const OCIOModelData&) const;
            DJV_API bool operator != (const OCIOModelData&) const;
        };

        //! OpenColorIO model.
        class DJV_API_TYPE OCIOModel : public std::enable_shared_from_this<OCIOModel>
        {
            FTK_NON_COPYABLE(OCIOModel);

        protected:
            void _init(const std::shared_ptr<ftk::Context>&);

            OCIOModel();

        public:
            DJV_API ~OCIOModel();

            //! Create a new model.
            DJV_API static std::shared_ptr<OCIOModel> create(const std::shared_ptr<ftk::Context>&);

            //! Observe the options.
            DJV_API std::shared_ptr<ftk::IObservable<tl::OCIOOptions> > observeOptions() const;

            //! Set the options.
            DJV_API void setOptions(const tl::OCIOOptions&);

            //! Observe the model data.
            DJV_API std::shared_ptr<ftk::IObservable<OCIOModelData> > observeData() const;

            //! Set whether the color configuration is enabled.
            DJV_API void setEnabled(bool);

            //! Set the color configuration.
            DJV_API void setConfig(tl::OCIOConfig);

            //! Set the color configuration file.
            DJV_API void setFileName(const std::string& fileName);

            //! Set the input index.
            DJV_API void setInputIndex(size_t);

            //! Set the display index.
            DJV_API void setDisplayIndex(size_t);

            //! Set the view index.
            DJV_API void setViewIndex(size_t);

            //! Set the look index.
            DJV_API void setLookIndex(size_t);

        private:
            OCIOModelData _getData(const tl::OCIOOptions&) const;

            void _configUpdate(tl::OCIOOptions&);

            FTK_PRIVATE();
        };
    }
}
