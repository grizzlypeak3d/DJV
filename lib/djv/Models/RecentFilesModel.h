// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/RecentFilesModel.h>

namespace ftk
{
    class Settings;
}

namespace djv
{
    namespace models
    {
        //! Recent files model.
        class RecentFilesModel : public ftk::RecentFilesModel
        {
            FTK_NON_COPYABLE(RecentFilesModel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::string& settingsGroup);

            RecentFilesModel();

        public:
            ~RecentFilesModel();

            //! Create a new model. The settings group is the prefix under which
            //! the recent list is persisted, e.g. "Files" -> "/Files/Recent".
            static std::shared_ptr<RecentFilesModel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::string& settingsGroup = "Files");

        private:
            FTK_PRIVATE();
        };
    }
}
