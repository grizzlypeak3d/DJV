// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/TestLib/ITest.h>

#include <ftk/Core/Mesh.h>

#include <vector>

namespace djv
{
    namespace models_tests
    {
        class StrokeTest : public ftk::test::ITest
        {
        protected:
            StrokeTest(const std::shared_ptr<ftk::Context>&);

        public:
            static std::shared_ptr<StrokeTest> create(
                const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _simplify();
            void _smooth();
            void _mesh();

            // FTK_CHECK reports through the test, so the helpers are part of
            // it rather than free functions.
            void _checkIndices(const ftk::TriMesh2F&);
            void _checkFinite(const std::vector<ftk::V2F>&);
        };
    }
}
