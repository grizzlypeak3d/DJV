// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/StrokeTest.h>

#include <djv/Models/Stroke.h>

#include <ftk/Core/Assert.h>

#include <cmath>

namespace djv
{
    namespace models_tests
    {
        StrokeTest::StrokeTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::StrokeTest")
        {}

        std::shared_ptr<StrokeTest> StrokeTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<StrokeTest>(new StrokeTest(context));
        }

        void StrokeTest::run()
        {
            _simplify();
            _smooth();
            _mesh();
            _shapes();
            _hit();
        }

        // Every index is one-based and inside the mesh, or the renderer
        // reads past the vertices.
        void StrokeTest::_checkIndices(const ftk::TriMesh2F& mesh)
        {
            for (const auto& triangle : mesh.triangles)
            {
                for (size_t i = 0; i < 3; ++i)
                {
                    FTK_CHECK(triangle.v[i].v >= 1);
                    FTK_CHECK(triangle.v[i].v <= mesh.v.size());
                }
            }
        }

        void StrokeTest::_checkFinite(const std::vector<ftk::V2F>& points)
        {
            for (const auto& i : points)
            {
                FTK_CHECK(std::isfinite(i.x));
                FTK_CHECK(std::isfinite(i.y));
            }
        }

        void StrokeTest::_simplify()
        {
            FTK_CHECK(models::simplifyPath({}, 3.F).empty());

            // A single point is the whole path.
            {
                const std::vector<ftk::V2F> in = { ftk::V2F(1.F, 2.F) };
                FTK_CHECK(1 == models::simplifyPath(in, 3.F).size());
            }

            // The two ends are kept however close together they are: it is
            // the middle that is thinned.
            {
                const std::vector<ftk::V2F> in =
                {
                    ftk::V2F(0.F, 0.F),
                    ftk::V2F(.1F, 0.F)
                };
                FTK_CHECK(2 == models::simplifyPath(in, 3.F).size());
            }

            // The clustered middle points go, the ends stay.
            {
                const std::vector<ftk::V2F> in =
                {
                    ftk::V2F(0.F, 0.F),
                    ftk::V2F(1.F, 0.F),
                    ftk::V2F(2.F, 0.F),
                    ftk::V2F(10.F, 0.F)
                };
                const auto out = models::simplifyPath(in, 3.F);
                FTK_CHECK(2 == out.size());
                FTK_CHECK(0.F == out.front().x);
                FTK_CHECK(10.F == out.back().x);
            }

            // Far enough apart to all survive.
            {
                const std::vector<ftk::V2F> in =
                {
                    ftk::V2F(0.F, 0.F),
                    ftk::V2F(5.F, 0.F),
                    ftk::V2F(10.F, 0.F)
                };
                FTK_CHECK(3 == models::simplifyPath(in, 3.F).size());
            }
        }

        void StrokeTest::_smooth()
        {
            // Fewer than three points is not a curve, and comes back as it
            // went in.
            {
                const std::vector<ftk::V2F> in =
                {
                    ftk::V2F(0.F, 0.F),
                    ftk::V2F(1.F, 1.F)
                };
                const auto out = models::smoothPath(in, 8);
                FTK_CHECK(in.size() == out.size());
                for (size_t i = 0; i < in.size(); ++i)
                {
                    FTK_CHECK(in[i].x == out[i].x);
                    FTK_CHECK(in[i].y == out[i].y);
                }
            }

            // The curve ends where the path does.
            {
                const std::vector<ftk::V2F> in =
                {
                    ftk::V2F(0.F, 0.F),
                    ftk::V2F(5.F, 5.F),
                    ftk::V2F(10.F, 0.F)
                };
                const auto out = models::smoothPath(in, 8);
                FTK_CHECK(out.size() > in.size());
                FTK_CHECK(in.back().x == out.back().x);
                FTK_CHECK(in.back().y == out.back().y);
                _checkFinite(out);
            }

            // Points on top of each other collapse their span rather than
            // dividing by the distance between them.
            {
                const std::vector<ftk::V2F> in =
                {
                    ftk::V2F(0.F, 0.F),
                    ftk::V2F(0.F, 0.F),
                    ftk::V2F(10.F, 0.F)
                };
                _checkFinite(models::smoothPath(in, 8));
            }
        }

        void StrokeTest::_mesh()
        {
            // Nothing to draw.
            {
                const auto mesh = models::strokeMesh({}, 4.F);
                FTK_CHECK(mesh.v.empty());
                FTK_CHECK(mesh.triangles.empty());
            }

            // A single point is a dot.
            {
                const std::vector<ftk::V2F> in = { ftk::V2F(0.F, 0.F) };
                const auto mesh = models::strokeMesh(in, 4.F);
                FTK_CHECK(!mesh.v.empty());
                FTK_CHECK(!mesh.triangles.empty());
                _checkIndices(mesh);
            }

            // A stroke is two vertices per point, plus a cap at each end.
            {
                const std::vector<ftk::V2F> in =
                {
                    ftk::V2F(0.F, 0.F),
                    ftk::V2F(10.F, 0.F),
                    ftk::V2F(20.F, 0.F)
                };
                const auto mesh = models::strokeMesh(in, 4.F);
                FTK_CHECK(mesh.v.size() > in.size() * 2);
                _checkIndices(mesh);
            }

            // A corner sharp enough to turn back on itself: the mitre is
            // held to three radii so the outside cannot spike away.
            {
                const float width = 4.F;
                const float radius = width / 2.F;
                const std::vector<ftk::V2F> in =
                {
                    ftk::V2F(0.F, 0.F),
                    ftk::V2F(10.F, 0.F),
                    ftk::V2F(0.F, .5F)
                };
                const auto mesh = models::strokeMesh(in, width);
                _checkIndices(mesh);
                // The ribbon comes first, two vertices per point.
                for (size_t i = 0; i < in.size(); ++i)
                {
                    for (size_t j = 0; j < 2; ++j)
                    {
                        const ftk::V2F& v = mesh.v[i * 2 + j];
                        const float dx = v.x - in[i].x;
                        const float dy = v.y - in[i].y;
                        FTK_CHECK(
                            std::sqrt(dx * dx + dy * dy) <= radius * 3.F + .001F);
                    }
                }
            }
        }

        void StrokeTest::_shapes()
        {
            const std::vector<ftk::V2F> corners =
            {
                ftk::V2F(10.F, 20.F),
                ftk::V2F(50.F, 40.F)
            };
            // A rectangle is its four sides, closed.
            {
                const auto path = models::shapePath(models::ReviewStrokeKind::Rectangle, corners);
                FTK_CHECK(5 == path.size());
                FTK_CHECK(path.front().x == path.back().x && path.front().y == path.back().y);
                FTK_CHECK(50.F == path[1].x && 20.F == path[1].y);
                FTK_CHECK(10.F == path[3].x && 40.F == path[3].y);
            }
            // An ellipse fills the box its corners give, and is closed.
            {
                const auto path = models::shapePath(models::ReviewStrokeKind::Ellipse, corners);
                FTK_CHECK(path.size() > 16);
                FTK_CHECK(path.front().x == path.back().x && path.front().y == path.back().y);
                for (const auto& p : path)
                {
                    FTK_CHECK(p.x >= 10.F - .01F && p.x <= 50.F + .01F);
                    FTK_CHECK(p.y >= 20.F - .01F && p.y <= 40.F + .01F);
                }
                _checkFinite(path);
            }
            // A line is its two ends; with one point so far it is a dot.
            {
                FTK_CHECK(2 == models::shapePath(models::ReviewStrokeKind::Line, corners).size());
                FTK_CHECK(1 == models::shapePath(models::ReviewStrokeKind::Line, { corners[0] }).size());
            }
            // Freehand ink comes back as it is.
            {
                const auto path = models::shapePath(models::ReviewStrokeKind::Freehand, corners);
                FTK_CHECK(corners.size() == path.size());
            }
            // Every shape has a mesh the renderer can draw; the arrow's has
            // its head over the ribbon, text has none.
            for (const auto kind : {
                models::ReviewStrokeKind::Line,
                models::ReviewStrokeKind::Arrow,
                models::ReviewStrokeKind::Rectangle,
                models::ReviewStrokeKind::Ellipse })
            {
                const auto mesh = models::shapeMesh(kind, corners, 4.F);
                FTK_CHECK(!mesh.triangles.empty());
                _checkIndices(mesh);
                _checkFinite(mesh.v);
            }
            {
                const auto line = models::shapeMesh(models::ReviewStrokeKind::Line, corners, 4.F);
                const auto arrow = models::shapeMesh(models::ReviewStrokeKind::Arrow, corners, 4.F);
                FTK_CHECK(arrow.triangles.size() == line.triangles.size() + 1);
                // The tip of the head is the second point.
                bool tip = false;
                for (const auto& v : arrow.v)
                {
                    tip = tip || (v.x == corners[1].x && v.y == corners[1].y);
                }
                FTK_CHECK(tip);
            }
            FTK_CHECK(models::shapeMesh(models::ReviewStrokeKind::Text, corners, 4.F).triangles.empty());
            // An arrow of no length is a dot, not a division by zero.
            {
                const auto mesh = models::shapeMesh(
                    models::ReviewStrokeKind::Arrow,
                    { corners[0], corners[0] },
                    4.F);
                FTK_CHECK(!mesh.triangles.empty());
                _checkFinite(mesh.v);
            }
        }

        void StrokeTest::_hit()
        {
            models::ReviewStroke stroke;
            stroke.width = 4.F;
            stroke.points = { ftk::V2F(10.F, 20.F), ftk::V2F(50.F, 40.F) };
            // A rectangle is hit on its sides and not in its middle.
            stroke.kind = models::ReviewStrokeKind::Rectangle;
            FTK_CHECK(models::strokeHit(stroke, ftk::V2F(30.F, 20.F), 1.F));
            FTK_CHECK(!models::strokeHit(stroke, ftk::V2F(30.F, 30.F), 1.F));
            // A line is hit along it and not across the box's other corner.
            stroke.kind = models::ReviewStrokeKind::Line;
            FTK_CHECK(models::strokeHit(stroke, ftk::V2F(30.F, 30.F), 1.F));
            FTK_CHECK(!models::strokeHit(stroke, ftk::V2F(50.F, 20.F), 1.F));
            // The width counts.
            FTK_CHECK(models::strokeHit(stroke, ftk::V2F(30.F, 32.5F), 1.F));
            FTK_CHECK(!models::strokeHit(stroke, ftk::V2F(30.F, 40.F), 1.F));
            // Text is hit in its box.
            stroke.kind = models::ReviewStrokeKind::Text;
            stroke.text = "Note";
            stroke.textSize = 20.F;
            stroke.points = { ftk::V2F(10.F, 20.F) };
            FTK_CHECK(models::strokeHit(stroke, ftk::V2F(30.F, 30.F), 1.F));
            FTK_CHECK(!models::strokeHit(stroke, ftk::V2F(30.F, 60.F), 1.F));
            FTK_CHECK(!models::strokeHit(stroke, ftk::V2F(200.F, 30.F), 1.F));
            // Nothing is hit on nothing.
            stroke.points.clear();
            FTK_CHECK(!models::strokeHit(stroke, ftk::V2F(10.F, 20.F), 100.F));
        }
    }
}
