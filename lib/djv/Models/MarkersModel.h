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
        //! The review markers.
        //!
        //! Markers are session state: they live in the review file, not in
        //! the application settings. The list is kept in time order with the
        //! markers about no frame in particular first, so every reader shows
        //! the same feedback in the same order.
        class DJV_MODELS_API_TYPE MarkersModel : public std::enable_shared_from_this<MarkersModel>
        {
            FTK_NON_COPYABLE(MarkersModel);

        protected:
            DJV_MODELS_API void _init();

            MarkersModel();

        public:
            DJV_MODELS_API ~MarkersModel();

            //! Create a new model.
            DJV_MODELS_API static std::shared_ptr<MarkersModel> create();

            //! Get the markers.
            DJV_MODELS_API const std::vector<ReviewMarker>& getMarkers() const;

            //! Observe the markers.
            DJV_MODELS_API std::shared_ptr<ftk::IObservableList<ReviewMarker> > observeMarkers() const;

            //! Replace all the markers, e.g. when a review is opened.
            DJV_MODELS_API void setMarkers(const std::vector<ReviewMarker>&);

            //! Add a marker, returning its identifier. The identifier,
            //! creation time and author are filled in here, so callers only
            //! provide the rest.
            DJV_MODELS_API std::string add(
                const std::optional<OTIO_NS::TimeRange>&,
                const std::string& name,
                const std::string& text);

            //! Replace the text of the marker with the given identifier. The
            //! creation time and author stay: they say who raised the point
            //! and when, not who last touched the wording.
            DJV_MODELS_API void update(const std::string& id, const std::string& text);

            //! Replace the color of the marker with the given identifier.
            DJV_MODELS_API void updateColor(const std::string& id, const ftk::Color4F&);

            //! Remove the marker with the given identifier.
            DJV_MODELS_API void remove(const std::string& id);

            //! Remove all the markers.
            DJV_MODELS_API void clear();

        private:
            //! Sort into time order, then set.
            DJV_MODELS_API void _set(std::vector<ReviewMarker>);

            FTK_PRIVATE();
        };
    }
}
