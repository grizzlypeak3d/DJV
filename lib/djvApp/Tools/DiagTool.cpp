// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/DiagTool.h>

#include <tlRender/Timeline/Player.h>
#include <tlRender/IO/Plugin.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/GraphWidget.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/GL/Mesh.h>
#include <ftk/GL/OffscreenBuffer.h>
#include <ftk/GL/Shader.h>
#include <ftk/GL/Texture.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>
#include <ftk/Core/Timer.h>

namespace djv
{
    namespace app
    {
        struct DiagTool::Private
        {
            std::map<std::string, std::shared_ptr<ftk::GraphWidget> > graphs;
            std::shared_ptr<ftk::Timer> timer;
        };

        void DiagTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                Tool::Diag,
                "djv::app::DiagTool",
                parent);
            FTK_P();

            p.graphs["GLObjects"] = ftk::GraphWidget::create(
                context,
                "OpenGL Objects",
                {
                    { ftk::ColorRole::Cyan, "Meshes: {0}" },
                    { ftk::ColorRole::Magenta, "Textures: {0}" },
                    { ftk::ColorRole::Yellow, "Buffers: {0}" },
                    { ftk::ColorRole::Red, "Shaders: {0}" }
                });

            p.graphs["GLMemory"] = ftk::GraphWidget::create(
                context,
                "OpenGL Memory",
                {
                    { ftk::ColorRole::Cyan, "Meshes: {0}MB" },
                    { ftk::ColorRole::Magenta, "Textures: {0}MB" },
                    { ftk::ColorRole::Yellow, "Buffers: {0}MB" }
                });

            p.graphs["Objects"] = ftk::GraphWidget::create(
                context,
                "Objects",
                {
                    { ftk::ColorRole::Cyan, "Players: {0}" },
                    { ftk::ColorRole::Magenta, "Timelines: {0}" },
                    { ftk::ColorRole::Yellow, "IO: {0}" },
                    { ftk::ColorRole::Red, "Images: {0}" },
                    { ftk::ColorRole::Green, "Audio: {0}" },
                    { ftk::ColorRole::Blue, "Widgets: {0}" }
                });

            p.graphs["Memory"] = ftk::GraphWidget::create(
                context,
                "Memory",
                {
                    { ftk::ColorRole::Cyan, "Images: {0}MB" },
                    { ftk::ColorRole::Magenta, "Audio: {0}MB" }
                });

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);
            p.graphs["GLObjects"]->setParent(layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            p.graphs["GLMemory"]->setParent(layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            p.graphs["Objects"]->setParent(layout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            p.graphs["Memory"]->setParent(layout);

            auto scrollWidget = ftk::ScrollWidget::create(context);
            scrollWidget->setWidget(layout);
            scrollWidget->setBorder(false);
            scrollWidget->setVStretch(ftk::Stretch::Expanding);
            _setWidget(scrollWidget);

            p.timer = ftk::Timer::create(context);
            p.timer->setRepeating(true);
            p.timer->start(
                std::chrono::milliseconds(1000),
                [this]
                {
                    FTK_P();
                    p.graphs["GLObjects"]->addSample(
                        ftk::ColorRole::Cyan,
                        ftk::gl::VBO::getObjectCount());
                    p.graphs["GLObjects"]->addSample(
                        ftk::ColorRole::Magenta,
                        ftk::gl::Texture::getObjectCount());
                    p.graphs["GLObjects"]->addSample(
                        ftk::ColorRole::Yellow,
                        ftk::gl::OffscreenBuffer::getObjectCount());
                    p.graphs["GLObjects"]->addSample(
                        ftk::ColorRole::Red,
                        ftk::gl::Shader::getObjectCount());

                    p.graphs["GLMemory"]->addSample(
                        ftk::ColorRole::Cyan,
                        ftk::gl::VBO::getTotalByteCount() / ftk::megabyte);
                    p.graphs["GLMemory"]->addSample(
                        ftk::ColorRole::Magenta,
                        ftk::gl::Texture::getTotalByteCount() / ftk::megabyte);
                    p.graphs["GLMemory"]->addSample(
                        ftk::ColorRole::Yellow,
                        ftk::gl::OffscreenBuffer::getTotalByteCount() / ftk::megabyte);

                    p.graphs["Objects"]->addSample(
                        ftk::ColorRole::Cyan,
                        tl::Player::getObjectCount());
                    p.graphs["Objects"]->addSample(
                        ftk::ColorRole::Magenta,
                        tl::Timeline::getObjectCount());
                    p.graphs["Objects"]->addSample(
                        ftk::ColorRole::Yellow,
                        tl::IIO::getObjectCount());
                    p.graphs["Objects"]->addSample(
                        ftk::ColorRole::Red,
                        ftk::Image::getObjectCount());
                    p.graphs["Objects"]->addSample(
                        ftk::ColorRole::Green,
                        tl::Audio::getObjectCount());
                    p.graphs["Objects"]->addSample(
                        ftk::ColorRole::Blue,
                        ftk::IWidget::getObjectCount());

                    p.graphs["Memory"]->addSample(
                        ftk::ColorRole::Cyan,
                        ftk::Image::getTotalByteCount() / ftk::megabyte);
                    p.graphs["Memory"]->addSample(
                        ftk::ColorRole::Yellow,
                        tl::Audio::getTotalByteCount() / ftk::megabyte);
                });
        }

        DiagTool::DiagTool() :
            _p(new Private)
        {}

        DiagTool::~DiagTool()
        {}

        std::shared_ptr<DiagTool> DiagTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<DiagTool>(new DiagTool);
            out->_init(context, app, parent);
            return out;
        }
    }
}
