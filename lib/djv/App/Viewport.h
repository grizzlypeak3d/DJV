// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/Models/Export.h>

#include <tlRender/UI/Viewport.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Viewport.
        class DJV_APP_API_TYPE Viewport : public tl::ui::Viewport
        {
            FTK_NON_COPYABLE(Viewport);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            Viewport();

        public:
            DJV_APP_API virtual ~Viewport();

            DJV_APP_API static std::shared_ptr<Viewport> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Observe picking. Unset when the position is not over an
            //! image, where there is no pixel to name.
            DJV_APP_API std::shared_ptr<ftk::IObservable<std::optional<ftk::V2I> > > observePick() const;

            //! Observe the sample position.
            DJV_APP_API std::shared_ptr<ftk::IObservable<ftk::V2I> > observeSamplePos() const;

            //! Observe the color sample. Unset when the position is not over
            //! an image.
            DJV_APP_API std::shared_ptr<ftk::IObservable<std::optional<ftk::Color4F> > > observeColorSample() const;

            //! Sample the image at the given image pixel, as the pick mouse
            //! action would. Used by the documentation screenshot capture.
            DJV_APP_API void pick(const ftk::V2I& imagePos);

            DJV_APP_API void setPlayer(const std::shared_ptr<tl::Player>&) override;

            //! Set whether the toast stands in for the status bar.
            DJV_APP_API void setToastActive(bool);

            //! Set whether the heads up display is shown at all. Separate from
            //! the display's own options so that turning it off for
            //! presentation does not forget what was being shown.
            DJV_APP_API void setHUDActive(bool);

            DJV_APP_API ftk::Size2I getSizeHint() const override;
            DJV_APP_API void tickEvent(bool, bool, const ftk::TickEvent&) override;
            DJV_APP_API void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;
            DJV_APP_API void setGeometry(const ftk::Box2I&) override;
            DJV_APP_API void mouseMoveEvent(ftk::MouseMoveEvent&) override;
            DJV_APP_API void mousePressEvent(ftk::MouseClickEvent&) override;
            DJV_APP_API void mouseReleaseEvent(ftk::MouseClickEvent&) override;

        private:
            bool _getSourceBox(ftk::Box2I&, ftk::Size2I&) const;
            std::optional<ftk::V2I> _toSourcePixel(const ftk::V2I&) const;
            ftk::V2I _fromSourcePixel(const ftk::V2I&) const;
            void _sampleUpdate();
            void _videoUpdate();
            void _toastUpdate();
            void _compareUpdate();
            void _hudUpdate();
            void _hudLayout();

            FTK_PRIVATE();
        };
    }
}

