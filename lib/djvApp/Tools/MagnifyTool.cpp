// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/MagnifyTool.h>

#include <djvApp/Models/ColorModel.h>
#include <djvApp/Models/FilesModel.h>
#include <djvApp/Models/ViewportModel.h>
#include <djvApp/Widgets/Viewport.h>
#include <djvApp/App.h>
#include <djvApp/MainWindow.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/ComboBox.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/Settings.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        FTK_ENUM_IMPL(
            MagnifyLevel,
            "2X",
            "4X",
            "8X",
            "16X",
            "32X",
            "64X",
            "128X");

        int getMagnifyLevel(MagnifyLevel value)
        {
            const std::array<int, static_cast<size_t>(MagnifyLevel::Count)> data =
            {
                2, 4, 8, 16, 32, 64, 128
            };
            return data[static_cast<size_t>(value)];
        }

        struct MagnifyTool::Private
        {
            std::shared_ptr<ftk::Settings> settings;
            std::weak_ptr<MainWindow> mainWindow;

            MagnifyLevel level = MagnifyLevel::_4X;
            ftk::V2I viewPos;
            double viewZoom = 1.0;
            ftk::V2I pick;
            ftk::V2I samplePos;
            size_t videoFramesSize = 0;
            ftk::ImageOptions imageOptions;
            tl::DisplayOptions displayOptions;
            bool sizeInit = true;

            std::shared_ptr<tl::ui::Viewport> viewport;
            std::shared_ptr<ftk::ComboBox> comboBox;
            std::shared_ptr<ftk::Label> pixelLabel;
            std::shared_ptr<ftk::Label> mouseLabel;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::ListObserver<tl::VideoFrame> > videoObserver;
            std::shared_ptr<ftk::Observer<std::pair<ftk::V2I, double> > > viewPosAndZoomObserver;
            std::shared_ptr<ftk::Observer<ftk::V2I> > pickObserver;
            std::shared_ptr<ftk::Observer<ftk::V2I> > samplePosObserver;
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageOptions> > imageOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::BackgroundOptions> > bgOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > fgOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageType> > colorBufferObserver;
            std::shared_ptr<ftk::Observer<MouseSettings> > settingsObserver;
        };

        void MagnifyTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                Tool::Magnify,
                "djv::app::MagnifyTool",
                parent);
            FTK_P();

            p.settings = app->getSettings();
            std::string s;
            p.settings->get("/Magnify/Level", s);
            from_string(s, p.level);

            p.mainWindow = mainWindow;

            p.viewport = tl::ui::Viewport::create(context);
            p.viewport->setInputEnabled(false);

            p.comboBox = ftk::ComboBox::create(context, getMagnifyLevelLabels());

            p.pixelLabel = ftk::Label::create(context);
            p.pixelLabel->setFontRole(ftk::FontRole::Mono);

            p.mouseLabel = ftk::Label::create(context);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);
            p.viewport->setParent(layout);
            auto formLayout = ftk::FormLayout::create(context, layout);
            formLayout->setMarginRole(ftk::SizeRole::Margin);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Magnify:", p.comboBox);
            formLayout->addRow("Pixel:", p.pixelLabel);
            formLayout->addRow("Mouse:", p.mouseLabel);
            _setWidget(layout);

            p.comboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    p.level = static_cast<MagnifyLevel>(value);
                    _widgetUpdate();
                });

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    FTK_P();
                    p.viewport->setPlayer(value);
                    if (value)
                    {
                        p.videoObserver = ftk::ListObserver<tl::VideoFrame>::create(
                            value->observeCurrentVideo(),
                            [this](const std::vector<tl::VideoFrame>& value)
                            {
                                _p->videoFramesSize = value.size();
                                _videoUpdate();
                            });
                    }
                    else
                    {
                        p.videoFramesSize = 0;
                        p.videoObserver.reset();
                        _videoUpdate();
                    }
                });

            p.viewPosAndZoomObserver = ftk::Observer<std::pair<ftk::V2I, double> >::create(
                mainWindow->getViewport()->observeViewPosAndZoom(),
                [this](const std::pair<ftk::V2I, double>& value)
                {
                    FTK_P();
                    p.viewPos = value.first;
                    p.viewZoom = value.second;
                    _widgetUpdate();
                });

            p.pickObserver = ftk::Observer<ftk::V2I>::create(
                mainWindow->getViewport()->observePick(),
                [this](const ftk::V2I& value)
                {
                    FTK_P();
                    p.pick = value;
                    _widgetUpdate();
                });

            p.samplePosObserver = ftk::Observer<ftk::V2I>::create(
                mainWindow->getViewport()->observeSamplePos(),
                [this](const ftk::V2I& value)
                {
                    FTK_P();
                    p.samplePos = value;
                    _widgetUpdate();
                });

            p.compareOptionsObserver = ftk::Observer<tl::CompareOptions>::create(
                app->getFilesModel()->observeCompareOptions(),
                [this](const tl::CompareOptions& value)
                {
                    _p->viewport->setCompareOptions(value);
                });

            p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                app->getColorModel()->observeOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                    _p->viewport->setOCIOOptions(value);
                });

            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                app->getColorModel()->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                    _p->viewport->setLUTOptions(value);
                });

            p.imageOptionsObserver = ftk::Observer<ftk::ImageOptions>::create(
                app->getViewportModel()->observeImageOptions(),
                [this](const ftk::ImageOptions& value)
                {
                    _p->imageOptions = value;
                    _videoUpdate();
                });

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                app->getViewportModel()->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    _p->displayOptions = value;
                    _videoUpdate();
                });

            p.bgOptionsObserver = ftk::Observer<tl::BackgroundOptions>::create(
                app->getViewportModel()->observeBackgroundOptions(),
                [this](const tl::BackgroundOptions& value)
                {
                    _p->viewport->setBackgroundOptions(value);
                });

            p.fgOptionsObserver = ftk::Observer<tl::ForegroundOptions>::create(
                app->getViewportModel()->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
                {
                    _p->viewport->setForegroundOptions(value);
                });

            p.colorBufferObserver = ftk::Observer<ftk::ImageType>::create(
                app->getViewportModel()->observeColorBuffer(),
                [this](ftk::ImageType value)
                {
                    _p->viewport->setColorBuffer(value);
                });

            p.settingsObserver = ftk::Observer<MouseSettings>::create(
                app->getSettingsModel()->observeMouse(),
                [this](const MouseSettings& value)
                {
                    std::vector<std::string> s;
                    auto i = value.bindings.find(MouseAction::Pick);
                    if (i != value.bindings.end())
                    {
                        if (i->second.button != ftk::MouseButton::None)
                        {
                            if (i->second.modifier != ftk::KeyModifier::None)
                            {
                                s.push_back(ftk::to_string(i->second.modifier));
                            }
                            s.push_back(ftk::getLabel(i->second.button));
                        }
                    }
                    _p->mouseLabel->setText(
                        ftk::Format("{0} Click").arg(ftk::join(s, " + ")));
                });
        }

        MagnifyTool::MagnifyTool() :
            _p(new Private)
        {}

        MagnifyTool::~MagnifyTool()
        {
            FTK_P();
            p.settings->set("/Magnify/Level", to_string(p.level));
        }

        std::shared_ptr<MagnifyTool> MagnifyTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<MagnifyTool>(new MagnifyTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }

        void MagnifyTool::setGeometry(const ftk::Box2I& value)
        {
            IToolWidget::setGeometry(value);
            FTK_P();
            if (p.sizeInit)
            {
                p.sizeInit = false;
                _widgetUpdate();
            }
        }

        void MagnifyTool::_widgetUpdate()
        {
            FTK_P();
            const ftk::Box2I& g = getGeometry();
            const int level = getMagnifyLevel(p.level);
            const ftk::V2I magnifyPos =
                (p.viewPos - p.samplePos) * level + (center(g) - g.min);

            const double magnifyZoom = p.viewZoom * level;
            p.viewport->setViewPosAndZoom(magnifyPos, magnifyZoom);

            p.comboBox->setCurrentIndex(static_cast<int>(p.level));

            p.pixelLabel->setText(ftk::Format("{0}").arg(p.pick));
        }

        void MagnifyTool::_videoUpdate()
        {
            FTK_P();
            std::vector<ftk::ImageOptions> imageOptions;
            std::vector<tl::DisplayOptions> displayOptions;
            for (size_t i = 0; i < p.videoFramesSize; ++i)
            {
                imageOptions.push_back(p.imageOptions);
                displayOptions.push_back(p.displayOptions);
            }
            p.viewport->setImageOptions(imageOptions);
            p.viewport->setDisplayOptions(displayOptions);
        }
    }
}
