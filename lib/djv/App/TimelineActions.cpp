// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/TimelineActions.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>

#include <tlRender/UI/TimelineWidget.h>

namespace djv
{
    namespace app
    {
        struct TimelineActions::Private
        {
            std::weak_ptr<MainWindow> mainWindow;

            std::shared_ptr<ftk::Observer<models::TimelineSettings> > settingsObserver;
        };

        void TimelineActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow)
        {
            IActions::_init(context, app, "Timeline");
            FTK_P();

            p.mainWindow = mainWindow;

            auto appWeak = std::weak_ptr<App>(app);
            _actions["Minimize"] = ftk::Action::create(
                "Minimize",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.minimize = value;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _actions["FrameView"] = ftk::Action::create(
                "Frame View",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.frameView = value;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _actions["ScrollBars"] = ftk::Action::create(
                "Scroll Bars",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.scrollBars = value;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _actions["AutoScroll"] = ftk::Action::create(
                "Auto Scroll",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.autoScroll = value;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _actions["StopOnScrub"] = ftk::Action::create(
                "Stop Playback When Scrubbing",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.stopOnScrub = value;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _actions["Thumbnails"] = ftk::Action::create(
                "Thumbnails",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.thumbnails = value;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _actions["ThumbnailSizeSmall"] = ftk::Action::create(
                "Small",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.thumbnailSize = models::TimelineThumbnailSize::Small;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _actions["ThumbnailSizeMedium"] = ftk::Action::create(
                "Medium",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.thumbnailSize = models::TimelineThumbnailSize::Medium;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _actions["ThumbnailSizeLarge"] = ftk::Action::create(
                "Large",
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.thumbnailSize = models::TimelineThumbnailSize::Large;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _tooltips =
            {
                { "Minimize", "Minimize the timeline." },
                { "FrameView", "Frame the timeline view." },
                { "ScrollBars", "Toggle the scroll bars." },
                { "AutoScroll", "Automatically scroll the timeline to the current frame." },
                { "StopOnScrub", "Stop playback when scrubbing the timeline." },
                { "Thumbnails", "Toggle timeline thumbnails." },
                { "ThumbnailSizeSmall", "Small timeline thumbnails." },
                { "ThumbnailSizeMedium", "Medium timeline thumbnails." },
                { "ThumbnailSizeLarge", "Large timeline thumbnails." }
            };

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.settingsObserver = ftk::Observer<models::TimelineSettings>::create(
                app->getSettingsModel()->observeTimeline(),
                [this](const models::TimelineSettings& value)
                {
                    _actions["Minimize"]->setChecked(value.minimize);
                    _actions["FrameView"]->setChecked(value.frameView);
                    _actions["ScrollBars"]->setChecked(value.scrollBars);
                    _actions["AutoScroll"]->setChecked(value.autoScroll);
                    _actions["StopOnScrub"]->setChecked(value.stopOnScrub);
                    _actions["Thumbnails"]->setChecked(value.thumbnails);
                    _actions["ThumbnailSizeSmall"]->setChecked(models::TimelineThumbnailSize::Small == value.thumbnailSize);
                    _actions["ThumbnailSizeMedium"]->setChecked(models::TimelineThumbnailSize::Medium == value.thumbnailSize);
                    _actions["ThumbnailSizeLarge"]->setChecked(models::TimelineThumbnailSize::Large == value.thumbnailSize);
                });
        }

        TimelineActions::TimelineActions() :
            _p(new Private)
        {}

        TimelineActions::~TimelineActions()
        {}

        std::shared_ptr<TimelineActions> TimelineActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow)
        {
            auto out = std::shared_ptr<TimelineActions>(new TimelineActions);
            out->_init(context, app, mainWindow);
            return out;
        }
    }
}
