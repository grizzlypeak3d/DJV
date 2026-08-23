// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/App/IActions.h>
#include <djv/Models/Export.h>

namespace djv
{
    namespace app
    {
        //! Compare actions.
        class DJV_APP_API_TYPE CompareActions : public IActions
        {
            FTK_NON_COPYABLE(CompareActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            CompareActions();

        public:
            DJV_APP_API ~CompareActions();

            DJV_APP_API static std::shared_ptr<CompareActions> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

        private:
            FTK_PRIVATE();
        };
    }
}
