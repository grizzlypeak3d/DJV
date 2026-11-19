// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>

#include <ftk/UI/IButton.h>

namespace djv
{
    namespace app
    {
        class App;

        //! How far the exports have got, as a ring the size of a letter of
        //! the status indicator, beside it, filling clockwise from the top:
        //! there while exports run, so that they can be followed from
        //! anywhere, and full once they have stopped, red if any failed,
        //! until the export tool is opened. Clicking it shows or hides the
        //! export tool, as the tool bar's buttons do.
        class DJV_APP_API_TYPE ExportIndicator : public ftk::IButton
        {
            FTK_NON_COPYABLE(ExportIndicator);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            ExportIndicator();

        public:
            DJV_APP_API virtual ~ExportIndicator();

            //! Create a new widget.
            DJV_APP_API static std::shared_ptr<ExportIndicator> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set what is told, once, when the exports of a run have all
            //! stopped: "Exports finished: 3", "Exports: 2 finished, 1
            //! failed".
            DJV_APP_API void setFinishedCallback(const std::function<void(const std::string&)>&);

            DJV_APP_API ftk::Size2I getSizeHint() const override;
            DJV_APP_API void sizeHintEvent(const ftk::SizeHintEvent&) override;
            DJV_APP_API void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;

        private:
            void _update();
            std::string _getSummary() const;

            FTK_PRIVATE();
        };
    }
}
