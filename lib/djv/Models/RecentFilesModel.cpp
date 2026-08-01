// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/RecentFilesModel.h>

#include <ftk/UI/Settings.h>

namespace djv
{
    namespace models
    {
        struct RecentFilesModel::Private
        {
            std::shared_ptr<ftk::Settings> settings;
            std::string settingsGroup;
        };

        void RecentFilesModel::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings,
            const std::string& settingsGroup)
        {
            ftk::RecentFilesModel::_init(context);
            FTK_P();

            p.settings = settings;
            p.settingsGroup = settingsGroup;

            std::vector<std::filesystem::path> recent;
            nlohmann::json json;
            if (p.settings->get("/" + p.settingsGroup + "/Recent", json))
            {
                for (auto i = json.begin(); i != json.end(); ++i)
                {
                    if (i->is_string())
                    {
                        recent.push_back(std::filesystem::u8path(i->get<std::string>()));
                    }
                }
            }
            setRecent(recent);
            size_t max = 10;
            p.settings->get("/" + p.settingsGroup + "/RecentMax", max);
            setRecentMax(max);
        }

        RecentFilesModel::RecentFilesModel() :
            _p(new Private)
        {}

        RecentFilesModel::~RecentFilesModel()
        {
            FTK_P();
            nlohmann::json json;
            for (const auto& path : getRecent())
            {
                json.push_back(path.u8string());
            }
            p.settings->set("/" + p.settingsGroup + "/Recent", json);
            p.settings->set("/" + p.settingsGroup + "/RecentMax", getRecentMax());
        }

        std::shared_ptr<RecentFilesModel> RecentFilesModel::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings,
            const std::string& settingsGroup)
        {
            auto out = std::shared_ptr<RecentFilesModel>(new RecentFilesModel);
            out->_init(context, settings, settingsGroup);
            return out;
        }
    }
}
