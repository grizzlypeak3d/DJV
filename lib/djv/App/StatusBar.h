// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <tlRender/IO/IO.h>

#include <ftk/UI/IMouseWidget.h>
#include <ftk/Core/LogSystem.h>
#include <ftk/Core/Path.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Status bar widget.
        class StatusBar : public ftk::IMouseWidget
        {
            FTK_NON_COPYABLE(StatusBar);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            StatusBar();

        public:
            virtual ~StatusBar();

            //! Create a new widget.
            static std::shared_ptr<StatusBar> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;
            void mousePressEvent(ftk::MouseClickEvent&) override;
            void mouseReleaseEvent(ftk::MouseClickEvent&) override;

        protected:
            virtual bool _hasIndicator() const;
            virtual std::vector<std::pair<std::string, std::string> > _getIndicators() const;
            virtual std::map<std::string, bool> _getIndicatorValues() const;
            void _indicatorUpdate();
            
        private:
            void _infoUpdate(const ftk::Path&, const tl::IOInfo&);
            void _showIndicatorPopup();

            FTK_PRIVATE();
        };
    }
}
