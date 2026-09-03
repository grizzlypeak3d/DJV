// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <tlRender/Timeline/ColorOptions.h>

#include <ftk/Core/Image.h>
#include <ftk/Core/Observable.h>

#include <map>
#include <utility>
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
        //! Color model.
        class DJV_MODELS_API_TYPE ColorModel : public std::enable_shared_from_this<ColorModel>
        {
            FTK_NON_COPYABLE(ColorModel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            ColorModel();

        public:
            DJV_MODELS_API ~ColorModel();

            //! Create a new model.
            DJV_MODELS_API static std::shared_ptr<ColorModel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            //! Save the settings.
            DJV_MODELS_API void save();

            //! Get the OpenColorIO options.
            DJV_MODELS_API const tl::OCIOOptions& getOCIOOptions() const;

            //! Observe the OpenColorIO options.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::OCIOOptions> > observeOCIOOptions() const;

            //! Set the OpenColorIO options.
            DJV_MODELS_API void setOCIOOptions(const tl::OCIOOptions&);

            //! Observe the OpenColorIO options with the input color space
            //! resolved for the active file. This is what rendering should
            //! use; observeOCIOOptions() is the settings as the user wrote
            //! them.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::OCIOOptions> > observeResolvedOCIOOptions() const;

            //! Set the files that input color spaces are resolved for --
            //! the active file first, then the compare files -- with the
            //! metadata tags each was read with.
            DJV_MODELS_API void setActiveFiles(const std::vector<std::pair<std::string, ftk::ImageTags> >&);

            //! Resolve an input color space for a file: the extension
            //! assignments, then what the file declares, then the
            //! configuration's file rules. Empty when nothing resolves.
            DJV_MODELS_API std::string resolveInput(
                const std::string& path,
                const ftk::ImageTags& = {}) const;

            //! Observe the input color space resolved for each of the
            //! active files, in their order; empty entries resolve
            //! nothing and use the options' input. For the per item
            //! display options overrides.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<std::vector<std::string> > > observeResolvedInputs() const;

            //! Observe the resolved input color space and where it came
            //! from, for display; e.g., "sRGB (extension)". Empty when
            //! nothing is resolved.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<std::string> > observeResolvedInput() const;

            //! Get the file name extension color spaces.
            DJV_MODELS_API const std::map<std::string, std::string>& getExtColorSpaces() const;

            //! Observe the file name extension color spaces.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<std::map<std::string, std::string> > > observeExtColorSpaces() const;

            //! Set the file name extension color spaces. These take
            //! precedence over the configuration's file rules when the
            //! input color space is resolved.
            DJV_MODELS_API void setExtColorSpaces(const std::map<std::string, std::string>&);

            //! Get the LUT options.
            DJV_MODELS_API const tl::LUTOptions& getLUTOptions() const;

            //! Observe the LUT options.
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<tl::LUTOptions> > observeLUTOptions() const;

            //! Set the LUT options.
            DJV_MODELS_API void setLUTOptions(const tl::LUTOptions&);

        private:
            void _resolvedUpdate();
            tl::OCIOOptions _resolvedOCIOOptions();
            std::string _resolveInput(
                const std::string& path,
                const ftk::ImageTags&,
                std::string* label) const;
            //! declaredUnmatched is set when the file declares its color
            //! -- a colorInteropID, chromaticities -- but the configuration
            //! has no matching space: an explicit declaration that cannot be
            //! honoured must not be papered over by an extension rule.
            //! declaredName is what the file declared, for saying so.
            std::string _declaredColorSpace(
                const std::string& ext,
                const ftk::ImageTags&,
                bool& declaredUnmatched,
                std::string& declaredName,
                std::string& declaredSource) const;
            void _ocioConfigUpdate(const tl::OCIOOptions&);

            FTK_PRIVATE();
        };
    }
}
