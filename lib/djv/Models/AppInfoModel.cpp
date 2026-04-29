// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/AppInfoModel.h>

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
                "POSSIBILITY OF SUCH DAMAGE.\n";
        }

        std::string AppInfoModel::getLicensesURL() const
        {
            return "https://github.com/grizzlypeak3d/DJV/tree/main/etc/Legal";
        }
    }
}
