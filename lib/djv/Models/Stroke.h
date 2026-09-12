// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

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

        ///@}
    }
}
