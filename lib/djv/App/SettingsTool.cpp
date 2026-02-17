// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/SettingsTool.h>

#include <djv/App/App.h>
#include <djv/UI/SettingsWidgets.h>

#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>

namespace djv
{
    namespace app
    {
        struct SettingsTool::Private
        {
            std::shared_ptr<ftk::VerticalLayout> layout;
            std::shared_ptr<ftk::PushButton> saveButton;
            std::shared_ptr<ftk::PushButton> resetButton;
            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
        };

        void SettingsTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                models::Tool::Settings,
                "djv::app::SettingsTool",
                parent);
            FTK_P();

            auto settingsModel = app->getSettingsModel();
            auto advancedWidget = ui::AdvancedSettingsWidget::create(context, settingsModel);
            auto cacheWidget = ui::CacheSettingsWidget::create(context, settingsModel);
#if defined(FTK_NFD)
            auto fileBrowserWidget = ui::FileBrowserSettingsWidget::create(context, settingsModel);
#endif // FTK_NFD
            auto imageSeqWidget = ui::ImageSeqSettingsWidget::create(context, settingsModel);
            auto miscWidget = ui::MiscSettingsWidget::create(context, settingsModel);
            auto mouseWidget = ui::MouseSettingsWidget::create(context, settingsModel);
            auto shortcutsWidget = ui::ShortcutsSettingsWidget::create(context, settingsModel);
            auto styleWidget = ui::StyleSettingsWidget::create(context, settingsModel);
            auto timeWidget = ui::TimeSettingsWidget::create(context, app->getTimeUnitsModel());
#if defined(TLRENDER_FFMPEG)
            auto ffmpegWidget = ui::FFmpegSettingsWidget::create(context, settingsModel);
#endif // TLRENDER_FFMPEG
#if defined(TLRENDER_USD)
            auto usdWidget = ui::USDSettingsWidget::create(context, settingsModel);
#endif // TLRENDER_USD

            auto vLayout = ftk::VerticalLayout::create(context);
            vLayout->setSpacingRole(ftk::SizeRole::None);
            p.bellows["Cache"] = ftk::Bellows::create(context, "Cache", vLayout);
            p.bellows["Cache"]->setWidget(cacheWidget);
#if defined(FTK_NFD)
            p.bellows["FileBrowser"] = ftk::Bellows::create(context, "File Browser", vLayout);
            p.bellows["FileBrowser"]->setWidget(fileBrowserWidget);
#endif // FTK_NFD
            p.bellows["ImageSeqs"] = ftk::Bellows::create(context, "Image Sequences", vLayout);
            p.bellows["ImageSeqs"]->setWidget(imageSeqWidget);
            p.bellows["Misc"] = ftk::Bellows::create(context, "Miscellaneous", vLayout);
            p.bellows["Misc"]->setWidget(miscWidget);
            p.bellows["Mouse"] = ftk::Bellows::create(context, "Mouse", vLayout);
            p.bellows["Mouse"]->setWidget(mouseWidget);
            p.bellows["Shortcuts"] = ftk::Bellows::create(context, "Keyboard Shortcuts", vLayout);
            p.bellows["Shortcuts"]->setWidget(shortcutsWidget);
            p.bellows["Style"] = ftk::Bellows::create(context, "Style", vLayout);
            p.bellows["Style"]->setWidget(styleWidget);
            p.bellows["Time"] = ftk::Bellows::create(context, "Time", vLayout);
            p.bellows["Time"]->setWidget(timeWidget);
#if defined(TLRENDER_FFMPEG)
            p.bellows["FFmpeg"] = ftk::Bellows::create(context, "FFmpeg", vLayout);
            p.bellows["FFmpeg"]->setWidget(ffmpegWidget);
#endif // TLRENDER_USD
#if defined(TLRENDER_USD)
            p.bellows["USD"] = ftk::Bellows::create(context, "USD", vLayout);
            p.bellows["USD"]->setWidget(usdWidget);
#endif // TLRENDER_USD
            p.bellows["Advanced"] = ftk::Bellows::create(context, "Advanced", vLayout);
            p.bellows["Advanced"]->setWidget(advancedWidget);

            p.saveButton = ftk::PushButton::create(context, "Save");
            p.saveButton->setTooltip("Save the settings. Settings are also saved on exit.");

            p.resetButton = ftk::PushButton::create(context, "Reset");
            p.resetButton->setTooltip("Restore settings to default values.");

            p.layout = ftk::VerticalLayout::create(context);
            p.layout->setSpacingRole(ftk::SizeRole::None);
            auto scrollWidget = ftk::ScrollWidget::create(context, ftk::ScrollType::Both, p.layout);
            scrollWidget->setWidget(vLayout);
            scrollWidget->setBorder(false);
            scrollWidget->setVStretch(ftk::Stretch::Expanding);
            ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.saveButton->setParent(hLayout);
            hLayout->addSpacer(ftk::Stretch::Expanding);
            p.resetButton->setParent(hLayout);
            _setWidget(p.layout);

            _loadSettings(p.bellows);

            std::weak_ptr<App> appWeak(app);
            p.saveButton->setClickedCallback(
                [this, appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getSettingsModel()->save();
                    }
                });

            p.resetButton->setClickedCallback(
                [this, appWeak]
                {
                    if (auto context = getContext())
                    {
                        if (auto dialogSystem = context->getSystem<ftk::DialogSystem>())
                        {
                            dialogSystem->confirm(
                                "Reset Settings",
                                "Reset settings to default values?",
                                getWindow(),
                                [appWeak](bool value)
                                {
                                    if (value)
                                    {
                                        if (auto app = appWeak.lock())
                                        {
                                            app->getSettingsModel()->reset();
                                        }
                                    }
                                });
                        }
                    }
                });
        }

        SettingsTool::SettingsTool() :
            _p(new Private)
        {}

        SettingsTool::~SettingsTool()
        {
            _saveSettings(_p->bellows);
        }

        std::shared_ptr<SettingsTool> SettingsTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SettingsTool>(new SettingsTool);
            out->_init(context, app, parent);
            return out;
        }
    }
}
