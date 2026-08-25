// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>
#include <djv/Models/Export.h>

#include <ftk/UI/IContainer.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace djv
{
    namespace models
    {
        class AudioModel;
        class ColorModel;
        class ViewportModel;
    }

    namespace ui
    {
        //! Status indicator widget.
        //!
        //! Shows whether any options are enabled that can affect video, audio,
        //! or performance, and opens a popup listing them.
        class DJV_UI_API_TYPE StatusIndicator : public ftk::IContainer
        {
            FTK_NON_COPYABLE(StatusIndicator);

        protected:
            DJV_UI_API void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<models::AudioModel>&,
                const std::shared_ptr<IWidget>& parent);

            DJV_UI_API StatusIndicator();

        public:
            DJV_UI_API virtual ~StatusIndicator();

            //! Create a new widget.
            DJV_UI_API static std::shared_ptr<StatusIndicator> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<models::AudioModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        protected:
            DJV_UI_API virtual bool _hasIndicator() const;
            DJV_UI_API virtual std::vector<std::pair<std::string, std::string> > _getIndicators() const;
            DJV_UI_API virtual std::map<std::string, bool> _getIndicatorValues() const;
            DJV_UI_API void _indicatorUpdate();

        private:
            void _showIndicatorPopup();

            FTK_PRIVATE();
        };
    }
}
