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

            //! Observe picking.
            TL_API std::shared_ptr<ftk::IObservable<ftk::V2I> > observePick() const;

            //! Observe the sample position.
            TL_API std::shared_ptr<ftk::IObservable<ftk::V2I> > observeSamplePos() const;

            //! Observe the color sample.
            TL_API std::shared_ptr<ftk::IObservable<ftk::Color4F> > observeColorSample() const;

            //! Sample the image at the given image pixel, as the pick mouse
            //! action would. Used by the documentation screenshot capture.
            void pick(const ftk::V2I& imagePos);

            void setPlayer(const std::shared_ptr<tl::Player>&) override;

            //! Set whether the toast stands in for the status bar.
            void setToastActive(bool);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;
            void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;
            void mouseMoveEvent(ftk::MouseMoveEvent&) override;
            void mousePressEvent(ftk::MouseClickEvent&) override;
            void mouseReleaseEvent(ftk::MouseClickEvent&) override;

        private:
            bool _getSourceBox(ftk::Box2I&, ftk::Size2I&) const;
            ftk::V2I _toSourcePixel(const ftk::V2I&) const;
            ftk::V2I _fromSourcePixel(const ftk::V2I&) const;

            //! Where a position in the viewport falls in the sources.
            struct SourceHit
            {
                //! Index into the active files, or -1 if the position is outside
                //! every source.
                int      index = -1;

                //! The position in the pixels of that source's image.
                ftk::V2F pos;

                //! Image pixels per render unit, for stroke widths.
                float    scale = 1.F;
            };

            //! Map a widget-local position to a source and its image pixels.
            SourceHit _hitTest(const ftk::V2I& widgetPos) const;

            //! Map a position in a source's image pixels back to widget-local
            //! coordinates.
            ftk::V2F _imageToWidget(int index, const ftk::V2F& imagePos) const;

            //! The boxes of the sources in render space, as used for drawing.
            std::vector<ftk::Box2I> _sourceBoxes() const;

            void _drawBegin(const ftk::V2I& widgetPos);
            void _drawContinue(const ftk::V2I& widgetPos);
            void _drawEnd();
            void _erase(const ftk::V2I& widgetPos);

            void _videoUpdate();
            void _toastUpdate();
            void _hudUpdate();
            void _hudLayout();

            FTK_PRIVATE();
        };
    }
}

