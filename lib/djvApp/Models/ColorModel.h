// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <tlRender/Timeline/ColorOptions.h>

#include <ftk/Core/Observable.h>

namespace ftk
{
    class Context;
    class Settings;
}

namespace djv
{
    namespace app
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

            //! Get the LUT options.
            const tl::LUTOptions& getLUTOptions() const;

            //! Observe the LUT options.
            std::shared_ptr<ftk::IObservable<tl::LUTOptions> > observeLUTOptions() const;

            //! Set the LUT options.
            void setLUTOptions(const tl::LUTOptions&);

        private:
            FTK_PRIVATE();
        };
    }
}
