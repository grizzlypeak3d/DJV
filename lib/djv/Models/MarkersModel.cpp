// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/MarkersModel.h>

#include <algorithm>

namespace djv
{
    namespace models
    {
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

        void MarkersModel::add(
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
