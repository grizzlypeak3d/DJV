// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

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
        class ColorModel : public std::enable_shared_from_this<ColorModel>
        {
            FTK_NON_COPYABLE(ColorModel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            ColorModel();

        public:
            ~ColorModel();

            //! Create a new model.
            static std::shared_ptr<ColorModel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&);

            //! Get the OpenColorIO options.
            const tl::OCIOOptions& getOCIOOptions() const;

            //! Observe the OpenColorIO options.
            std::shared_ptr<ftk::IObservable<tl::OCIOOptions> > observeOCIOOptions() const;

            //! Set the OpenColorIO options.
            void setOCIOOptions(const tl::OCIOOptions&);

            //! Observe the OpenColorIO options with the input color space
            //! resolved for the active file. This is what rendering should
            //! use; observeOCIOOptions() is the settings as the user wrote
            //! them.
            std::shared_ptr<ftk::IObservable<tl::OCIOOptions> > observeResolvedOCIOOptions() const;

            //! Set the files that input color spaces are resolved for --
            //! the active file first, then the compare files -- with the
            //! metadata tags each was read with.
            void setActiveFiles(const std::vector<std::pair<std::string, ftk::ImageTags> >&);

            //! Resolve an input color space for a file: the extension
            //! assignments, then what the file declares, then the
            //! configuration's file rules. Empty when nothing resolves.
            std::string resolveInput(
                const std::string& path,
                const ftk::ImageTags& = {}) const;

            //! Observe the input color space resolved for each of the
            //! active files, in their order; empty entries resolve
            //! nothing and use the options' input. For the per item
            //! display options overrides.
            std::shared_ptr<ftk::IObservable<std::vector<std::string> > > observeResolvedInputs() const;

            //! Observe the resolved input color space and where it came
            //! from, for display; e.g., "sRGB (extension)". Empty when
            //! nothing is resolved.
            std::shared_ptr<ftk::IObservable<std::string> > observeResolvedInput() const;

            //! Get the file name extension color spaces.
            const std::map<std::string, std::string>& getExtColorSpaces() const;

            //! Observe the file name extension color spaces.
            std::shared_ptr<ftk::IObservable<std::map<std::string, std::string> > > observeExtColorSpaces() const;

            //! Set the file name extension color spaces. These take
            //! precedence over the configuration's file rules when the
            //! input color space is resolved.
            void setExtColorSpaces(const std::map<std::string, std::string>&);

            //! Get the LUT options.
            const tl::LUTOptions& getLUTOptions() const;

            //! Observe the LUT options.
            std::shared_ptr<ftk::IObservable<tl::LUTOptions> > observeLUTOptions() const;

            //! Set the LUT options.
            void setLUTOptions(const tl::LUTOptions&);

        private:
            void _resolvedUpdate();
            tl::OCIOOptions _resolvedOCIOOptions();
            std::string _resolveInput(
                const std::string& path,
                const ftk::ImageTags&,
                std::string* label) const;
            std::string _declaredColorSpace(const ftk::ImageTags&) const;
            void _ocioConfigUpdate(const tl::OCIOOptions&);

            FTK_PRIVATE();
        };
    }
}
