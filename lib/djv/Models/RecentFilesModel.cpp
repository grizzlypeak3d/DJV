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
        };

        void RecentFilesModel::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            ftk::RecentFilesModel::_init(context);
            FTK_P();

            p.settings = settings;

            std::vector<ftk::Path> recent;
            nlohmann::json json;
            if (p.settings->get("/Files/Recent", json))
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
            p.settings->get("/Files/RecentMax", max);
            setRecentMax(max);
        }

        RecentFilesModel::RecentFilesModel() :
            _p(new Private)
        {}

        RecentFilesModel::~RecentFilesModel()
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
            p.settings->set("/Files/Recent", json);
            p.settings->set("/Files/RecentMax", getRecentMax());
        }

        std::shared_ptr<RecentFilesModel> RecentFilesModel::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings)
        {
            auto out = std::shared_ptr<RecentFilesModel>(new RecentFilesModel);
            out->_init(context, settings);
            return out;
        }
    }
}
