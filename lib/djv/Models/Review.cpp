// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/Review.h>

#include <ftk/Core/Format.h>
#include <ftk/Core/OS.h>

#include <opentimelineio/clip.h>
#include <opentimelineio/marker.h>
#include <opentimelineio/stack.h>
#include <opentimelineio/track.h>

#include <algorithm>
#include <atomic>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace djv
{
    namespace models
    {
        namespace
        {
            nlohmann::json timeToJson(const OTIO_NS::RationalTime& value)
            {
                nlohmann::json out;
                out["value"] = value.value();
                out["rate"] = value.rate();
                return out;
            }

            OTIO_NS::RationalTime jsonToTime(const nlohmann::json& json)
            {
                return OTIO_NS::RationalTime(
                    json.at("value").get<double>(),
                    json.at("rate").get<double>());
            }

            nlohmann::json rangeToJson(const OTIO_NS::TimeRange& value)
            {
                nlohmann::json out;
                out["start"] = timeToJson(value.start_time());
                out["duration"] = timeToJson(value.duration());
                return out;
            }

            OTIO_NS::TimeRange jsonToRange(const nlohmann::json& json)
            {
                return OTIO_NS::TimeRange(
                    jsonToTime(json.at("start")),
                    jsonToTime(json.at("duration")));
            }

            //! The only coordinate space this version knows: the pixels of the
            //! source image.
            const std::string imageSpace = "image";

            //! Check a coordinate space, refusing one this version cannot place.
            //!
            //! An absent space means the image space, which was not always
            //! written. Refusing here means the caller keeps the item verbatim
            //! instead of drawing it in the wrong coordinates.
            void requireSpace(const nlohmann::json& json, const std::string& key)
            {
                const auto i = json.find(key);
                if (i == json.end())
                {
                    return;
                }
                if (!i->is_string() || i->get<std::string>() != imageSpace)
                {
                    throw std::runtime_error("unknown " + key + ": " + i->dump());
                }
            }

            //! Read one top-level section, containing a failure to that section.
            //!
            //! The color, compare and interface sections delegate to serializers
            //! that require every key they know, so a key added upstream makes
            //! the whole section throw. Letting that reach the caller would cost
            //! the reader the entire document.
            template<typename T>
            void readSection(
                const nlohmann::json& json,
                const std::string& key,
                T& out,
                Review& review)
            {
                const auto i = json.find(key);
                if (i == json.end())
                {
                    return;
                }
                try
                {
                    i->get_to(out);
                }
                catch (const std::exception&)
                {
                    // A partial read leaves the section half-written.
                    out = T();
                    review.unreadSections.push_back(key);
                }
            }

            //! Read a list section, containing a failure to the offending item.
            //!
            //! Unlike the sections above, these lists are edited during the
            //! session, so they must be written back. An item that cannot be
            //! read is kept verbatim and re-emitted on save rather than dropped:
            //! it belongs to whoever wrote it.
            template<typename T>
            void readList(
                const nlohmann::json& json,
                const std::string& key,
                std::vector<T>& out,
                Review& review)
            {
                const auto i = json.find(key);
                if (i == json.end())
                {
                    return;
                }
                if (!i->is_array())
                {
                    review.unreadSections.push_back(key);
                    return;
                }
                out.clear();
                for (const auto& item : *i)
                {
                    try
                    {
                        out.push_back(item.get<T>());
                    }
                    catch (const std::exception&)
                    {
                        review.unreadItems[key].push_back(item);
                    }
                }
            }
        }

        bool sameTime(
            const std::optional<OTIO_NS::RationalTime>& a,
            const std::optional<OTIO_NS::RationalTime>& b)
        {
            if (a.has_value() != b.has_value())
            {
                return false;
            }
            return !a.has_value() || a->strictly_equal(*b);
        }

        bool sameRange(
            const std::optional<OTIO_NS::TimeRange>& a,
            const std::optional<OTIO_NS::TimeRange>& b)
        {
            if (a.has_value() != b.has_value())
            {
                return false;
            }
            return !a.has_value() || tl::compareExact(*a, *b);
        }

        std::string generateId()
        {
            // 64 random bits plus a monotonic counter: random enough to keep two
            // separately-authored reviews from colliding, and the counter
            // guarantees uniqueness within a single session even if the engine is
            // re-seeded to the same value. Not a formal UUID, which is not needed.
            static std::mt19937_64 engine(std::random_device{}());
            static std::atomic<uint64_t> counter{ 0 };
            const uint64_t r = engine();
            const uint64_t c = counter.fetch_add(1);
            std::stringstream ss;
            ss << std::hex << std::setw(16) << std::setfill('0') << r
               << std::setw(8) << std::setfill('0') << static_cast<uint32_t>(c);
            return ss.str();
        }

        bool reviewVersionSupported(int version)
        {
            return version <= reviewVersion;
        }

        std::string reviewAuthor()
        {
            std::string out;
#if defined(_WIN32)
            ftk::getEnv("USERNAME", out);
#else
            ftk::getEnv("USER", out);
#endif
            return out;
        }

        std::string timestamp()
        {
            const std::time_t t = std::time(nullptr);
            std::tm tm {};
#if defined(_WIN32)
            gmtime_s(&tm, &t);
#else
            gmtime_r(&t, &tm);
#endif
            char buf[32] = {};
            std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
            return std::string(buf);
        }

        void to_json(nlohmann::json& json, const ReviewFile& in)
        {
            json = nlohmann::json::object();
            json["id"] = in.id;
            json["path"] = in.path;
            if (!in.pathAbsolute.empty())
            {
                json["pathAbsolute"] = in.pathAbsolute;
            }
            if (!in.audioPath.empty())
            {
                json["audioPath"] = in.audioPath;
            }
            if (!in.audioPathAbsolute.empty())
            {
                json["audioPathAbsolute"] = in.audioPathAbsolute;
            }
            json["videoLayer"] = in.videoLayer;
            if (in.speed >= 0.0)
            {
                json["speed"] = in.speed;
            }
            if (in.currentTime.has_value())
            {
                json["currentTime"] = timeToJson(in.currentTime.value());
            }
            if (in.inOutRange.has_value())
            {
                json["inOutRange"] = rangeToJson(in.inOutRange.value());
            }
        }

        void from_json(const nlohmann::json& json, ReviewFile& out)
        {
            if (json.contains("id")) json.at("id").get_to(out.id);
            if (json.contains("path")) json.at("path").get_to(out.path);
            if (json.contains("pathAbsolute")) json.at("pathAbsolute").get_to(out.pathAbsolute);
            if (json.contains("audioPath")) json.at("audioPath").get_to(out.audioPath);
            if (json.contains("audioPathAbsolute")) json.at("audioPathAbsolute").get_to(out.audioPathAbsolute);
            if (json.contains("videoLayer")) json.at("videoLayer").get_to(out.videoLayer);
            if (json.contains("speed")) json.at("speed").get_to(out.speed);
            if (json.contains("currentTime")) out.currentTime = jsonToTime(json.at("currentTime"));
            if (json.contains("inOutRange")) out.inOutRange = jsonToRange(json.at("inOutRange"));
        }

        void to_json(nlohmann::json& json, const ReviewCompare& in)
        {
            json = nlohmann::json::object();
            json["aId"] = in.aId;
            json["bIds"] = in.bIds;
            json["options"] = in.options;
            json["time"] = to_string(in.time);
        }

        void from_json(const nlohmann::json& json, ReviewCompare& out)
        {
            if (json.contains("aId")) json.at("aId").get_to(out.aId);
            if (json.contains("bIds")) json.at("bIds").get_to(out.bIds);
            if (json.contains("options")) json.at("options").get_to(out.options);
            if (json.contains("time"))
            {
                std::string s;
                json.at("time").get_to(s);
                from_string(s, out.time);
            }
        }

        void to_json(nlohmann::json& json, const ReviewView& in)
        {
            json = nlohmann::json::object();
            json["frameView"] = in.frameView;
            json["pos"] = in.pos;
            json["zoom"] = in.zoom;
        }

        void from_json(const nlohmann::json& json, ReviewView& out)
        {
            if (json.contains("frameView")) json.at("frameView").get_to(out.frameView);
            if (json.contains("pos")) json.at("pos").get_to(out.pos);
            if (json.contains("zoom")) json.at("zoom").get_to(out.zoom);
        }

        void to_json(nlohmann::json& json, const ReviewColor& in)
        {
            json = nlohmann::json::object();
            json["ocio"] = in.ocio;
            json["lut"] = in.lut;
            json["display"] = in.display;
            json["background"] = in.background;
            json["foreground"] = in.foreground;
            json["aspectRatio"] = in.aspectRatio;
            json["hud"] = in.hud;
        }

        void from_json(const nlohmann::json& json, ReviewColor& out)
        {
            if (json.contains("ocio")) json.at("ocio").get_to(out.ocio);
            if (json.contains("lut")) json.at("lut").get_to(out.lut);
            if (json.contains("display")) json.at("display").get_to(out.display);
            if (json.contains("background")) json.at("background").get_to(out.background);
            if (json.contains("foreground")) json.at("foreground").get_to(out.foreground);
            if (json.contains("aspectRatio")) json.at("aspectRatio").get_to(out.aspectRatio);
            if (json.contains("hud")) json.at("hud").get_to(out.hud);
        }

        void to_json(nlohmann::json& json, const ReviewUI& in)
        {
            json = nlohmann::json::object();
            json["openTools"] = in.openTools;
        }

        void from_json(const nlohmann::json& json, ReviewUI& out)
        {
            if (json.contains("openTools")) json.at("openTools").get_to(out.openTools);
        }

        bool ReviewStroke::operator == (const ReviewStroke& other) const
        {
            return
                color == other.color &&
                width == other.width &&
                points == other.points;
        }

        bool ReviewStroke::operator != (const ReviewStroke& other) const
        {
            return !(*this == other);
        }

        bool ReviewAnnotation::operator == (const ReviewAnnotation& other) const
        {
            return
                id == other.id &&
                sourceId == other.sourceId &&
                sameTime(time, other.time) &&
                author == other.author &&
                created == other.created &&
                strokes == other.strokes;
        }

        bool ReviewAnnotation::operator != (const ReviewAnnotation& other) const
        {
            return !(*this == other);
        }

        void to_json(nlohmann::json& json, const ReviewStroke& in)
        {
            json = nlohmann::json::object();
            json["color"] = in.color;
            json["width"] = in.width;
            json["widthSpace"] = "image";
            // Flat [x, y, x, y, ...] keeps long strokes compact and readable.
            nlohmann::json points = nlohmann::json::array();
            for (const auto& point : in.points)
            {
                points.push_back(point.x);
                points.push_back(point.y);
            }
            json["points"] = points;
        }

        void from_json(const nlohmann::json& json, ReviewStroke& out)
        {
            requireSpace(json, "widthSpace");
            if (json.contains("color")) json.at("color").get_to(out.color);
            if (json.contains("width")) json.at("width").get_to(out.width);
            out.points.clear();
            if (json.contains("points"))
            {
                const auto& points = json.at("points");
                for (size_t i = 0; i + 1 < points.size(); i += 2)
                {
                    out.points.push_back(ftk::V2F(
                        points[i].get<float>(),
                        points[i + 1].get<float>()));
                }
            }
        }

        void to_json(nlohmann::json& json, const ReviewAnnotation& in)
        {
            json = nlohmann::json::object();
            json["id"] = in.id;
            json["sourceId"] = in.sourceId;
            json["space"] = imageSpace;
            if (in.time.has_value())
            {
                json["time"] = timeToJson(in.time.value());
            }
            if (!in.author.empty())
            {
                json["author"] = in.author;
            }
            if (!in.created.empty())
            {
                json["created"] = in.created;
            }
            json["strokes"] = in.strokes;
        }

        void from_json(const nlohmann::json& json, ReviewAnnotation& out)
        {
            requireSpace(json, "space");
            if (json.contains("id")) json.at("id").get_to(out.id);
            if (json.contains("sourceId")) json.at("sourceId").get_to(out.sourceId);
            if (json.contains("time")) out.time = jsonToTime(json.at("time"));
            if (json.contains("author")) json.at("author").get_to(out.author);
            if (json.contains("created")) json.at("created").get_to(out.created);
            if (json.contains("strokes")) json.at("strokes").get_to(out.strokes);
        }

        const ftk::Color4F& reviewMarkerColor()
        {
            // Green: the traditional marker color, and distinct from the
            // drawing default so feedback and strokes read apart.
            static const ftk::Color4F out(0.F, .6F, 0.F, 1.F);
            return out;
        }

        bool ReviewMarker::operator == (const ReviewMarker& other) const
        {
            return
                id == other.id &&
                name == other.name &&
                sameRange(range, other.range) &&
                color == other.color &&
                text == other.text &&
                author == other.author &&
                created == other.created;
        }

        bool ReviewMarker::operator != (const ReviewMarker& other) const
        {
            return !(*this == other);
        }

        void to_json(nlohmann::json& json, const ReviewMarker& in)
        {
            json = nlohmann::json::object();
            json["id"] = in.id;
            if (!in.name.empty())
            {
                json["name"] = in.name;
            }
            if (in.range.has_value())
            {
                json["range"] = rangeToJson(in.range.value());
            }
            json["color"] = in.color;
            if (!in.text.empty())
            {
                json["text"] = in.text;
            }
            if (!in.author.empty())
            {
                json["author"] = in.author;
            }
            if (!in.created.empty())
            {
                json["created"] = in.created;
            }
        }

        void from_json(const nlohmann::json& json, ReviewMarker& out)
        {
            if (json.contains("id")) json.at("id").get_to(out.id);
            if (json.contains("name")) json.at("name").get_to(out.name);
            if (json.contains("range")) out.range = jsonToRange(json.at("range"));
            if (json.contains("color")) json.at("color").get_to(out.color);
            if (json.contains("text")) json.at("text").get_to(out.text);
            if (json.contains("author")) json.at("author").get_to(out.author);
            if (json.contains("created")) json.at("created").get_to(out.created);
        }

        void to_json(nlohmann::json& json, const Review& in)
        {
            // Start from the loaded document so unknown sections survive a
            // load/save round-trip untouched.
            json = in.raw.is_object() ? in.raw : nlohmann::json::object();

            // A section that could not be read is left exactly as it was found.
            // Overwriting it with the defaults the application fell back to
            // would destroy state this version merely failed to understand.
            const auto& unread = in.unreadSections;
            auto write = [&json, &unread](
                const std::string& key, nlohmann::json value)
            {
                if (std::find(unread.begin(), unread.end(), key) == unread.end())
                {
                    json[key] = std::move(value);
                }
            };

            // A list is written even when some of its items were not read: it is
            // edited during the session, so a new marker has to reach the file.
            // Those items are appended back, after the ones this version
            // understands.
            auto writeList = [&in, &write](
                const std::string& key, nlohmann::json value)
            {
                if (in.unreadItems.is_object() && in.unreadItems.contains(key))
                {
                    for (const auto& item : in.unreadItems.at(key))
                    {
                        value.push_back(item);
                    }
                }
                write(key, std::move(value));
            };

            json["djvReview"] = in.version;
            json["app"] = in.app;
            json["created"] = in.created;
            writeList("files", in.files);
            write("compare", in.compare);
            write("view", in.view);
            write("color", in.color);
            write("ui", in.ui);
            writeList("annotations", in.annotations);
            writeList("markers", in.markers);
            // The development-era sections were lifted into the markers on
            // load; left in the raw copy they would come back as duplicates
            // the next time the document is read.
            json.erase("notes");
            json.erase("ranges");
        }

        void from_json(const nlohmann::json& json, Review& out)
        {
            out.raw = json;

            // The version first: a caller that finds one it does not know must
            // refuse the document before trusting anything below. See
            // docs/review-format.md.
            if (json.contains("djvReview")) json.at("djvReview").get_to(out.version);
            if (json.contains("app")) json.at("app").get_to(out.app);
            if (json.contains("created")) json.at("created").get_to(out.created);

            // Every section below is read on its own. The annotations and the
            // markers are the part of a review that exists nowhere else, and one
            // stale section elsewhere must not be allowed to cost them.
            readList(json, "files", out.files, out);
            readSection(json, "compare", out.compare, out);
            readSection(json, "view", out.view, out);
            readSection(json, "color", out.color, out);
            readSection(json, "ui", out.ui, out);
            readList(json, "annotations", out.annotations, out);
            readList(json, "markers", out.markers, out);

            // Courtesy read of the development-era shape: the "notes" and
            // "ranges" sections written between August 2026 and the marker
            // unification lift into markers on load, for one development
            // cycle. Format version 1 never shipped with them.
            if (json.contains("notes") && json.at("notes").is_array())
            {
                for (const auto& item : json.at("notes"))
                {
                    try
                    {
                        ReviewMarker marker;
                        if (item.contains("id")) item.at("id").get_to(marker.id);
                        if (item.contains("time"))
                        {
                            const OTIO_NS::RationalTime time =
                                jsonToTime(item.at("time"));
                            marker.range = OTIO_NS::TimeRange(
                                time,
                                OTIO_NS::RationalTime(1.0, time.rate()));
                        }
                        if (item.contains("created")) item.at("created").get_to(marker.created);
                        if (item.contains("author")) item.at("author").get_to(marker.author);
                        if (item.contains("text")) item.at("text").get_to(marker.text);
                        out.markers.push_back(marker);
                    }
                    catch (const std::exception&)
                    {}
                }
            }
            if (json.contains("ranges") && json.at("ranges").is_array())
            {
                for (const auto& item : json.at("ranges"))
                {
                    try
                    {
                        ReviewMarker marker;
                        if (item.contains("id")) item.at("id").get_to(marker.id);
                        if (item.contains("name")) item.at("name").get_to(marker.name);
                        if (item.contains("range")) marker.range = jsonToRange(item.at("range"));
                        out.markers.push_back(marker);
                    }
                    catch (const std::exception&)
                    {}
                }
            }
        }

        const std::string& reviewExtension()
        {
            static const std::string out = ".djvr";
            return out;
        }

        namespace
        {
            ftk::Color4F fromOTIOColor(const std::optional<OTIO_NS::Color>& value)
            {
                // OTIO gives markers green by default, the same default the
                // review markers use.
                return value.has_value() ?
                    ftk::Color4F(
                        static_cast<float>(value->r()),
                        static_cast<float>(value->g()),
                        static_cast<float>(value->b()),
                        static_cast<float>(value->a())) :
                    reviewMarkerColor();
            }

            std::string getMeta(
                const OTIO_NS::AnyDictionary& dict,
                const std::string& key)
            {
                std::string out;
                const auto i = dict.find(key);
                if (i != dict.end() && i->second.type() == typeid(std::string))
                {
                    out = std::any_cast<std::string>(i->second);
                }
                return out;
            }

            void markerFromOTIO(
                const OTIO_NS::SerializableObject::Retainer<OTIO_NS::Marker>& marker,
                const std::optional<OTIO_NS::TimeRange>& range,
                std::vector<ReviewMarker>& out)
            {
                ReviewMarker m;
                m.name = marker->name();
                m.range = range;
                m.color = fromOTIOColor(marker->color());
                m.text = marker->comment();
                // A marker DJV wrote comes back with its identity and
                // attribution; a foreign marker gets a fresh identity and
                // no false attribution.
                const auto& metadata = marker->metadata();
                const auto i = metadata.find("djv");
                if (i != metadata.end() &&
                    i->second.type() == typeid(OTIO_NS::AnyDictionary))
                {
                    const auto djv =
                        std::any_cast<OTIO_NS::AnyDictionary>(i->second);
                    m.id = getMeta(djv, "id");
                    m.author = getMeta(djv, "author");
                    m.created = getMeta(djv, "created");
                    if (m.text.empty())
                    {
                        m.text = getMeta(djv, "text");
                    }
                    const auto j = djv.find("rangeless");
                    if (j != djv.end() &&
                        j->second.type() == typeid(bool) &&
                        std::any_cast<bool>(j->second))
                    {
                        m.range.reset();
                    }
                }
                if (m.id.empty())
                {
                    m.id = generateId();
                }
                out.push_back(m);
            }
        }

        std::vector<ReviewMarker> reviewMarkersFromTimeline(
            const OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline>& timeline)
        {
            std::vector<ReviewMarker> out;
            const auto stack = timeline->tracks();
            for (const auto& marker : stack->markers())
            {
                markerFromOTIO(marker, marker->marked_range(), out);
            }
            for (const auto& child : stack->children())
            {
                auto track = OTIO_NS::dynamic_retainer_cast<OTIO_NS::Track>(child);
                if (!track)
                {
                    continue;
                }
                for (const auto& marker : track->markers())
                {
                    markerFromOTIO(marker, marker->marked_range(), out);
                }
                for (const auto& item : track->children())
                {
                    auto clip = OTIO_NS::dynamic_retainer_cast<OTIO_NS::Item>(item);
                    if (!clip)
                    {
                        continue;
                    }
                    for (const auto& marker : clip->markers())
                    {
                        // Clip markers are in the clip's own time; the
                        // review's clock is the timeline's.
                        OTIO_NS::ErrorStatus errorStatus;
                        const OTIO_NS::TimeRange transformed =
                            clip->transformed_time_range(
                                marker->marked_range(),
                                stack,
                                &errorStatus);
                        if (!OTIO_NS::is_error(errorStatus))
                        {
                            markerFromOTIO(marker, transformed, out);
                        }
                    }
                }
            }
            return out;
        }

        void reviewMarkersToTimeline(
            const std::vector<ReviewMarker>& markers,
            const OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline>& timeline)
        {
            const auto stack = timeline->tracks();
            const double rate = timeline->duration().rate();
            for (const auto& marker : markers)
            {
                OTIO_NS::AnyDictionary djv;
                djv["id"] = marker.id;
                if (!marker.author.empty())
                {
                    djv["author"] = marker.author;
                }
                if (!marker.created.empty())
                {
                    djv["created"] = marker.created;
                }
                if (!marker.range.has_value())
                {
                    djv["rangeless"] = true;
                }
                OTIO_NS::AnyDictionary metadata;
                metadata["djv"] = djv;
                auto otioMarker =
                    OTIO_NS::SerializableObject::Retainer<OTIO_NS::Marker>(
                        new OTIO_NS::Marker(
                            marker.name,
                            marker.range.has_value() ?
                                *marker.range :
                                OTIO_NS::TimeRange(
                                    OTIO_NS::RationalTime(0.0, rate),
                                    OTIO_NS::RationalTime(0.0, rate)),
                            OTIO_NS::Color(
                                marker.color.r,
                                marker.color.g,
                                marker.color.b,
                                marker.color.a),
                            metadata,
                            marker.text));
                stack->markers().push_back(otioMarker);
            }
        }

        Review reviewOpen(const std::string& fileName)
        {
            std::ifstream f(std::filesystem::u8path(fileName));
            if (!f.is_open())
            {
                throw std::runtime_error(ftk::Format(
                    "Cannot open review: {0}").arg(fileName));
            }
            Review out;
            try
            {
                nlohmann::json json;
                f >> json;
                out = json.get<Review>();
            }
            catch (const std::exception& e)
            {
                throw std::runtime_error(ftk::Format(
                    "Cannot read review \"{0}\": {1}").
                    arg(fileName).arg(e.what()));
            }
            if (!reviewVersionSupported(out.version))
            {
                throw std::runtime_error(ftk::Format(
                    "Cannot read review \"{0}\": it is format version {1}, "
                    "and this build of DJV reads up to version {2}. Open it "
                    "with a newer DJV.").
                    arg(fileName).
                    arg(out.version).
                    arg(reviewVersion));
            }
            return out;
        }

        void reviewSave(const std::string& fileName, Review& review)
        {
            nlohmann::json json = review;
            std::ofstream f(std::filesystem::u8path(fileName));
            if (!f.is_open())
            {
                throw std::runtime_error(ftk::Format(
                    "Cannot save review: {0}").arg(fileName));
            }
            f << std::setw(4) << json << std::endl;
            review.raw = json;
        }

        std::string reviewRelativePath(
            const std::string& path,
            const std::filesystem::path& base)
        {
            if (path.empty() || base.empty())
            {
                return path;
            }
            std::error_code ec;
            const std::filesystem::path rel = std::filesystem::relative(
                std::filesystem::u8path(path), base, ec);
            if (ec || rel.empty())
            {
                return path;
            }
            return rel.generic_u8string();
        }

        std::string reviewGenericPath(const std::string& path)
        {
            if (path.empty())
            {
                return path;
            }
            return std::filesystem::u8path(path).generic_u8string();
        }

        namespace
        {
            //! Does the file exist on disk? A single file is checked exactly; an
            //! image sequence -- whose padded/pattern path is not a literal file
            //! -- is considered present when its directory holds a matching frame.
            bool reviewFilePresent(
                const std::filesystem::path& p,
                const ftk::PathOptions& pathOptions)
            {
                std::error_code ec;
                if (std::filesystem::exists(p, ec))
                {
                    return true;
                }
                const ftk::Path ftkPath(p.u8string(), pathOptions);
                if (ftkPath.getNum().empty())
                {
                    // Single file: genuinely missing.
                    return false;
                }
                const std::filesystem::path dir = std::filesystem::u8path(ftkPath.getDir());
                if (!std::filesystem::exists(dir, ec))
                {
                    return false;
                }
                const std::string base = ftkPath.getBase();
                const std::string ext = ftkPath.getExt();
                for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
                {
                    const std::string name = entry.path().filename().u8string();
                    if (name.size() >= base.size() + ext.size() &&
                        0 == name.compare(0, base.size(), base) &&
                        0 == name.compare(name.size() - ext.size(), ext.size(), ext))
                    {
                        return true;
                    }
                }
                return false;
            }
        }

        std::filesystem::path resolveReviewPath(
            const std::string& relative,
            const std::string& absolute,
            const std::filesystem::path& base,
            const std::filesystem::path& substituteRoot,
            const ftk::PathOptions& pathOptions,
            bool& exists)
        {
            if (!relative.empty())
            {
                const std::filesystem::path rel =
                    (base / std::filesystem::u8path(relative)).lexically_normal();
                if (reviewFilePresent(rel, pathOptions))
                {
                    exists = true;
                    return rel;
                }
            }
            if (!absolute.empty())
            {
                const std::filesystem::path abs =
                    std::filesystem::u8path(absolute);
                if (reviewFilePresent(abs, pathOptions))
                {
                    exists = true;
                    return abs;
                }
            }
            if (!substituteRoot.empty())
            {
                std::filesystem::path fileName;
                if (!relative.empty())
                {
                    fileName = std::filesystem::u8path(relative).filename();
                }
                else if (!absolute.empty())
                {
                    fileName = std::filesystem::u8path(absolute).filename();
                }
                if (!fileName.empty())
                {
                    const std::filesystem::path candidate = substituteRoot / fileName;
                    if (reviewFilePresent(candidate, pathOptions))
                    {
                        exists = true;
                        return candidate;
                    }
                }
            }
            exists = false;
            if (!relative.empty())
            {
                return (base / std::filesystem::u8path(relative)).lexically_normal();
            }
            return std::filesystem::u8path(absolute);
        }
    }
}
