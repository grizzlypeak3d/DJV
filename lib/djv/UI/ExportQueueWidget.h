// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <ftk/UI/IContainer.h>

namespace djv
{
    namespace models
    {
        class ExportQueue;
    }

    namespace ui
    {
        //! The exports queued and running: one row each, with what it is
        //! called, how far it has got, and a button to cancel it, or to
        //! take it off the list once it has stopped. Hidden while the queue
        //! is empty.
        class DJV_UI_API_TYPE ExportQueueWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ExportQueueWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ExportQueue>&,
                const std::shared_ptr<IWidget>& parent);

            ExportQueueWidget();

        public:
            DJV_UI_API virtual ~ExportQueueWidget();

            DJV_UI_API static std::shared_ptr<ExportQueueWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ExportQueue>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
