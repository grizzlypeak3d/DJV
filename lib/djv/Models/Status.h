// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/ViewportModel.h>

#include <tlRender/Timeline/ColorOptions.h>
#include <tlRender/Timeline/CompareOptions.h>
#include <tlRender/Timeline/DisplayOptions.h>
#include <tlRender/Timeline/ForegroundOptions.h>

#include <map>
#include <string>
#include <vector>

namespace djv
{
    namespace models
    {
        //! An option that changes what is seen or heard from the media as
        //! it is, which the status indicator reminds of: by an id, which
        //! the applications find its controls by, and a name.
        struct DJV_MODELS_API_TYPE StatusItem
        {
            std::string id;
            std::string label;
        };

        //! Get the options the status indicator knows, in the order it
        //! lists them.
        DJV_MODELS_API std::vector<StatusItem> getStatusItems();

        //! What the options that the status indicator watches are.
        struct DJV_MODELS_API_TYPE StatusOptions
        {
            tl::DisplayOptions    display;
            tl::ForegroundOptions foreground;
            AspectRatioOptions    aspectRatio;
            tl::OCIOOptions       ocio;
            tl::LUTOptions        lut;
            double                audioOffset = 0.0;
            int                   audioChannel = -1;
            tl::Compare           compare = tl::Compare::None;
            bool                  heldFrame = false;
        };

        //! Get whether each option is on, by its id: on where it changes
        //! the picture or the sound. An adjustment enabled at its defaults
        //! is neutral, and not on; OpenColorIO is on with a display and a
        //! view to transform to, as the renderer decides; a LUT with a file;
        //! a held frame while a compare mode draws it, since with none the
        //! view shows A alone.
        DJV_MODELS_API std::map<std::string, bool> getStatus(const StatusOptions&);
    }
}
