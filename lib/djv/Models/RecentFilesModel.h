// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <djv/Models/SettingsKeys.h>

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
        class DJV_MODELS_API_TYPE RecentFilesModel : public ftk::RecentFilesModel
        {
            FTK_NON_COPYABLE(RecentFilesModel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::string& settingsGroup,
                size_t recentMax);

            RecentFilesModel();

        public:
            DJV_MODELS_API ~RecentFilesModel();

//! Create a new model; "recentMax" is how many are kept unless
            //! another number was saved. The settings group is the prefix under which
            //! the recent list is persisted, e.g. "Files" -> "/Files/Recent".
            DJV_MODELS_API static std::shared_ptr<RecentFilesModel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::string& settingsGroup = settingsKeys::recentFilesGroup,
                size_t recentMax = 10);

            //! Save the settings.
            DJV_MODELS_API void save();

        private:
            FTK_PRIVATE();
        };
    }
}
