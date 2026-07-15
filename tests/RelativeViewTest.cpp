// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/RelativeView.h>
#include <djv/Models/SettingsModel.h>

#include <nlohmann/json.hpp>

#include <cmath>
#include <iostream>
#include <string>

namespace
{
    int failures = 0;

    void check(bool value, const std::string& message)
    {
        if (!value)
        {
            ++failures;
            std::cerr << "FAIL: " << message << '\n';
        }
    }

    bool close(double a, double b, double epsilon = 0.000001)
    {
        return std::abs(a - b) <= epsilon;
    }
}

int main()
{
    const ftk::RangeD zoomRange(0.01, 100.0);

    // Exact fit transfers between same-aspect clips with different resolutions.
    {
        djv::app::RelativeView view;
        const bool ok = djv::app::getRelativeView(
            ftk::Size2I(1280, 720),
            ftk::Size2I(1280, 720),
            ftk::Size2I(1920, 1080),
            ftk::Size2I(640, 360),
            ftk::V2I(0, 0),
            2.0 / 3.0,
            zoomRange,
            view);
        check(ok, "same-aspect fit transfer succeeds");
        check(close(view.zoom, 2.0), "640x360 clip receives a 2x fit zoom");
        check(view.pos == ftk::V2I(0, 0), "same-aspect fit remains centered");
        check(
            close(1920.0 * (2.0 / 3.0), 640.0 * view.zoom),
            "different resolutions have the same displayed width");
    }

    // A custom relative zoom remains custom instead of snapping back to fit.
    {
        djv::app::RelativeView view;
        const bool ok = djv::app::getRelativeView(
            ftk::Size2I(1280, 720),
            ftk::Size2I(1280, 720),
            ftk::Size2I(1920, 1080),
            ftk::Size2I(640, 360),
            ftk::V2I(-320, -180),
            1.0,
            zoomRange,
            view);
        check(ok, "custom zoom transfer succeeds");
        check(close(view.zoom, 3.0), "custom zoom keeps its factor relative to fit");
        check(view.pos == ftk::V2I(-320, -180), "normalized center is preserved");
    }

    // Unusual aspect ratios still preserve the same fit-relative scale.
    {
        djv::app::RelativeView view;
        const bool ok = djv::app::getRelativeView(
            ftk::Size2I(1280, 720),
            ftk::Size2I(1280, 720),
            ftk::Size2I(1920, 1080),
            ftk::Size2I(1080, 1920),
            ftk::V2I(-320, -180),
            1.0,
            zoomRange,
            view);
        check(ok, "landscape-to-portrait transfer succeeds");
        check(close(view.zoom, 0.5625), "portrait clip keeps 150% of fit");
        check(view.pos == ftk::V2I(336, -180), "portrait clip keeps the normalized center");
    }

    // Resizing the viewport also preserves the semantic zoom level.
    {
        djv::app::RelativeView view;
        const bool ok = djv::app::getRelativeView(
            ftk::Size2I(1280, 720),
            ftk::Size2I(1000, 800),
            ftk::Size2I(640, 360),
            ftk::Size2I(640, 360),
            ftk::V2I(0, 0),
            2.0,
            zoomRange,
            view);
        check(ok, "viewport resize transfer succeeds");
        check(close(view.zoom, 1.5625), "viewport resize recomputes fit-relative zoom");
        check(view.pos == ftk::V2I(0, 119), "resized view remains centered");
    }

    // Invalid input must not mutate a live view.
    {
        djv::app::RelativeView view;
        check(
            !djv::app::getRelativeView(
                ftk::Size2I(),
                ftk::Size2I(1280, 720),
                ftk::Size2I(1920, 1080),
                ftk::Size2I(640, 360),
                ftk::V2I(),
                1.0,
                zoomRange,
                view),
            "invalid viewport size is rejected");
    }

    // The new setting must round-trip while old settings remain compatible.
    {
        djv::models::TimelineSettings settings;
        settings.relativeZoom = true;
        nlohmann::json json;
        djv::models::to_json(json, settings);
        check(json.at("RelativeZoom").get<bool>(), "relative zoom is serialized");

        djv::models::TimelineSettings restored;
        djv::models::from_json(json, restored);
        check(restored.relativeZoom, "relative zoom round-trips");

        json.erase("RelativeZoom");
        djv::models::TimelineSettings legacy;
        djv::models::from_json(json, legacy);
        check(!legacy.relativeZoom, "legacy settings default to absolute zoom");
    }

    if (failures)
    {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "All relative view tests passed\n";
    return 0;
}
