// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/Stroke.h>

#include <algorithm>
#include <cmath>

namespace djv
{
    namespace models
    {
        //! Drop points closer together than the given distance.
        //!
        //! Freehand input arrives in dense clusters, and clustered control points
        //! make a Catmull-Rom spline overshoot; thinning first is what keeps the
        //! curve clean.
        std::vector<ftk::V2F> simplifyPath(const std::vector<ftk::V2F>& points, float minDistance)
        {
            std::vector<ftk::V2F> out;
            if (points.empty())
            {
                return out;
            }
            out.push_back(points.front());
            const float minSquared = minDistance * minDistance;
            for (size_t i = 1; i + 1 < points.size(); ++i)
            {
                const float dx = points[i].x - out.back().x;
                const float dy = points[i].y - out.back().y;
                if (dx * dx + dy * dy >= minSquared)
                {
                    out.push_back(points[i]);
                }
            }
            if (points.size() > 1)
            {
                out.push_back(points.back());
            }
            return out;
        }

        //! Smooth a path with a centripetal Catmull-Rom spline.
        //!
        //! The curve passes through every control point while rounding the corners.
        //! The centripetal parameterisation (alpha = 0.5) is what avoids the cusps
        //! and self-intersections that the uniform form produces on unevenly spaced
        //! points.
        std::vector<ftk::V2F> smoothPath(const std::vector<ftk::V2F>& points, int subdivisions)
        {
            if (points.size() < 3)
            {
                return points;
            }
            auto knot = [](float t, const ftk::V2F& a, const ftk::V2F& b)
            {
                const float dx = b.x - a.x;
                const float dy = b.y - a.y;
                // alpha = 0.5 -> the fourth root of the squared distance.
                return t + std::pow(std::sqrt(dx * dx + dy * dy), .5F);
            };
            std::vector<ftk::V2F> out;
            out.reserve(points.size() * subdivisions);
            const size_t size = points.size();
            for (size_t i = 0; i + 1 < size; ++i)
            {
                // Duplicate the ends so the first and last spans are drawn too.
                const ftk::V2F& p0 = points[i > 0 ? i - 1 : 0];
                const ftk::V2F& p1 = points[i];
                const ftk::V2F& p2 = points[i + 1];
                const ftk::V2F& p3 = points[i + 2 < size ? i + 2 : size - 1];

                const float t0 = 0.F;
                const float t1 = knot(t0, p0, p1);
                const float t2 = knot(t1, p1, p2);
                const float t3 = knot(t2, p2, p3);
                if (t2 - t1 <= 0.F)
                {
                    continue;
                }
                for (int j = 0; j < subdivisions; ++j)
                {
                    const float t = t1 + (t2 - t1) * (j / static_cast<float>(subdivisions));
                    // Barry-Goldman pyramidal evaluation, guarding the spans that
                    // collapse when two control points coincide.
                    auto lerp = [](const ftk::V2F& a, const ftk::V2F& b, float ta, float tb, float t)
                    {
                        const float d = tb - ta;
                        if (d <= 0.F)
                        {
                            return a;
                        }
                        const float u = (t - ta) / d;
                        return ftk::V2F(
                            a.x + (b.x - a.x) * u,
                            a.y + (b.y - a.y) * u);
                    };
                    const ftk::V2F a1 = lerp(p0, p1, t0, t1, t);
                    const ftk::V2F a2 = lerp(p1, p2, t1, t2, t);
                    const ftk::V2F a3 = lerp(p2, p3, t2, t3, t);
                    const ftk::V2F b1 = lerp(a1, a2, t0, t2, t);
                    const ftk::V2F b2 = lerp(a2, a3, t1, t3, t);
                    out.push_back(lerp(b1, b2, t1, t2, t));
                }
            }
            out.push_back(points.back());
            return out;
        }

        //! Append a filled disc to a mesh, used for the two end caps.
        //!
        //! The number of sides follows the radius: a fixed count would show flat
        //! facets once a stroke is thick or zoomed in. Mesh vertex indices are
        //! one-based.
        void addDisc(ftk::TriMesh2F& mesh, const ftk::V2F& center, float radius)
        {
            const int segments = std::max(16, std::min(96, static_cast<int>(radius * 2.F)));
            const size_t centerIndex = mesh.v.size() + 1;
            mesh.v.push_back(center);
            for (int i = 0; i < segments; ++i)
            {
                const float a = i / static_cast<float>(segments) * 2.F * 3.14159265F;
                mesh.v.push_back(ftk::V2F(
                    center.x + std::cos(a) * radius,
                    center.y + std::sin(a) * radius));
            }
            for (int i = 0; i < segments; ++i)
            {
                // Wind the same way as the ribbon quads: the renderer culls back
                // faces, so a disc wound the other way is simply never drawn.
                ftk::Triangle2 triangle;
                triangle.v[0].v = centerIndex;
                triangle.v[1].v = centerIndex + 1 + ((i + 1) % segments);
                triangle.v[2].v = centerIndex + 1 + i;
                mesh.triangles.push_back(triangle);
            }
        }

        //! Build a mesh for a stroke as one continuous ribbon.
        //!
        //! Consecutive quads share their vertices, so the outline has no gap and no
        //! overlap anywhere along the path -- that is what removes the notches that
        //! independent per-segment quads leave on the outside of a curve. Each point
        //! is offset along the averaged normal of its two neighbouring segments (a
        //! mitre), lengthened by 1/cos to hold the width through a turn and clamped
        //! so a sharp corner cannot spike. Round caps close the two ends.
        ftk::TriMesh2F strokeMesh(const std::vector<ftk::V2F>& points, float width)
        {
            ftk::TriMesh2F out;
            const float radius = std::max(.5F, width / 2.F);
            if (points.empty())
            {
                return out;
            }
            if (1 == points.size())
            {
                addDisc(out, points[0], radius);
                return out;
            }

            auto direction = [](const ftk::V2F& a, const ftk::V2F& b)
            {
                const float dx = b.x - a.x;
                const float dy = b.y - a.y;
                const float length = std::sqrt(dx * dx + dy * dy);
                return length > 0.F ?
                    ftk::V2F(dx / length, dy / length) :
                    ftk::V2F(0.F, 0.F);
            };

            const size_t size = points.size();
            for (size_t i = 0; i < size; ++i)
            {
                const ftk::V2F dirPrev = i > 0 ?
                    direction(points[i - 1], points[i]) :
                    direction(points[0], points[1]);
                const ftk::V2F dirNext = i + 1 < size ?
                    direction(points[i], points[i + 1]) :
                    direction(points[size - 2], points[size - 1]);

                ftk::V2F tangent(dirPrev.x + dirNext.x, dirPrev.y + dirNext.y);
                const float length = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
                if (length > 0.F)
                {
                    tangent.x /= length;
                    tangent.y /= length;
                }
                else
                {
                    tangent = dirNext;
                }

                float offset = radius;
                const float cosHalf = dirNext.x * tangent.x + dirNext.y * tangent.y;
                if (cosHalf > .25F)
                {
                    offset = std::min(radius / cosHalf, radius * 3.F);
                }
                const ftk::V2F normal(-tangent.y * offset, tangent.x * offset);

                out.v.push_back(ftk::V2F(points[i].x + normal.x, points[i].y + normal.y));
                out.v.push_back(ftk::V2F(points[i].x - normal.x, points[i].y - normal.y));
            }

            for (size_t i = 0; i + 1 < size; ++i)
            {
                // One-based indices: left(i) = 2i+1, right(i) = 2i+2.
                const size_t left = i * 2 + 1;
                const size_t right = left + 1;
                const size_t leftNext = left + 2;
                const size_t rightNext = right + 2;
                ftk::Triangle2 triangle;
                triangle.v[0].v = left;
                triangle.v[1].v = leftNext;
                triangle.v[2].v = rightNext;
                out.triangles.push_back(triangle);
                triangle.v[0].v = left;
                triangle.v[1].v = rightNext;
                triangle.v[2].v = right;
                out.triangles.push_back(triangle);
            }

            addDisc(out, points.front(), radius);
            addDisc(out, points.back(), radius);
            return out;
        }
    }
}
