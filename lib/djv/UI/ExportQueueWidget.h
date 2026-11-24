// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <ftk/UI/IContainer.h>

#include <ftk/Core/Path.h>

namespace djv
{
    namespace models
    {
        class ExportQueue;
    }

    namespace ui
    {
        //! The exports: the running one first, then the ones waiting in
        //! the order they will run, then the ones that have stopped, newest
        //! first. One row each, with what it is called, how far it has got,
        //! and a button to cancel it, or to take it off the list once it
        //! has stopped; one that has finished can also be opened, to look
        //! at what was written. Under them, once two have finished, how
        //! long the finished ones took in all. Hidden while the queue is
        //! empty.
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

            //! Set what opens a finished export; without it there is no
            //! button for it.
            DJV_UI_API void setOpenCallback(const std::function<void(const ftk::Path&)>&);

            DJV_UI_API void mouseEnterEvent(ftk::MouseEnterEvent&) override;
            DJV_UI_API void mouseLeaveEvent() override;

        private:
            void _jobsUpdate();
            void _orderUpdate();
            void _totalUpdate();

            FTK_PRIVATE();
        };
    }
}
