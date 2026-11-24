// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/SettingsTool.h>

#include <djv/App/App.h>
#include <djv/UI/SettingsWidgets.h>

#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>

namespace djv
{
    namespace app
    {
        struct SettingsTool::Private
        {
            std::shared_ptr<ftk::PushButton> saveButton;
            std::shared_ptr<ftk::PushButton> resetButton;
            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
            std::shared_ptr<ftk::VerticalLayout> layout;
        };

        void SettingsTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                mainWindow,
                "Settings",
                "Settings",
                "djv::app::SettingsTool",
                parent);
            FTK_P();

            auto settingsModel = app->getSettingsModel();
            auto vLayout = ftk::VerticalLayout::create(context);
            vLayout->setSpacingRole(ftk::SizeRole::Border);

            p.bellows["Cache"] = ftk::Bellows::create(context, "Cache", vLayout);
            auto vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ui::CacheSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["Cache"]->setWidget(vLayout2);
            ftk::setScreenshotTag(p.bellows["Cache"], "Settings.Cache");

            p.bellows["FileBrowser"] = ftk::Bellows::create(context, "File Browser", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ui::FileBrowserSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["FileBrowser"]->setWidget(vLayout2);

            p.bellows["ImageSeqs"] = ftk::Bellows::create(context, "Image Sequences", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ui::ImageSeqSettingsWidget::create(
                context, settingsModel, app->getViewportModel(), vLayout2);
            p.bellows["ImageSeqs"]->setWidget(vLayout2);

            p.bellows["OTIO"] = ftk::Bellows::create(context, "OTIO", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ftk::Label::create(
                context,
                "Changes are applied to new files.",
                vLayout2);
            ui::OTIOSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["OTIO"]->setWidget(vLayout2);

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

            p.bellows["Review"] = ftk::Bellows::create(context, "Review", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ui::ReviewSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["Review"]->setWidget(vLayout2);

            p.bellows["Audio"] = ftk::Bellows::create(context, "Audio", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ftk::Label::create(
                context,
                "Changes are applied to new files.",
                vLayout2);
            ui::AudioSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["Audio"]->setWidget(vLayout2);

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
            ftk::setScreenshotTag(p.bellows["Style"], "Settings.Style");

#if defined(FTK_GPU)
            // Only where there is a choice of renderer to make.
            p.bellows["Graphics"] = ftk::Bellows::create(context, "Graphics", vLayout);
            vLayout2 = ftk::VerticalLayout::create(context, vLayout);
            vLayout2->setMarginRole(ftk::SizeRole::Margin);
            ftk::Label::create(
                context,
                "Changes are applied when the application is restarted.",
                vLayout2);
            ui::GraphicsSettingsWidget::create(context, app, vLayout2);
            p.bellows["Graphics"]->setWidget(vLayout2);
            ftk::setScreenshotTag(p.bellows["Graphics"], "Settings.Graphics");
#endif // FTK_GPU

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
            ftk::setScreenshotTag(p.bellows["FFmpeg"], "Settings.FFmpeg");
#endif // TLRENDER_FFMPEG_PLUGIN

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
            ui::USDSettingsWidget::create(context, settingsModel, vLayout2);
            p.bellows["USD"]->setWidget(vLayout2);
#endif // TLRENDER_USD


            p.saveButton = ftk::PushButton::create(context, "Save");
            p.saveButton->setTooltip("Save the settings. Settings are also saved on exit.");

            p.resetButton = ftk::PushButton::create(context, "Reset");
            p.resetButton->setTooltip("Restore settings to default values.");
            ftk::setScreenshotTag(p.resetButton, "Settings.Reset");

            p.layout = ftk::VerticalLayout::create(context);
            p.layout->setSpacingRole(ftk::SizeRole::None);
            // The sections' labels share one width, so they line up as one
            // form would.
            ftk::setFormGroup(vLayout, ftk::FormGroup::create());
            // The tools share one scroll area, so the sections and the
            // buttons scroll with the panel rather than inside it.
            vLayout->setParent(p.layout);
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
                [appWeak]
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
                                            // The renderer is the
                                            // application's setting rather
                                            // than the model's, so the
                                            // model's reset cannot cover it.
                                            app->setRenderer(
                                                ftk::Renderer::OpenGL);
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
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SettingsTool>(new SettingsTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }
    }
}
