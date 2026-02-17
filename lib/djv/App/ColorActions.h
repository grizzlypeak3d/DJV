// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/IActions.h>

namespace djv
{
    namespace app
    {
        //! Color actions.
        class ColorActions : public IActions
        {
            FTK_NON_COPYABLE(ColorActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            ColorActions();

        public:
            ~ColorActions();

            static std::shared_ptr<ColorActions> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

        private:
            FTK_PRIVATE();
        };
    }
}
