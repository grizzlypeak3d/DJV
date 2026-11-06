// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/StrokeTest.h>

#include <djv/Models/Stroke.h>

#include <ftk/Core/Assert.h>

#include <algorithm>
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
            _mapping();
            _input();
            _move();
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
                // A rectangle is a closed loop: four mitred corners, two
                // triangles a side, and no caps, which rounded the corner
                // it started from. Its outline reaches past every corner by
                // half the width.
                const auto mesh = models::shapeMesh(models::ReviewStrokeKind::Rectangle, corners, 4.F);
                FTK_CHECK(8 == mesh.v.size());
                FTK_CHECK(8 == mesh.triangles.size());
                float minX = mesh.v.front().x;
                float minY = mesh.v.front().y;
                for (const auto& v : mesh.v)
                {
                    minX = std::min(minX, v.x);
                    minY = std::min(minY, v.y);
                }
                const float x = std::min(corners[0].x, corners[1].x);
                const float y = std::min(corners[0].y, corners[1].y);
                FTK_CHECK(std::abs(minX - (x - 2.F)) < .01F);
                FTK_CHECK(std::abs(minY - (y - 2.F)) < .01F);
            }
            {
                // A cap is left off where asked; an arrow is a shaft with no
                // cap at the head, plus the head.
                const auto capped = models::strokeMesh(corners, 4.F);
                const auto uncapped = models::strokeMesh(corners, 4.F, true, false);
                FTK_CHECK(uncapped.triangles.size() < capped.triangles.size());
                FTK_CHECK(models::strokeMesh(corners, 4.F, false, false).triangles.size() < uncapped.triangles.size());
                const auto arrow = models::shapeMesh(models::ReviewStrokeKind::Arrow, corners, 4.F);
                FTK_CHECK(arrow.triangles.size() == uncapped.triangles.size() + 1);
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

        void StrokeTest::_mapping()
        {
            // A 200 by 100 image drawn twice its size at 10, 20.
            models::ImageMapping m;
            FTK_CHECK(!m.isValid());
            m.box = ftk::Box2I(10, 20, 400, 200);
            m.imageSize = ftk::Size2I(200, 100);
            FTK_CHECK(m.isValid());
            FTK_CHECK(2.F == m.scale());
            FTK_CHECK(m.contains(ftk::V2F(10.F, 20.F)));
            FTK_CHECK(!m.contains(ftk::V2F(9.F, 20.F)));
            const ftk::V2F image = m.toImage(ftk::V2F(110.F, 120.F));
            FTK_CHECK(50.F == image.x && 50.F == image.y);
            const ftk::V2F render = m.toRender(image);
            FTK_CHECK(110.F == render.x && 120.F == render.y);

            // Mirrored, the image's own pixels are the other way round,
            // and a point comes back to where it was.
            m.mirrorX = true;
            const ftk::V2F mirrored = m.toImage(ftk::V2F(110.F, 120.F));
            FTK_CHECK(150.F == mirrored.x && 50.F == mirrored.y);
            const ftk::V2F back = m.toRender(mirrored);
            FTK_CHECK(110.F == back.x && 120.F == back.y);

            const ftk::V2F clamped = m.clamp(ftk::V2F(-5.F, 500.F));
            FTK_CHECK(0.F == clamped.x && 100.F == clamped.y);
        }

        void StrokeTest::_input()
        {
            // Ink: a pen's pressure is kept, a point that barely moved is
            // not.
            models::ReviewStroke ink;
            models::strokeBegin(ink, ftk::V2F(0.F, 0.F), true, .5F);
            FTK_CHECK(1 == ink.points.size() && 1 == ink.pressures.size());
            FTK_CHECK(models::strokeComplete(ink));
            models::strokeContinue(ink, ftk::V2F(.1F, .1F), .6F, false);
            FTK_CHECK(1 == ink.points.size());
            models::strokeContinue(ink, ftk::V2F(10.F, 0.F), .7F, false);
            FTK_CHECK(2 == ink.points.size() && 2 == ink.pressures.size());
            FTK_CHECK(.7F == ink.pressures.back());

            // A finger draws at the width: no pressures.
            models::ReviewStroke finger;
            models::strokeBegin(finger, ftk::V2F(0.F, 0.F), false, 1.F);
            models::strokeContinue(finger, ftk::V2F(10.F, 0.F), 1.F, false);
            FTK_CHECK(finger.pressures.empty());

            // A shape is its two points, the second constrained when asked.
            models::ReviewStroke rect;
            rect.kind = models::ReviewStrokeKind::Rectangle;
            models::strokeBegin(rect, ftk::V2F(0.F, 0.F), true, 1.F);
            FTK_CHECK(rect.pressures.empty());
            FTK_CHECK(!models::strokeComplete(rect));
            models::strokeContinue(rect, ftk::V2F(10.F, 4.F), 1.F, false);
            models::strokeContinue(rect, ftk::V2F(20.F, 5.F), 1.F, false);
            FTK_CHECK(2 == rect.points.size());
            FTK_CHECK(20.F == rect.points[1].x && 5.F == rect.points[1].y);
            FTK_CHECK(models::strokeComplete(rect));
            models::strokeContinue(rect, ftk::V2F(20.F, 5.F), 1.F, true);
            FTK_CHECK(20.F == rect.points[1].x && 20.F == rect.points[1].y);
            FTK_CHECK(models::isTwoPointShape(models::ReviewStrokeKind::Arrow));
            FTK_CHECK(!models::isTwoPointShape(models::ReviewStrokeKind::Freehand));

            // The eraser's reach: the size, or a fingertip on the screen.
            FTK_CHECK(10.F == models::eraserRadius(10.F, 22.F, 4.F));
            FTK_CHECK(11.F == models::eraserRadius(4.F, 22.F, 2.F));
        }
    
        void StrokeTest::_move()
        {
            const ftk::Size2I imageSize(100, 100);

            // Ink moves whole, and stops at the edge of the picture.
            models::ReviewStroke ink;
            ink.kind = models::ReviewStrokeKind::Freehand;
            ink.points = { ftk::V2F(10.F, 10.F), ftk::V2F(20.F, 20.F) };
            FTK_CHECK(models::strokeHandles(ink).empty());
            auto moved = models::moveStroke(ink, ftk::V2F(5.F, 5.F), imageSize);
            FTK_CHECK(15.F == moved.points[0].x && 15.F == moved.points[0].y);
            FTK_CHECK(25.F == moved.points[1].x && 25.F == moved.points[1].y);
            moved = models::moveStroke(ink, ftk::V2F(100.F, -50.F), imageSize);
            FTK_CHECK(90.F == moved.points[0].x && 0.F == moved.points[0].y);
            FTK_CHECK(100.F == moved.points[1].x && 10.F == moved.points[1].y);
            // Without the picture's size, nothing holds it.
            moved = models::moveStroke(ink, ftk::V2F(100.F, 0.F), ftk::Size2I());
            FTK_CHECK(110.F == moved.points[0].x);

            // A shape's two points are its handles.
            models::ReviewStroke rect;
            rect.kind = models::ReviewStrokeKind::Rectangle;
            rect.points = { ftk::V2F(10.F, 10.F), ftk::V2F(30.F, 20.F) };
            const auto handles = models::strokeHandles(rect);
            FTK_CHECK(2 == handles.size());
            FTK_CHECK(30.F == handles[1].x && 20.F == handles[1].y);
            moved = models::moveHandle(rect, 1, ftk::V2F(40.F, 25.F), false);
            FTK_CHECK(10.F == moved.points[0].x && 10.F == moved.points[0].y);
            FTK_CHECK(40.F == moved.points[1].x && 25.F == moved.points[1].y);
            // Held square against the other point.
            moved = models::moveHandle(rect, 0, ftk::V2F(0.F, 5.F), true);
            FTK_CHECK(0.F == moved.points[0].x && -10.F == moved.points[0].y);
            FTK_CHECK(30.F == moved.points[1].x && 20.F == moved.points[1].y);
            moved = models::moveHandle(rect, 2, ftk::V2F(0.F, 0.F), false);
            FTK_CHECK(10.F == moved.points[0].x && 30.F == moved.points[1].x);
            // The whole shape, the corners it is drawn through held inside.
            moved = models::moveStroke(rect, ftk::V2F(-20.F, 100.F), imageSize);
            FTK_CHECK(0.F == moved.points[0].x && 90.F == moved.points[0].y);
            FTK_CHECK(20.F == moved.points[1].x && 100.F == moved.points[1].y);

            // Text, by the box it is taken to cover.
            models::ReviewStroke text;
            text.kind = models::ReviewStrokeKind::Text;
            text.text = "abc";
            text.textSize = 10.F;
            text.points = { ftk::V2F(0.F, 0.F) };
            FTK_CHECK(models::strokeHandles(text).empty());
            moved = models::moveStroke(text, ftk::V2F(200.F, 200.F), imageSize);
            FTK_CHECK(82.F == moved.points[0].x && 90.F == moved.points[0].y);
        }
    }
}
