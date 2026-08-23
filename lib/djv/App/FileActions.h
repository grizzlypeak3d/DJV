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
        //! File actions.
        //!
        //! \todo Add actions for opening the next/previous file in the directory.
        class DJV_APP_API_TYPE FileActions : public IActions
        {
            FTK_NON_COPYABLE(FileActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            FileActions();

        public:
            DJV_APP_API ~FileActions();

            DJV_APP_API static std::shared_ptr<FileActions> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

        private:
            FTK_PRIVATE();
        };
    }
}
