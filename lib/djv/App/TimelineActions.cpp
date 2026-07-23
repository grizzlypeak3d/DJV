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

            // Register the commands.
            auto appWeak = std::weak_ptr<App>(app);
            _addCheckCommand(
                "Minimize",
                "Minimize the timeline.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.minimize = value;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _addCheckCommand(
                "FrameView",
                "Frame the timeline view.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.frameView = value;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _addCheckCommand(
                "ScrollBars",
                "Toggle the scroll bars.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.scrollBars = value;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _addCheckCommand(
                "AutoScroll",
                "Automatically scroll the timeline to the current frame.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.autoScroll = value;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _addCheckCommand(
                "StopOnScrub",
                "Stop playback when scrubbing the timeline.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.stopOnScrub = value;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _addCheckCommand(
                "Thumbnails",
                "Toggle timeline thumbnails.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.thumbnails = value;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _addCommand(
                "ThumbnailSizeSmall",
                "Small timeline thumbnails.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.thumbnailSize = models::TimelineThumbnailSize::Small;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _addCommand(
                "ThumbnailSizeMedium",
                "Medium timeline thumbnails.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.thumbnailSize = models::TimelineThumbnailSize::Medium;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            _addCommand(
                "ThumbnailSizeLarge",
                "Large timeline thumbnails.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto settings = app->getSettingsModel()->getTimeline();
                        settings.thumbnailSize = models::TimelineThumbnailSize::Large;
                        app->getSettingsModel()->setTimeline(settings);
                    }
                });

            // Create the actions.
            _actions["Minimize"] = ftk::Action::create(
                "Minimize",
                _checkCommand("Minimize"));
            _actions["FrameView"] = ftk::Action::create(
                "Frame View",
                _checkCommand("FrameView"));
            _actions["ScrollBars"] = ftk::Action::create(
                "Scroll Bars",
                _checkCommand("ScrollBars"));
            _actions["AutoScroll"] = ftk::Action::create(
                "Auto Scroll",
                _checkCommand("AutoScroll"));
            _actions["StopOnScrub"] = ftk::Action::create(
                "Stop Playback When Scrubbing",
                _checkCommand("StopOnScrub"));
            _actions["Thumbnails"] = ftk::Action::create(
                "Thumbnails",
                _checkCommand("Thumbnails"));
            _actions["ThumbnailSizeSmall"] = ftk::Action::create(
                "Small",
                _command("ThumbnailSizeSmall"));
            _actions["ThumbnailSizeMedium"] = ftk::Action::create(
                "Medium",
                _command("ThumbnailSizeMedium"));
            _actions["ThumbnailSizeLarge"] = ftk::Action::create(
                "Large",
                _command("ThumbnailSizeLarge"));

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
