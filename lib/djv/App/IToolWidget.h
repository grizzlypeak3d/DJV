// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/ToolsModel.h>

#include <ftk/UI/Bellows.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Base class for tool widgets.
        class IToolWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(IToolWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                models::Tool,
                const std::string& objectName,
                const std::shared_ptr<IWidget>& parent);

            IToolWidget();

        public:
            virtual ~IToolWidget() = 0;

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        protected:
            void _loadSettings(const std::map<std::string, std::shared_ptr<ftk::Bellows> >&);
            void _saveSettings(const std::map<std::string, std::shared_ptr<ftk::Bellows> >&);

            void _setWidget(const std::shared_ptr<ftk::IWidget>&);

            std::weak_ptr<App> _app;

        private:
            FTK_PRIVATE();
        };
    }
}
