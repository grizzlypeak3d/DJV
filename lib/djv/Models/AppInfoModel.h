// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <ftk/Core/Util.h>

#include <memory>
#include <string>

namespace djv
{
    namespace models
    {
        //! Application information model.
        class DJV_MODELS_API_TYPE AppInfoModel : public std::enable_shared_from_this<AppInfoModel>
        {
            FTK_NON_COPYABLE(AppInfoModel);

        protected:
            AppInfoModel() = default;

        public:
            virtual ~AppInfoModel() = default;

            //! Create a new model.
            DJV_MODELS_API static std::shared_ptr<AppInfoModel> create();

            //! \name Name
            ///@{

            DJV_MODELS_API virtual std::string getFullName() const;
            DJV_MODELS_API virtual std::string getShortName() const;

            //! Get the name of the directory under the user's documents where
            //! the settings and log file are kept. Defaults to the full name;
            //! a suite of applications built on DJV overrides it so that they
            //! share one directory instead of scattering one apiece.
            DJV_MODELS_API virtual std::string getDocsDirName() const;
            
            ///@}

            //! \name Version
            ///@{

            DJV_MODELS_API virtual int getVersionMajor() const;
            DJV_MODELS_API virtual int getVersionMinor() const;
            DJV_MODELS_API virtual int getVersionPatch() const;
            DJV_MODELS_API virtual std::string getVersionDev() const;
            DJV_MODELS_API virtual std::string getVersion() const;

            //! Get the date of the commit the build was made from.
            DJV_MODELS_API virtual std::string getCommitDate() const;

            //! Get the commit the build was made from, marked "-dirty" when
            //! anything was uncommitted.
            DJV_MODELS_API virtual std::string getGitCommit() const;

            ///@}

            //! \name Documentation
            ///@{

            //! Get the documentation installed beside the application, as a
            //! file URL, or empty when there is none.
            //!
            //! Installed rather than on the web, so that what it describes is
            //! the version that is running. A build that was not installed --
            //! a developer's -- has none, and the menu item says so rather
            //! than opening nothing.
            DJV_MODELS_API virtual std::string getDocsURL() const;

            ///@}

            //! \name License
            ///@{

            DJV_MODELS_API virtual std::string getLicense() const;
            DJV_MODELS_API virtual std::string getLicensesURL() const;

            ///@}

            //! \name DJV Studio
            ///@{

            //! Get the web site for DJV Studio, the commercial version.
            //! A brief mention with this link appears in the setup dialog
            //! and the Help menu; returning an empty string hides it,
            //! which is what the commercial applications themselves do.
            DJV_MODELS_API virtual std::string getStudioURL() const;

            ///@}
        };
    }
}
