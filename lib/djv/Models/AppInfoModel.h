// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/Core/Util.h>

#include <memory>
#include <string>

namespace djv
{
    namespace models
    {
        //! Application information model.
        class AppInfoModel : public std::enable_shared_from_this<AppInfoModel>
        {
            FTK_NON_COPYABLE(AppInfoModel);

        protected:
            AppInfoModel() = default;

        public:
            virtual ~AppInfoModel() = default;

            //! Create a new model.
            static std::shared_ptr<AppInfoModel> create();
            
            virtual std::string getFullName() const;
            virtual std::string getShortName() const;

            virtual int getVersionMajor() const;
            virtual int getVersionMinor() const;
            virtual int getVersionPatch() const;
            virtual std::string getVersionDev() const;
            virtual std::string getVersion() const;

            virtual std::string getLicense() const;
            virtual std::string getLicensesURL() const;
        };
    }
}
