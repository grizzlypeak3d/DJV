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
                    if (i->is_string())
                    {
                        recent.push_back(ftk::Path(i->get<std::string>()));
                    }
                    else if (i->is_object() && i->contains("Path"))
                    {
                        ftk::Path path(i->at("Path").get<std::string>());
                        if (i->contains("Frames") &&
                            i->at("Frames").is_array() &&
                            i->at("Frames").size() >= 2)
                        {
                            path.setFrames(ftk::RangeI64(
                                i->at("Frames")[0].get<int64_t>(),
                                i->at("Frames")[1].get<int64_t>()));
                        }
                        recent.push_back(path);
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
            // A sequence's range is the path's own state rather than part of
            // its name, so it is written beside the path. Without it an entry
            // could not say whether it was one frame or the sequence it sits
            // in, and reopening it would not give back what it opened.
            //
            // Only a sequence needs this. A file name with a number in it
            // parses back to the one frame it names, which is what a lone
            // frame is, so those are written as they always were and a
            // settings file from before this still reads.
            nlohmann::json json;
            for (const auto& path : getRecent())
            {
                if (path.isSeq())
                {
                    const ftk::RangeI64& frames = path.getFrames().value();
                    nlohmann::json item;
                    item["Path"] = path.get();
                    item["Frames"] = { frames.min(), frames.max() };
                    json.push_back(item);
                }
                else
                {
                    json.push_back(path.get());
                }
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
