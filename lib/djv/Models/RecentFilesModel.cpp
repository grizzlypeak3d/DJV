// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/RecentFilesModel.h>

#include <djv/Models/SettingsKeys.h>

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

            std::vector<ftk::Path> recent;
            nlohmann::json json;
            if (p.settings->get(settingsKeys::recent(p.settingsGroup), json))
            {
                for (auto i = json.begin(); i != json.end(); ++i)
                {
                    try
                    {
                        ftk::Path path;
                        from_json(*i, path);
                        recent.push_back(path);
                    }
                    catch (const std::exception&)
                    {
                        // A recent file that cannot be read is one fewer
                        // recent file, not a reason to start with none.
                    }
                }
            }
            setRecent(recent);
            size_t max = 10;
            p.settings->get(settingsKeys::recentMax(p.settingsGroup), max);
            setRecentMax(max);
        }

        RecentFilesModel::RecentFilesModel() :
            _p(new Private)
        {}

        RecentFilesModel::~RecentFilesModel()
        {
            save();
        }

        void RecentFilesModel::save()
        {
            FTK_P();
            // The path carries the frames it covers, so that reopening an
            // entry gives back what it opened; a path whose name already says
            // so is written as that name, the way it always was.
            nlohmann::json json;
            for (const auto& path : getRecent())
            {
                nlohmann::json item;
                to_json(item, path);
                json.push_back(item);
            }
            p.settings->set(settingsKeys::recent(p.settingsGroup), json);
            p.settings->set(settingsKeys::recentMax(p.settingsGroup), getRecentMax());
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
