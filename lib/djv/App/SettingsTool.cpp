// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/SettingsTool.h>

#include <djv/App/App.h>
#include <djv/UI/SettingsWidgets.h>

#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/Label.h>
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
            auto vLayout = ftk::VerticalLayout::create(context);
            vLayout->setSpacingRole(ftk::SizeRole::None);

            p.bellows["Cache"] = ftk::Bellows::create(context, "Cache", vLayout);
            auto vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ui::CacheSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["Cache"]->setWidget(vLayout2);

#if defined(FTK_NFD)
            p.bellows["FileBrowser"] = ftk::Bellows::create(context, "File Browser", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ui::FileBrowserSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["FileBrowser"]->setWidget(vLayout2);
#endif // FTK_NFD

            p.bellows["ImageSeqs"] = ftk::Bellows::create(context, "Image Sequences", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ui::ImageSeqSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["ImageSeqs"]->setWidget(vLayout2);

            p.bellows["Mouse"] = ftk::Bellows::create(context, "Mouse", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ui::MouseSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["Mouse"]->setWidget(vLayout2);

            p.bellows["Playback"] = ftk::Bellows::create(context, "Playback", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ui::PlaybackSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["Playback"]->setWidget(vLayout2);

            p.bellows["Shortcuts"] = ftk::Bellows::create(context, "Keyboard Shortcuts", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ui::ShortcutsSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["Shortcuts"]->setWidget(vLayout2);

            p.bellows["Style"] = ftk::Bellows::create(context, "Style", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ui::StyleSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["Style"]->setWidget(vLayout2);

            p.bellows["Time"] = ftk::Bellows::create(context, "Time", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ui::TimeSettingsWidget::create(context, app->getTimeUnitsModel(), vLayout2);
            p.bellows["Time"]->setWidget(vLayout2);

#if defined(TLRENDER_FFMPEG_PLUGIN)
            p.bellows["FFmpeg"] = ftk::Bellows::create(context, "FFmpeg", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ftk::Label::create(
                context,
                "Changes are applied to new files.",
                vLayout2);
            ui::FFmpegSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["FFmpeg"]->setWidget(vLayout2);
#endif // TLRENDER_FFMPEG_PLUGIN

#if defined(TLRENDER_FFMPEG_PIPE)
            p.bellows["FFmpegPipe"] = ftk::Bellows::create(context, "FFmpeg Pipe", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ftk::Label::create(
                context,
                "Changes are applied to new files.",
                vLayout2);
            ui::FFmpegPipeSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["FFmpegPipe"]->setWidget(vLayout2);
#endif // TLRENDER_FFMPEG_PIPE

#if defined(TLRENDER_USD)
            p.bellows["USD"] = ftk::Bellows::create(context, "USD", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ftk::Label::create(
                context,
                "Universal scene description (USD) settings.",
                vLayout2);
            ftk::Label::create(
                context,
                "Changes are applied to new files.",
                vLayout2);
            p.bellows["USD"]->setWidget(
                ui::USDSettingsWidget::create(context, settingsModel));
#endif // TLRENDER_USD

            p.bellows["Advanced"] = ftk::Bellows::create(context, "Advanced", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ftk::Label::create(
                context,
                "Changes are applied to new files.",
                vLayout2);
            ui::AdvancedSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["Advanced"]->setWidget(vLayout2);

            p.bellows["Misc"] = ftk::Bellows::create(context, "Miscellaneous", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ui::MiscSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["Misc"]->setWidget(vLayout2);

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
