// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <tlRender/UI/Viewport.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Viewport.
        class Viewport : public tl::ui::Viewport
        {
            FTK_NON_COPYABLE(Viewport);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            Viewport();

        public:
            virtual ~Viewport();

            static std::shared_ptr<Viewport> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Observe picking. Unset when the position is not over an
            //! image, where there is no pixel to name.
            TL_API std::shared_ptr<ftk::IObservable<std::optional<ftk::V2I> > > observePick() const;

            //! Observe the sample position.
            TL_API std::shared_ptr<ftk::IObservable<ftk::V2I> > observeSamplePos() const;

            //! Observe the color sample. Unset when the position is not over
            //! an image.
            TL_API std::shared_ptr<ftk::IObservable<std::optional<ftk::Color4F> > > observeColorSample() const;

            //! Sample the image at the given image pixel, as the pick mouse
            //! action would. Used by the documentation screenshot capture.
            void pick(const ftk::V2I& imagePos);

            void setPlayer(const std::shared_ptr<tl::Player>&) override;

            //! Set whether the toast stands in for the status bar.
            void setToastActive(bool);

            ftk::Size2I getSizeHint() const override;
            void tickEvent(bool, bool, const ftk::TickEvent&) override;
            void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;
            void setGeometry(const ftk::Box2I&) override;
            void mouseMoveEvent(ftk::MouseMoveEvent&) override;
            void mousePressEvent(ftk::MouseClickEvent&) override;
            void mouseReleaseEvent(ftk::MouseClickEvent&) override;

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

