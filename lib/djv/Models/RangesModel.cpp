// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/RangesModel.h>

#include <algorithm>

namespace djv
{
    namespace models
    {
        struct RangesModel::Private
        {
            std::shared_ptr<ftk::ObservableList<ReviewRange> > ranges;
        };

        void RangesModel::_init()
        {
            FTK_P();
            p.ranges = ftk::ObservableList<ReviewRange>::create();
        }

        RangesModel::RangesModel() :
            _p(new Private)
        {}

        RangesModel::~RangesModel()
        {}

        std::shared_ptr<RangesModel> RangesModel::create()
        {
            auto out = std::shared_ptr<RangesModel>(new RangesModel);
            out->_init();
            return out;
        }

        const std::vector<ReviewRange>& RangesModel::getRanges() const
        {
            return _p->ranges->get();
        }

        std::shared_ptr<ftk::IObservableList<ReviewRange> > RangesModel::observeRanges() const
        {
            return _p->ranges;
        }

        void RangesModel::setRanges(const std::vector<ReviewRange>& value)
        {
            _set(value);
        }

        void RangesModel::add(const OTIO_NS::TimeRange& range, const std::string& name)
        {
            FTK_P();
            ReviewRange value;
            value.id = generateId();
            value.name = name;
            value.range = range;
            auto ranges = p.ranges->get();
            ranges.push_back(value);
            _set(ranges);
        }

        void RangesModel::remove(const std::string& id)
        {
            FTK_P();
            auto ranges = p.ranges->get();
            const auto i = std::find_if(
                ranges.begin(),
                ranges.end(),
                [id](const ReviewRange& range) { return range.id == id; });
            if (i != ranges.end())
            {
                ranges.erase(i);
                _set(ranges);
            }
        }

        void RangesModel::clear()
        {
            _set({});
        }

        void RangesModel::_set(std::vector<ReviewRange> value)
        {
            FTK_P();
            // Sorting here rather than in the panel keeps every reader of the
            // model in the same order, and survives a load from a hand-edited
            // review file.
            std::stable_sort(
                value.begin(),
                value.end(),
                [](const ReviewRange& a, const ReviewRange& b)
                {
                    // A range with no bounds cannot be ordered against one that
                    // has them; park it at the end rather than at frame zero.
                    if (!a.range.has_value())
                    {
                        return false;
                    }
                    if (!b.range.has_value())
                    {
                        return true;
                    }
                    return a.range->start_time() < b.range->start_time();
                });
            p.ranges->setIfChanged(value);
        }
    }
}
