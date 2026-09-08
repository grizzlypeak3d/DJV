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

            //! Get the version of DJV this was built on, and the commit it
            //! was built from. Not virtual: an application overrides the
            //! pair above to report itself, and these two stay the library's
            //! own. In DJV they are the same values.
            //!
            //! The commit as well as the version, because a development
            //! version names a line rather than a build: every commit
            //! between two releases calls itself the same thing.
            DJV_MODELS_API std::string getLibraryVersion() const;
            DJV_MODELS_API std::string getLibraryCommit() const;

            ///@}

            //! \name Documentation
            ///@{

            //! Get the window title: the name and the version, and for a
            //! development build the commit date and hash as well, so that
            //! two of them can be told apart. A release is identified by its
            //! version, and the rest would be noise.
            DJV_MODELS_API virtual std::string getTitle() const;

            //! Get the documentation installed beside the application, as a
            //! file URL, or empty when there is none.
            //!
            //! Installed rather than on the web, so that what it describes is
            //! the version that is running. A build that was not installed --
            //! a developer's -- has none, and the menu item says so rather
            //! than opening nothing.
            DJV_MODELS_API virtual std::string getDocsURL() const;

            //! Get the page the documentation opens at. "index.html" unless
            //! an application has a section of its own to land in.
            DJV_MODELS_API virtual std::string getDocsPage() const;

            //! Get the directory the documentation search starts from: the
            //! executable's own, unless an application says otherwise. The
            //! Python application does -- its executable is the interpreter,
            //! which lives nowhere near the install.
            DJV_MODELS_API virtual std::string getDocsSearchPath() const;

            ///@}

            //! \name License
            ///@{

            DJV_MODELS_API virtual std::string getLicense() const;

            //! Get the licenses page installed beside the application, as a
            //! file URL, or empty when there is none.
            //!
            //! The notices ship with the build they describe, the way the
            //! documentation does, so what is listed is what is installed.
            //! A link to a repository is a list of what some other build
            //! carries, which for an application built on this library is
            //! the wrong list entirely.
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

        protected:
            //! Get a file in the installed documentation, as a file URL, or
            //! empty when the documentation is not there. The page to open
            //! and the licenses are both found this way.
            DJV_MODELS_API std::string _getDocsFileURL(const std::string&) const;
        };
    }
}
