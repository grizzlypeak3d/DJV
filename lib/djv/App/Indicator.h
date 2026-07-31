// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IWidget.h>

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
        class Indicator : public ftk::IWidget
        {
            FTK_NON_COPYABLE(Indicator);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            Indicator();

        public:
            virtual ~Indicator();

            //! Create a new widget.
            static std::shared_ptr<Indicator> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        protected:
            virtual bool _hasIndicator() const;
            virtual std::vector<std::pair<std::string, std::string> > _getIndicators() const;
            virtual std::map<std::string, bool> _getIndicatorValues() const;
            void _indicatorUpdate();

        private:
            void _showIndicatorPopup();

            FTK_PRIVATE();
        };
    }
}
