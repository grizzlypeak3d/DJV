// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <ftk/UI/IContainer.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace djv
{
    namespace app
    {
        class App;

        //! Status indicator widget.
        //!
        //! Shows whether any options are enabled that can affect video, audio,
        //! or performance, and opens a popup listing them.
        class DJV_API_TYPE Indicator : public ftk::IContainer
        {
            FTK_NON_COPYABLE(Indicator);

        protected:
            DJV_API void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            DJV_API Indicator();

        public:
            DJV_API virtual ~Indicator();

            //! Create a new widget.
            DJV_API static std::shared_ptr<Indicator> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        protected:
            DJV_API virtual bool _hasIndicator() const;
            DJV_API virtual std::vector<std::pair<std::string, std::string> > _getIndicators() const;
            DJV_API virtual std::map<std::string, bool> _getIndicatorValues() const;
            void _indicatorUpdate();

        private:
            void _showIndicatorPopup();

            FTK_PRIVATE();
        };
    }
}
