// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/ColorModelTest.h>

#include <djv/ModelsTest/ModelsTestUtil.h>

#include <djv/Models/ColorModel.h>

#include <ftk/Core/Assert.h>

namespace djv
{
    namespace models_tests
    {
        ColorModelTest::ColorModelTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::ColorModelTest")
        {}

        std::shared_ptr<ColorModelTest> ColorModelTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<ColorModelTest>(new ColorModelTest(context));
        }

        void ColorModelTest::run()
        {
            _resolve();
            _labels();
        }

        // The resolution order against the built-in configuration, which
        // carries the canonical names and interop aliases the resolver
        // matches. The tests compare resolutions to each other rather than
        // to configuration names, so a configuration update that renames a
        // space does not break them. Without OpenColorIO -- the GLES3
        // continuous integration build -- nothing resolves, and the tests
        // check that instead.
        void ColorModelTest::_resolve()
        {
            auto settings = createTestSettings(_context);
            auto model = models::ColorModel::create(_context, settings);

#if defined(TLRENDER_OCIO)
            // The colorInteropID attribute is the file naming its color
            // space directly.
            const std::string rec709 = model->resolveInput(
                "render.0001.exr", { { "colorInteropID", "lin_rec709" } });
            FTK_CHECK(!rec709.empty());

            // Chromaticities within tolerance of a standard set resolve
            // to that set.
            FTK_CHECK(rec709 == model->resolveInput(
                "render.0001.exr",
                { { "Chromaticities",
                    "0.64 0.33 0.3 0.6 0.15 0.06 0.3127 0.329" } }));
            const std::string acescg = model->resolveInput(
                "render.0001.exr",
                { { "Chromaticities",
                    "0.713 0.293 0.165 0.83 0.128 0.044 0.32168 0.33767" } });
            FTK_CHECK(!acescg.empty());
            FTK_CHECK(acescg != rec709);

            // An OpenEXR that declares nothing is taken at the format's own
            // default: linear with Rec.709 primaries.
            FTK_CHECK(rec709 == model->resolveInput("render.0001.exr"));

            // A declaration the configuration cannot match resolves nothing,
            // even for an OpenEXR: the file said what it is, and neither the
            // format default nor an extension rule may paper over it.
            FTK_CHECK(model->resolveInput(
                "render.0001.exr",
                { { "colorInteropID", "not_a_color_space" } }).empty());

            // Movie color primaries and transfer tags resolve like a
            // declaration.
            FTK_CHECK(!model->resolveInput(
                "movie.mov",
                { { "Color Primaries", "bt709" },
                  { "Color Transfer", "iec61966-2-1" } }).empty());

            // A file that declares nothing only resolves through a rule the
            // configuration author wrote; the default rule matches every
            // path and is refused.
            FTK_CHECK(model->resolveInput("image.png").empty());

            // A timeline is a container of media in their own color spaces,
            // not an image with one of its own.
            FTK_CHECK(model->resolveInput("timeline.otio").empty());
            FTK_CHECK(model->resolveInput("timeline.otioz").empty());

            // The user's extension assignments outrank everything the file
            // says, including the OpenEXR default.
            model->setExtColorSpaces({ { ".exr", acescg } });
            FTK_CHECK(acescg == model->resolveInput("render.0001.exr"));
#else // TLRENDER_OCIO
            FTK_CHECK(model->resolveInput(
                "render.0001.exr",
                { { "colorInteropID", "lin_rec709" } }).empty());
            FTK_CHECK(model->resolveInput("image.png").empty());
#endif // TLRENDER_OCIO
        }

        // The resolved input label names the resolution and its source, or
        // says why nothing resolved.
        void ColorModelTest::_labels()
        {
            auto settings = createTestSettings(_context);
            auto model = models::ColorModel::create(_context, settings);

            // The label is resolved for the active file, and only while the
            // options are enabled with an automatic (empty) input.
            tl::OCIOOptions options;
            options.enabled = true;
            model->setOCIOOptions(options);

#if defined(TLRENDER_OCIO)
            const std::string rec709 = model->resolveInput("render.0001.exr");
            model->setActiveFiles({ { "render.0001.exr", {} } });
            FTK_CHECK(
                rec709 + " (EXR default)" ==
                model->observeResolvedInput()->get());

            model->setActiveFiles({ {
                "render.0001.exr",
                { { "colorInteropID", "lin_rec709" } } } });
            FTK_CHECK(
                rec709 + " (file)" ==
                model->observeResolvedInput()->get());

            model->setActiveFiles({ {
                "render.0001.exr",
                { { "colorInteropID", "not_a_color_space" } } } });
            FTK_CHECK(
                "not_a_color_space (not in the configuration)" ==
                model->observeResolvedInput()->get());

            model->setExtColorSpaces({ { ".exr", rec709 } });
            model->setActiveFiles({ { "render.0001.exr", {} } });
            FTK_CHECK(
                rec709 + " (extension)" ==
                model->observeResolvedInput()->get());
#else // TLRENDER_OCIO
            model->setActiveFiles({ { "render.0001.exr", {} } });
            FTK_CHECK(model->observeResolvedInput()->get().empty());
#endif // TLRENDER_OCIO
        }
    }
}
