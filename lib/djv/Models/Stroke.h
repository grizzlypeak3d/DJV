// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <djv/Models/Review.h>

#include <ftk/Core/Box.h>
#include <ftk/Core/Mesh.h>
#include <ftk/Core/Size.h>

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
        //! simplifyPath() and smoothPath() with a value carried at each
        //! point, a pen's pressure: dropped and interpolated as the points
        //! are. The output values are empty when the input ones are.
        DJV_MODELS_API void simplifyPath(
            const std::vector<ftk::V2F>& points,
            const std::vector<float>& values,
            float minDistance,
            std::vector<ftk::V2F>& outPoints,
            std::vector<float>& outValues);
        DJV_MODELS_API void smoothPath(
            const std::vector<ftk::V2F>& points,
            const std::vector<float>& values,
            int subdivisions,
            std::vector<ftk::V2F>& outPoints,
            std::vector<float>& outValues);

        //! The ribbon with a width at each point.
        DJV_MODELS_API ftk::TriMesh2F strokeMesh(
            const std::vector<ftk::V2F>& points,
            const std::vector<float>& widths,
            bool capStart = true,
            bool capEnd = true);

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
        //! round caps at the ends unless told to leave one off, as the shaft
        //! of an arrow does at its head. Nothing in the mesh overlaps, so a
        //! translucent stroke blends once everywhere. Mesh vertex indices
        //! are one-based.
        DJV_MODELS_API ftk::TriMesh2F strokeMesh(
            const std::vector<ftk::V2F>& points,
            float width,
            bool capStart = true,
            bool capEnd = true);

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

        //! Where a source image is in a view: its box in render space
        //! (the view's pixels before the zoom and the pan), the size of the
        //! image, and whether it is mirrored. Strokes are kept in the
        //! image's own pixels, unmirrored, so they stay on the picture
        //! whatever the zoom, the comparison or the mirror.
        struct DJV_MODELS_API_TYPE ImageMapping
        {
            ftk::Box2I  box;
            ftk::Size2I imageSize;
            bool        mirrorX = false;
            bool        mirrorY = false;

            DJV_MODELS_API bool isValid() const;

            //! Whether a point of render space is on the image.
            DJV_MODELS_API bool contains(const ftk::V2F& render) const;

            //! A point of render space in the image's pixels, and back.
            DJV_MODELS_API ftk::V2F toImage(const ftk::V2F& render) const;
            DJV_MODELS_API ftk::V2F toRender(const ftk::V2F& image) const;

            //! Held inside the image.
            DJV_MODELS_API ftk::V2F clamp(const ftk::V2F& image) const;

            //! Render pixels to one image pixel.
            DJV_MODELS_API float scale() const;
        };

        //! Whether a kind of stroke is a shape between two points.
        DJV_MODELS_API bool isTwoPointShape(ReviewStrokeKind);

        //! One point of a shape against its other point, constrained:
        //! square or round, or at a multiple of forty-five degrees.
        DJV_MODELS_API ftk::V2F constrainShape(
            ReviewStrokeKind,
            const ftk::V2F& anchor,
            const ftk::V2F& pos,
            bool constrain);

        //! \name Drawing a Stroke
        //! What the desktop's viewport and the iPad do with a pointer, a
        //! finger or a pen while a stroke is drawn. The stroke comes in
        //! with its kind, color and width set; the points are in the
        //! image's pixels.
        ///@{

        //! The first point, and a pen's pressure for freehand ink.
        DJV_MODELS_API void strokeBegin(
            ReviewStroke&,
            const ftk::V2F& pos,
            bool pen,
            float pressure);

        //! The next point: ink takes it, unless it barely moved, with the
        //! pressure when the stroke has them; a shape takes it as its
        //! second point, which the caller holds inside the picture, and
        //! constrained when asked.
        DJV_MODELS_API void strokeContinue(
            ReviewStroke&,
            const ftk::V2F& pos,
            float pressure,
            bool constrain);

        //! Whether a stroke is worth keeping: a shape wants its second
        //! point, ink and text one.
        DJV_MODELS_API bool strokeComplete(const ReviewStroke&);

        //! How far the eraser reaches, in the image's pixels: the tool's
        //! size, and for a finger never less than a fingertip covers on
        //! the screen. The reach is in screen pixels, and the scale is the
        //! screen pixels to one image pixel.
        DJV_MODELS_API float eraserRadius(float size, float screenReach, float screenScale);

        ///@}

        //! \name Moving a Stroke
        //! What the desktop's viewport and the iPad do with the Select
        //! tool. The points are in the image's pixels.
        ///@{

        //! The points of a stroke that are dragged on their own: the two
        //! of a shape; none for ink or text, which move whole.
        DJV_MODELS_API std::vector<ftk::V2F> strokeHandles(const ReviewStroke&);

        //! A stroke moved by an offset, held so that what it covers stays
        //! on an image of the size: it stops at the edge rather than
        //! leaving the picture.
        DJV_MODELS_API ReviewStroke moveStroke(
            const ReviewStroke&,
            const ftk::V2F& offset,
            const ftk::Size2I& imageSize);

        //! A stroke with one of its handles moved to a point, against the
        //! other, constrained when asked.
        DJV_MODELS_API ReviewStroke moveHandle(
            const ReviewStroke&,
            int handle,
            const ftk::V2F& pos,
            bool constrain);

        ///@}

        ///@}
    }
}
