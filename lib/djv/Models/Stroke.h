// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <djv/Models/Review.h>

#include <ftk/Core/Mesh.h>

#include <vector>

namespace djv
{
    namespace models
    {
        //! \name Annotation Strokes
        ///@{

        //! Drop points closer together than the given distance.
        //!
        //! Freehand input arrives in dense clusters, and clustered control
        //! points make a Catmull-Rom spline overshoot; thinning first is what
        //! keeps the curve clean.
        DJV_MODELS_API std::vector<ftk::V2F> simplifyPath(
            const std::vector<ftk::V2F>& points,
            float minDistance);

        //! Smooth a path into a curve, with the given subdivisions between
        //! each pair of points. A path of fewer than three points is returned
        //! as it is.
        DJV_MODELS_API std::vector<ftk::V2F> smoothPath(
            const std::vector<ftk::V2F>& points,
            int subdivisions);

        //! Build the mesh for a stroke of the given width along a path, with
        //! round caps at the ends. Mesh vertex indices are one-based.
        DJV_MODELS_API ftk::TriMesh2F strokeMesh(
            const std::vector<ftk::V2F>& points,
            float width);

        //! The path a shape is drawn along, from the points a stroke of
        //! that kind holds: the four sides of a rectangle, the circumference
        //! of an ellipse, a line's two ends. Freehand ink and text give their
        //! points back as they are. A rectangle's and an ellipse's paths are
        //! closed: the first point comes again at the end.
        DJV_MODELS_API std::vector<ftk::V2F> shapePath(
            ReviewStrokeKind,
            const std::vector<ftk::V2F>& points);

        //! Build the mesh for a shape stroke of the given width: a ribbon along
        //! shapePath(), and for an arrow a head at the second point, sized
        //! with the width. The points and the width are in the same space,
        //! whichever that is. Text has no mesh.
        DJV_MODELS_API ftk::TriMesh2F shapeMesh(
            ReviewStrokeKind,
            const std::vector<ftk::V2F>& points,
            float width);

        //! Does the stroke pass within the radius of the position? The
        //! position and the radius are in the pixels of the source image,
        //! as the stroke is; its own width counts, so a thick stroke is as
        //! easy to hit as it looks. Text is hit anywhere in the box it is
        //! drawn in, taken from its size and length.
        DJV_MODELS_API bool strokeHit(
            const ReviewStroke&,
            const ftk::V2F& pos,
            float radius);

        ///@}
    }
}
