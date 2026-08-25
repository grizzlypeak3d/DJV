// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/AppInfoModel.h>

#include <ftk/Core/OS.h>

#include <filesystem>

#include <BuildInfo.h>

#include <djv/Models/Version.h>

#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>

namespace djv
{
    namespace models
    {
        std::shared_ptr<AppInfoModel> AppInfoModel::create()
        {
            return std::shared_ptr<AppInfoModel>(new AppInfoModel);
        }

        std::string AppInfoModel::getFullName() const
        {
            return "DJV";
        }

        std::string AppInfoModel::getShortName() const
        {
            return "djv";
        }

        std::string AppInfoModel::getDocsDirName() const
        {
            return getFullName();
        }

        int AppInfoModel::getVersionMajor() const
        {
            return DJV_VERSION_MAJOR;
        }

        int AppInfoModel::getVersionMinor() const
        {
            return DJV_VERSION_MINOR;
        }

        int AppInfoModel::getVersionPatch() const
        {
            return DJV_VERSION_PATCH;
        }

        std::string AppInfoModel::getVersionDev() const
        {
            return DJV_VERSION_DEV;
        }

        std::string AppInfoModel::getVersion() const
        {
            return DJV_VERSION_FULL;
        }

        std::string AppInfoModel::getCommitDate() const
        {
            return DJV_COMMIT_DATE;
        }

        std::string AppInfoModel::getGitCommit() const
        {
            return DJV_GIT_COMMIT;
        }

        std::string AppInfoModel::getLicense() const
        {
            return
                "Copyright Contributors to the DJV project.\n"
                "\n"
                "Redistribution and use in source and binary forms, with or without\n"
                "modification, are permitted provided that the following conditions are met:\n"
                "\n"
                "* Redistributions of source code must retain the above copyright notice, this\n"
                "  list of conditions, and the following disclaimer.\n"
                "* Redistributions in binary form must reproduce the above copyright notice,\n"
                "  this list of conditions, and the following disclaimer in the documentation\n"
                "  and/or other materials provided with the distribution.\n"
                "* Neither the names of the copyright holders nor the names of any\n"
                "  contributors may be used to endorse or promote products derived from this\n"
                "  software without specific prior written permission.\n"
                "\n"
                "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
                "AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
                "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
                "ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE\n"
                "LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n"
                "CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n"
                "SUBSTITUE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n"
                "INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n"
                "CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n"
                "ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n"
                "POSSIBILITY OF SUCH DAMAGE.";
        }

        std::string AppInfoModel::getDocsURL() const
        {
            const std::filesystem::path exe = ftk::getExePath();
            if (exe.empty())
            {
                return std::string();
            }
            const std::filesystem::path dir = exe.parent_path();
            // The install trees the packages make: bin/ beside share/ on
            // Linux, bin/ beside docs/ in the Windows package, and the
            // "Resources" of a macOS bundle.
            for (const auto& relative : {
                "../share/djv/docs",
                "../docs",
                "../Resources/docs" })
            {
                std::error_code ec;
                const std::filesystem::path index =
                    std::filesystem::weakly_canonical(
                        dir / relative / "index.html", ec);
                if (!ec && std::filesystem::exists(index))
                {
                    return "file://" + index.u8string();
                }
            }
            return std::string();
        }

        std::string AppInfoModel::getLicensesURL() const
        {
            return "https://github.com/grizzlypeak3d/DJV/tree/main/etc/Legal";
        }

        std::string AppInfoModel::getStudioURL() const
        {
            return "https://grizzlypeak3d.com";
        }
    }
}
