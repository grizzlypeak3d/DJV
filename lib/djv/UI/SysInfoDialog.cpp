// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/SysInfoDialog.h>

#include <djv/Models/AppInfoModel.h>
#include <djv/Models/SettingsModel.h>

#include <tlRender/Timeline/AudioSystem.h>
#include <tlRender/IO/Plugin.h>
#include <tlRender/IO/System.h>

#include <ftk/UI/ClipboardSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/IWindow.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/TextEdit.h>

#include <ftk/GL/Window.h>

#include <ftk/Core/Format.h>
#include <ftk/Core/OS.h>
#include <ftk/Core/String.h>

namespace djv
{
    namespace ui
    {
        std::vector<std::string> getSysInfo(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::AppInfoModel>& appInfoModel,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::vector<std::pair<std::string, std::string> >& windowInfo)
        {
            std::vector<std::pair<std::string, std::string> > labels;
            labels.push_back(std::make_pair(
                appInfoModel->getFullName() + " version: ",
                appInfoModel->getVersion()));
            labels.push_back(std::make_pair(
                "Commit date: ",
                appInfoModel->getCommitDate()));
            labels.push_back(std::make_pair(
                "Commit: ",
                appInfoModel->getGitCommit()));

            labels.push_back(std::make_pair("", ""));
            const auto sysInfo = ftk::getSysInfo();
            labels.push_back(std::make_pair("System: ", sysInfo.name));
            if (!sysInfo.cpu.empty())
            {
                labels.push_back(std::make_pair("CPU: ", sysInfo.cpu));
            }
            labels.push_back(std::make_pair(
                "CPU Cores: ",
                ftk::Format("{0}").arg(sysInfo.cores)));
            labels.push_back(std::make_pair(
                "Memory: ",
                ftk::Format("{0}GB").arg(sysInfo.ramGB)));

            // -sysInfo prints this and exits before the window is made. The
            // OpenGL strings are the most useful part of the report, so ask a
            // hidden one pixel window for them rather than leave them out.
            labels.push_back(std::make_pair("", ""));
            if (!windowInfo.empty())
            {
                for (const auto& i : windowInfo)
                {
                    labels.push_back(std::make_pair(i.first + ": ", i.second));
                }
            }
            else
            {
                try
                {
                    auto glWindow = ftk::gl::Window::create(
                        context,
                        "sysInfo",
                        ftk::Size2I(1, 1),
                        static_cast<int>(ftk::gl::WindowOptions::MakeCurrent));
                    const auto& glInfo = glWindow->getGLInfo();
                    labels.push_back(std::make_pair("GL vendor: ", glInfo.vendor));
                    labels.push_back(std::make_pair("GL renderer: ", glInfo.renderer));
                    labels.push_back(std::make_pair("GL version: ", glInfo.version));
                }
                catch (const std::exception& e)
                {
                    labels.push_back(std::make_pair("OpenGL: ", e.what()));
                }
            }

            if (auto audioSystem = context->getSystem<tl::AudioSystem>())
            {
                labels.push_back(std::make_pair("", ""));
                labels.push_back(std::make_pair(
                    "Audio driver: ",
                    audioSystem->getCurrentDriver()));
                const auto& devices = audioSystem->getDevices();
                for (size_t i = 0; i < devices.size(); ++i)
                {
                    const auto& device = devices[i];
                    labels.push_back(std::make_pair(
                        ftk::Format("Audio device {0}: ").arg(i),
                        device.id.name));
                    labels.push_back(std::make_pair(
                        std::string(),
                        tl::getLabel(device.info)));
                }
            }

            auto readSystem = context->getSystem<tl::ReadSystem>();
            auto writeSystem = context->getSystem<tl::WriteSystem>();
            if (readSystem || writeSystem)
            {
                const auto& ioOptions = settingsModel->getIOOptions();
                if (readSystem)
                {
                    labels.push_back(std::make_pair("", ""));
                    labels.push_back(std::make_pair("Read plugins:", ""));
                    for (const auto& plugin : readSystem->getPlugins())
                    {
                        labels.push_back(std::make_pair(
                            plugin->getPluginName() + ": ",
                            plugin->getPluginInfo(ioOptions)));
                    }
                }
                if (writeSystem)
                {
                    labels.push_back(std::make_pair("", ""));
                    labels.push_back(std::make_pair("Write plugins:", ""));
                    for (const auto& plugin : writeSystem->getPlugins())
                    {
                        labels.push_back(std::make_pair(
                            plugin->getPluginName() + ": ",
                            plugin->getPluginInfo(ioOptions)));
                    }
                }
            }

            size_t sizeMax = 0;
            for (const auto& i : labels)
            {
                sizeMax = std::max(sizeMax, i.first.size());
            }
            for (auto& i : labels)
            {
                if (!(i.first.empty() && i.second.empty()))
                {
                    i.first.resize(sizeMax, ' ');
                }
            }

            std::vector<std::string> out;
            for (const auto& i : labels)
            {
                out.push_back(i.first + i.second);
            }
            return out;
        }

        struct SysInfoDialog::Private
        {
            std::vector<std::string> text;
            std::shared_ptr<ftk::PushButton> closeButton;
        };

        void SysInfoDialog::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::vector<std::string>& text,
            const std::shared_ptr<IWidget>& parent)
        {
            IDialog::_init(
                context,
                "djv::ui::SysInfoDialog",
                parent);
            FTK_P();
            
            p.text = text;

            setTitle("System Information");


            auto copyButton = ftk::PushButton::create(context, "Copy");
            p.closeButton = ftk::PushButton::create(context, "Close");

            auto layout = ftk::VerticalLayout::create(context, shared_from_this());
            layout->setSpacingRole(ftk::SizeRole::None);
            layout->setStretch(ftk::Stretch::Expanding, ftk::Stretch::Expanding);

            auto textEdit = ftk::TextEdit::create(context, layout);
            textEdit->setReadOnly(true);
            ftk::TextEditOptions textEditOptions;
            textEditOptions.fontInfo.name = ftk::getDefaultFont(ftk::FontType::Mono);
            textEdit->setOptions(textEditOptions);
            textEdit->setMarginRole(ftk::SizeRole::Margin);
            textEdit->setVStretch(ftk::Stretch::Expanding);
            textEdit->setText(p.text);

            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);
            auto hLayout = ftk::HorizontalLayout::create(context, layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            copyButton->setParent(hLayout);
            hLayout->addSpacer(ftk::SizeRole::Spacing, ftk::Stretch::Expanding);
            p.closeButton->setParent(hLayout);

            copyButton->setClickedCallback(
                [this]
                {
                    if (auto context = getContext())
                    {
                        auto clipboardSystem = context->getSystem<ftk::ClipboardSystem>();
                        clipboardSystem->setText(ftk::join(_p->text, '\n'));
                    }
                });

            p.closeButton->setClickedCallback(
                [this]
                {
                    close();
                });
        }

        SysInfoDialog::SysInfoDialog() :
            _p(new Private)
        {}

        SysInfoDialog::~SysInfoDialog()
        {}

        std::shared_ptr<SysInfoDialog> SysInfoDialog::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::vector<std::string>& text,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SysInfoDialog>(new SysInfoDialog);
            out->_init(context, text, parent);
            return out;
        }

        std::shared_ptr<ftk::IWidget> SysInfoDialog::getKeyFocus() const
        {
            return _p->closeButton;
        }
    }
}
