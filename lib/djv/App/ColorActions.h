// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/IActions.h>
#include <djv/Models/Export.h>

namespace djv
{
    namespace app
    {
        //! Color actions.
        class DJV_API_TYPE ColorActions : public IActions
        {
            FTK_NON_COPYABLE(ColorActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            ColorActions();

        public:
            DJV_API ~ColorActions();

            DJV_API static std::shared_ptr<ColorActions> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

        private:
            FTK_PRIVATE();
        };
    }
}
