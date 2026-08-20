// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/IActions.h>
#include <djv/Models/Export.h>

namespace djv
{
    namespace app
    {
        //! Audio actions.
        class DJV_API_TYPE AudioActions : public IActions
        {
            FTK_NON_COPYABLE(AudioActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            AudioActions();

        public:
            DJV_API ~AudioActions();

            DJV_API static std::shared_ptr<AudioActions> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

        private:
            FTK_PRIVATE();
        };
    }
}
