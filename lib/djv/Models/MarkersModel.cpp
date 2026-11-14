// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/MarkersModel.h>

#include <algorithm>

namespace djv
{
    namespace models
    {
        OTIO_NS::TimeRange moveMarkerRange(
            const OTIO_NS::TimeRange& range,
            const OTIO_NS::RationalTime& time,
            bool start,
            const std::optional<OTIO_NS::TimeRange>& bounds)
        {
            const double rate = range.duration().rate();
            const OTIO_NS::RationalTime frame(1.0, rate);
            const OTIO_NS::RationalTime t = time.rescaled_to(rate).round();
            std::optional<OTIO_NS::RationalTime> first;
            std::optional<OTIO_NS::RationalTime> last;
            if (bounds.has_value())
            {
                first = bounds->start_time().rescaled_to(rate).round();
                last = bounds->end_time_inclusive().rescaled_to(rate).round();
            }
            if (start)
            {
                const OTIO_NS::RationalTime duration = range.duration();
                OTIO_NS::RationalTime s = t;
                if (last.has_value())
                {
                    s = std::min(s, *last - duration + frame);
                }
                if (first.has_value())
                {
                    s = std::max(s, *first);
                }
                return OTIO_NS::TimeRange(s, duration);
            }
            OTIO_NS::RationalTime e = t;
            if (last.has_value())
            {
                e = std::min(e, *last);
            }
            e = std::max(e, range.start_time() + frame);
            return OTIO_NS::TimeRange::range_from_start_end_time_inclusive(
                range.start_time(), e);
        }

        struct MarkersModel::Private
        {
            std::shared_ptr<ftk::ObservableList<ReviewMarker> > markers;
        };

        void MarkersModel::_init()
        {
            FTK_P();
            p.markers = ftk::ObservableList<ReviewMarker>::create();
        }

        MarkersModel::MarkersModel() :
            _p(new Private)
        {}

        MarkersModel::~MarkersModel()
        {}

        std::shared_ptr<MarkersModel> MarkersModel::create()
        {
            auto out = std::shared_ptr<MarkersModel>(new MarkersModel);
            out->_init();
            return out;
        }

        const std::vector<ReviewMarker>& MarkersModel::getMarkers() const
        {
            return _p->markers->get();
        }

        std::shared_ptr<ftk::IObservableList<ReviewMarker> > MarkersModel::observeMarkers() const
        {
            return _p->markers;
        }

        void MarkersModel::setMarkers(const std::vector<ReviewMarker>& value)
        {
            _set(value);
        }

        std::string MarkersModel::add(
            const std::optional<OTIO_NS::TimeRange>& range,
            const std::string& name,
            const std::string& text)
        {
            FTK_P();
            ReviewMarker marker;
            marker.id = generateId();
            marker.name = name;
            marker.range = range;
            marker.text = text;
            marker.created = timestamp();
            marker.author = reviewAuthor();
            auto markers = p.markers->get();
            markers.push_back(marker);
            _set(markers);
            return marker.id;
        }

        void MarkersModel::update(const std::string& id, const std::string& text)
        {
            FTK_P();
            auto markers = p.markers->get();
            const auto i = std::find_if(
                markers.begin(),
                markers.end(),
                [id](const ReviewMarker& marker) { return marker.id == id; });
            if (i != markers.end())
            {
                i->text = text;
                p.markers->setIfChanged(markers);
            }
        }

        void MarkersModel::updateColor(const std::string& id, const ftk::Color4F& color)
        {
            FTK_P();
            auto markers = p.markers->get();
            const auto i = std::find_if(
                markers.begin(),
                markers.end(),
                [id](const ReviewMarker& marker) { return marker.id == id; });
            if (i != markers.end())
            {
                i->color = color;
                p.markers->setIfChanged(markers);
            }
        }

        void MarkersModel::setName(const std::string& id, const std::string& name)
        {
            FTK_P();
            const size_t first = name.find_first_not_of(" \t");
            const size_t last = name.find_last_not_of(" \t");
            const std::string trimmed = first != std::string::npos ?
                name.substr(first, last - first + 1) :
                std::string();
            auto markers = p.markers->get();
            const auto i = std::find_if(
                markers.begin(),
                markers.end(),
                [id](const ReviewMarker& marker) { return marker.id == id; });
            if (i != markers.end())
            {
                i->name = trimmed;
                p.markers->setIfChanged(markers);
            }
        }

        void MarkersModel::setRange(const std::string& id, const OTIO_NS::TimeRange& range)
        {
            FTK_P();
            if (range.duration().value() <= 0.0)
            {
                return;
            }
            auto markers = p.markers->get();
            const auto i = std::find_if(
                markers.begin(),
                markers.end(),
                [id](const ReviewMarker& marker) { return marker.id == id; });
            if (i != markers.end() && i->range.has_value())
            {
                if (range != i->range.value())
                {
                    i->range = range;
                    // Sorted again: it may now come before or after others.
                    _set(markers);
                }
            }
        }

        void MarkersModel::remove(const std::string& id)
        {
            FTK_P();
            auto markers = p.markers->get();
            const auto i = std::find_if(
                markers.begin(),
                markers.end(),
                [id](const ReviewMarker& marker) { return marker.id == id; });
            if (i != markers.end())
            {
                markers.erase(i);
                _set(markers);
            }
        }

        void MarkersModel::clear()
        {
            _set({});
        }

        size_t MarkersModel::addCuts(const std::vector<OTIO_NS::RationalTime>& times)
        {
            FTK_P();
            auto markers = p.markers->get();
            const std::string created = timestamp();
            const std::string author = reviewAuthor();
            size_t out = 0;
            for (const auto& time : times)
            {
                const auto i = std::find_if(
                    markers.begin(),
                    markers.end(),
                    [&time](const ReviewMarker& marker)
                    {
                        return marker.range.has_value() &&
                            marker.range->start_time().rescaled_to(time.rate()).round() ==
                                time.round();
                    });
                if (i != markers.end())
                {
                    continue;
                }
                ReviewMarker marker;
                marker.id = generateId();
                marker.name = "Cut";
                marker.range = OTIO_NS::TimeRange(
                    time,
                    OTIO_NS::RationalTime(1.0, time.rate()));
                marker.created = created;
                marker.author = author;
                markers.push_back(marker);
                ++out;
            }
            if (out > 0)
            {
                _set(markers);
            }
            return out;
        }

        void MarkersModel::_set(std::vector<ReviewMarker> value)
        {
            FTK_P();
            // Sorting here rather than in the panel keeps every reader of the
            // model in the same order, and survives a load from a hand-edited
            // review file. The markers about no frame in particular come
            // first: they speak about the whole review.
            std::stable_sort(
                value.begin(),
                value.end(),
                [](const ReviewMarker& a, const ReviewMarker& b)
                {
                    if (a.range.has_value() != b.range.has_value())
                    {
                        return !a.range.has_value();
                    }
                    if (!a.range.has_value())
                    {
                        return false;
                    }
                    return a.range->start_time() < b.range->start_time();
                });
            p.markers->setIfChanged(value);
        }
    }
}
