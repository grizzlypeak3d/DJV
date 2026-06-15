// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IWidget.h>

#include <memory>
#include <string>

namespace djv
{
    namespace ui
    {
        namespace detail
        {
            //! Property key under which a screenshot tag is stored. This is an
            //! implementation detail: widgets tag themselves with
            //! setScreenshotTag(), and the documentation capture system is the
            //! only reader.
            const std::string screenshotKey = "Screenshot";
        }

        //! Tag a widget so the documentation screenshot capture can find it and
        //! emit its bounding box. The tag is a stable, semantic id, e.g.
        //! "MainWindow.Viewport". Lives in djvUI so any djvUI or djvApp widget
        //! can tag itself (or its children) with a single call.
        inline void setScreenshotTag(
            const std::shared_ptr<ftk::IWidget>& widget,
            const std::string& id)
        {
            if (widget)
            {
                widget->setProperty(detail::screenshotKey, id);
            }
        }
    }
}
