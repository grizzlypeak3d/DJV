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
        //! the applications find its controls by, a name, and the group it
        //! is shown in.
        struct DJV_MODELS_API_TYPE StatusItem
        {
            std::string id;
            std::string label;
            std::string group;
        };

        //! Get the options the status indicator knows, in the order it
        //! lists them, a group's options together.
        DJV_MODELS_API std::vector<StatusItem> getStatusItems();

        //! A group of options, which the status indicator shows as a
        //! letter, lit while any of them is on: always in the same place,
        //! so what is on can be seen at a glance without reading.
        struct DJV_MODELS_API_TYPE StatusGroup
        {
            std::string id;
            std::string letter;
            std::string label;
        };

        //! Get the groups, in the order the status indicator shows them.
        DJV_MODELS_API std::vector<StatusGroup> getStatusGroups();

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
            bool                  bFiles = false;
            bool                  storedFrame = false;
        };

        //! Get whether each option is on, by its id: on where it changes
        //! the picture or the sound. An adjustment enabled at its defaults
        //! is neutral, and not on; OpenColorIO is on with a display and a
        //! view to transform to, as the renderer decides; a LUT with a file;
        //! a stored frame while a compare mode draws it, since with none the
        //! view shows A alone; a comparison while it has B files to draw,
        //! and no stored frame, which stands in for them.
        DJV_MODELS_API std::map<std::string, bool> getStatus(const StatusOptions&);
    }
}
