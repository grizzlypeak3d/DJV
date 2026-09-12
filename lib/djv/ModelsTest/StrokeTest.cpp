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
    }
}
