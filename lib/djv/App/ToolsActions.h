// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/IActions.h>
#include <djv/Models/Export.h>

namespace djv
{
    namespace app
    {
        //! Tools actions.
        class DJV_API_TYPE ToolsActions : public IActions
        {
            FTK_NON_COPYABLE(ToolsActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            ToolsActions();

        public:
            DJV_API ~ToolsActions();

            DJV_API static std::shared_ptr<ToolsActions> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

        private:
            FTK_PRIVATE();
        };
    }
}
