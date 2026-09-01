// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <djv/Models/Review.h>

#include <ftk/Core/ObservableList.h>

namespace djv
{
    namespace models
    {
        //! The named frame ranges of a review.
        //!
        //! Ranges are session state: they live in the review file, not in the
        //! application settings. The list is kept sorted by start frame, so it
        //! reads like a shot breakdown rather than like a history.
        class DJV_MODELS_API_TYPE RangesModel : public std::enable_shared_from_this<RangesModel>
        {
            FTK_NON_COPYABLE(RangesModel);

        protected:
            DJV_MODELS_API void _init();

            RangesModel();

        public:
            DJV_MODELS_API ~RangesModel();

            //! Create a new model.
            DJV_MODELS_API static std::shared_ptr<RangesModel> create();

            //! Get the ranges.
            DJV_MODELS_API const std::vector<ReviewRange>& getRanges() const;

            //! Observe the ranges.
            DJV_MODELS_API std::shared_ptr<ftk::IObservableList<ReviewRange> > observeRanges() const;

            //! Replace all the ranges, e.g. when a review is opened.
            DJV_MODELS_API void setRanges(const std::vector<ReviewRange>&);

            //! Add a range. The identifier is filled in here, so callers only
            //! provide the range and its name.
            DJV_MODELS_API void add(const OTIO_NS::TimeRange&, const std::string& name);

            //! Remove the range with the given identifier.
            DJV_MODELS_API void remove(const std::string& id);

            //! Remove all the ranges.
            DJV_MODELS_API void clear();

        private:
            //! Sort by start frame, then set.
            DJV_MODELS_API void _set(std::vector<ReviewRange>);

            FTK_PRIVATE();
        };
    }
}
