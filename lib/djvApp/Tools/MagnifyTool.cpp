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
            static const std::array<int, static_cast<size_t>(MagnifyLevel::Count)> data =
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

            std::shared_ptr<Viewport> viewport;
            std::shared_ptr<ftk::ComboBox> comboBox;
            std::shared_ptr<ftk::Label> label;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::timeline::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<ftk::V2I> > pickObserver;
            std::shared_ptr<ftk::Observer<std::pair<ftk::V2I, double> > > viewPosAndZoomObserver;
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

            p.viewport = Viewport::create(context, app);
            p.viewport->setInputEnabled(false);

            p.comboBox = ftk::ComboBox::create(context, getMagnifyLevelLabels());

            p.label = ftk::Label::create(context);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);
            p.viewport->setParent(layout);
            auto hLayout = ftk::HorizontalLayout::create(context, layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.comboBox->setParent(hLayout);
            hLayout->addSpacer(ftk::Stretch::Expanding);
            p.label->setParent(hLayout);
            _setWidget(layout);

            p.comboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    p.level = static_cast<MagnifyLevel>(value);
                    _widgetUpdate();
                });

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::timeline::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::timeline::Player>& value)
                {
                    _p->viewport->setPlayer(value);
                });

            p.pickObserver = ftk::Observer<ftk::V2I>::create(
                mainWindow->getViewport()->observePick(),
                [this](const ftk::V2I& value)
                {
                    FTK_P();
                    p.pick = value;
                    _widgetUpdate();
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
                    _p->label->setText(ftk::Format("Mouse binding: {0} Click").
                        arg(ftk::join(s, " + ")));
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

        void MagnifyTool::_widgetUpdate()
        {
            FTK_P();
            const ftk::Box2I& g = getGeometry();
            const int level = getMagnifyLevel(p.level);
            const ftk::V2I magnifyPos =
                (p.viewPos - p.pick) * level + (center(g) - g.min);
            const double magnifyZoom = p.viewZoom * level;
            p.viewport->setViewPosAndZoom(magnifyPos, magnifyZoom);

            p.comboBox->setCurrentIndex(static_cast<int>(p.level));
        }
    }
}
