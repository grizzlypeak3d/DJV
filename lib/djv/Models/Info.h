// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <tlRender/IO/IO.h>

#include <ftk/Core/Path.h>

#include <string>
#include <vector>

namespace djv
{
    namespace models
    {
        //! A line of information about a file: its section (one of
        //! getInfoSections()), a name and a value.
        struct DJV_MODELS_API_TYPE InfoItem
        {
            std::string section;
            std::string name;
            std::string value;

            bool operator == (const InfoItem&) const = default;
        };

        //! The information sections, in the order they are shown: File,
        //! Video, Audio, Metadata.
        DJV_MODELS_API const std::vector<std::string>& getInfoSections();

        //! The information about a file, section by section: its name and
        //! directory, the video it holds (of the layer shown) and is decoded
        //! to, the audio, and the file's own metadata. What the Info tool
        //! shows, on the desktop and the iPad.
        DJV_MODELS_API std::vector<InfoItem> getInfo(
            const ftk::Path&,
            const tl::IOInfo&,
            int videoLayer = 0);

        //! The items matching a search. Each word stands on its own, and an
        //! item is kept when any of them is in its name or its value, case
        //! ignored: "name duration" keeps both rows (#316). An empty search
        //! keeps everything.
        DJV_MODELS_API std::vector<InfoItem> filterInfo(
            const std::vector<InfoItem>&,
            const std::string& search);
    }
}
