// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/Core/Observable.h>
#include <ftk/Core/Color.h>

namespace ftk
{
    class Settings;
}

namespace djv
{
    namespace models
    {
        //! Drawing tools.
        enum class DrawTool
        {
            Pen,
            Eraser
        };

        //! The drawing state: whether drawing is active, and with what.
        //!
        //! These are preferences rather than review contents, so they are kept
        //! in the settings and not in the review file.
        class DrawModel : public std::enable_shared_from_this<DrawModel>
        {
            FTK_NON_COPYABLE(DrawModel);

        protected:
            void _init(const std::shared_ptr<ftk::Settings>&);

            DrawModel();

        public:
            ~DrawModel();

            //! Create a new model.
            static std::shared_ptr<DrawModel> create(
                const std::shared_ptr<ftk::Settings>&);

            //! Is drawing active? While it is, the viewport draws instead of
            //! shuttling frames with the left mouse button.
            bool isEnabled() const;
            std::shared_ptr<ftk::IObservable<bool> > observeEnabled() const;
            void setEnabled(bool);

            //! The current tool.
            DrawTool getTool() const;
            std::shared_ptr<ftk::IObservable<DrawTool> > observeTool() const;
            void setTool(DrawTool);

            //! The stroke colour.
            const ftk::Color4F& getColor() const;
            std::shared_ptr<ftk::IObservable<ftk::Color4F> > observeColor() const;
            void setColor(const ftk::Color4F&);

            //! The stroke width, in the pixels of the source image.
            float getSize() const;
            std::shared_ptr<ftk::IObservable<float> > observeSize() const;
            void setSize(float);

        private:
            FTK_PRIVATE();
        };
    }
}
