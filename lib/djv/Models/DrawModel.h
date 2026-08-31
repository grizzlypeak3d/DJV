// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

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
        class DJV_MODELS_API_TYPE DrawModel : public std::enable_shared_from_this<DrawModel>
        {
            FTK_NON_COPYABLE(DrawModel);

        protected:
            DJV_MODELS_API void _init(const std::shared_ptr<ftk::Settings>&);

            DrawModel();

        public:
            ~DrawModel();

            //! Create a new model.
            DJV_MODELS_API static std::shared_ptr<DrawModel> create(
                const std::shared_ptr<ftk::Settings>&);

            //! Is drawing active? While it is, the viewport draws instead of
            //! shuttling frames with the left mouse button.
            DJV_MODELS_API bool isEnabled() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<bool> > observeEnabled() const;
            DJV_MODELS_API void setEnabled(bool);

            //! The current tool.
            DJV_MODELS_API DrawTool getTool() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<DrawTool> > observeTool() const;
            DJV_MODELS_API void setTool(DrawTool);

            //! The stroke colour.
            DJV_MODELS_API const ftk::Color4F& getColor() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<ftk::Color4F> > observeColor() const;
            DJV_MODELS_API void setColor(const ftk::Color4F&);

            //! The stroke width, in the pixels of the source image.
            DJV_MODELS_API float getSize() const;
            DJV_MODELS_API std::shared_ptr<ftk::IObservable<float> > observeSize() const;
            DJV_MODELS_API void setSize(float);

        private:
            FTK_PRIVATE();
        };
    }
}
