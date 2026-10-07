// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/DrawModel.h>

#include <ftk/UI/Settings.h>

namespace djv
{
    namespace models
    {
        FTK_ENUM_IMPL(
            DrawTool,
            "Pen",
            "Eraser",
            "Line",
            "Arrow",
            "Rectangle",
            "Ellipse",
            "Text",
            "Select");

        ReviewStrokeKind getStrokeKind(DrawTool value)
        {
            ReviewStrokeKind out = ReviewStrokeKind::Freehand;
            switch (value)
            {
            case DrawTool::Line: out = ReviewStrokeKind::Line; break;
            case DrawTool::Arrow: out = ReviewStrokeKind::Arrow; break;
            case DrawTool::Rectangle: out = ReviewStrokeKind::Rectangle; break;
            case DrawTool::Ellipse: out = ReviewStrokeKind::Ellipse; break;
            case DrawTool::Text: out = ReviewStrokeKind::Text; break;
            default: break;
            }
            return out;
        }

        struct DrawModel::Private
        {
            std::shared_ptr<ftk::Settings> settings;
            std::shared_ptr<ftk::Observable<bool> > enabled;
            std::shared_ptr<ftk::Observable<DrawTool> > tool;
            std::shared_ptr<ftk::Observable<ftk::Color4F> > color;
            std::shared_ptr<ftk::Observable<float> > size;
            std::shared_ptr<ftk::Observable<float> > textSize;
            std::shared_ptr<ftk::Observable<bool> > onionSkin;
        };

        void DrawModel::_init(const std::shared_ptr<ftk::Settings>& settings)
        {
            FTK_P();

            p.settings = settings;

            // Drawing always starts off: it takes over the left mouse button.
            p.enabled = ftk::Observable<bool>::create(false);
            p.tool = ftk::Observable<DrawTool>::create(DrawTool::Pen);

            // A warm orange reads on both dark and bright footage; a dark
            // default would be invisible on dark shots.
            ftk::Color4F color(1.F, .365F, .02F, 1.F);
            p.settings->getT("/Draw/Color", color);
            p.color = ftk::Observable<ftk::Color4F>::create(color);

            float size = 4.F;
            p.settings->get("/Draw/Size", size);
            p.size = ftk::Observable<float>::create(size > 0.F ? size : 4.F);
            float textSize = 48.F;
            p.settings->get("/Draw/TextSize", textSize);
            p.textSize = ftk::Observable<float>::create(textSize > 0.F ? textSize : 48.F);
            bool onionSkin = false;
            p.settings->get("/Draw/OnionSkin", onionSkin);
            p.onionSkin = ftk::Observable<bool>::create(onionSkin);
        }

        DrawModel::DrawModel() :
            _p(new Private)
        {}

        DrawModel::~DrawModel()
        {
            FTK_P();
            p.settings->setT("/Draw/Color", p.color->get());
            p.settings->set("/Draw/Size", p.size->get());
            p.settings->set("/Draw/TextSize", p.textSize->get());
            p.settings->set("/Draw/OnionSkin", p.onionSkin->get());
        }

        std::shared_ptr<DrawModel> DrawModel::create(
            const std::shared_ptr<ftk::Settings>& settings)
        {
            auto out = std::shared_ptr<DrawModel>(new DrawModel);
            out->_init(settings);
            return out;
        }

        bool DrawModel::isEnabled() const
        {
            return _p->enabled->get();
        }

        std::shared_ptr<ftk::IObservable<bool> > DrawModel::observeEnabled() const
        {
            return _p->enabled;
        }

        void DrawModel::setEnabled(bool value)
        {
            _p->enabled->setIfChanged(value);
        }

        DrawTool DrawModel::getTool() const
        {
            return _p->tool->get();
        }

        std::shared_ptr<ftk::IObservable<DrawTool> > DrawModel::observeTool() const
        {
            return _p->tool;
        }

        void DrawModel::setTool(DrawTool value)
        {
            _p->tool->setIfChanged(value);
        }

        const ftk::Color4F& DrawModel::getColor() const
        {
            return _p->color->get();
        }

        std::shared_ptr<ftk::IObservable<ftk::Color4F> > DrawModel::observeColor() const
        {
            return _p->color;
        }

        void DrawModel::setColor(const ftk::Color4F& value)
        {
            _p->color->setIfChanged(value);
        }

        float DrawModel::getSize() const
        {
            return _p->size->get();
        }

        std::shared_ptr<ftk::IObservable<float> > DrawModel::observeSize() const
        {
            return _p->size;
        }

        void DrawModel::setSize(float value)
        {
            // A zero width would draw nothing.
            _p->size->setIfChanged(std::max(.5F, value));
        }

        float DrawModel::getTextSize() const
        {
            return _p->textSize->get();
        }

        std::shared_ptr<ftk::IObservable<float> > DrawModel::observeTextSize() const
        {
            return _p->textSize;
        }

        void DrawModel::setTextSize(float value)
        {
            _p->textSize->setIfChanged(std::max(4.F, value));
        }

        bool DrawModel::isOnionSkin() const
        {
            return _p->onionSkin->get();
        }

        std::shared_ptr<ftk::IObservable<bool> > DrawModel::observeOnionSkin() const
        {
            return _p->onionSkin;
        }

        void DrawModel::setOnionSkin(bool value)
        {
            _p->onionSkin->setIfChanged(value);
        }
    }
}
