// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/StatusIndicator.h>

#include <djv/Models/AudioModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/Status.h>
#include <djv/Models/ToolsModel.h>
#include <djv/Models/ViewportModel.h>

#include <djv/UI/StatusIndicatorPopup.h>

#include <ftk/UI/ToolButton.h>
#include <ftk/Core/Context.h>

namespace djv
{
    namespace ui
    {
        struct StatusIndicator::Private
        {
            // The options watched, which the models say what is on from;
            // see models::getStatus().
            models::StatusOptions options;

            std::weak_ptr<models::ViewportModel> viewportModel;
            std::weak_ptr<models::ToolsModel> toolsModel;

            std::shared_ptr<ftk::ToolButton> button;
            std::shared_ptr<StatusIndicatorPopup> popup;

            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > fgOptionsObserver;
            std::shared_ptr<ftk::Observer<models::AspectRatioOptions> > aspectRatioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<double> > audioSyncOffsetObserver;
            std::shared_ptr<ftk::Observer<int> > audioChannelObserver;
        };

        void StatusIndicator::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::AudioModel>& audioModel,
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ToolsModel>& toolsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(
                context,
                "djv::ui::StatusIndicator",
                parent);
            FTK_P();

            p.viewportModel = viewportModel;
            p.toolsModel = toolsModel;

            p.button = ftk::ToolButton::create(context);

            _setWidget(p.button);
            p.button->setIcon("MenuChecked");
            p.button->setPopupIcon(true);
            p.button->setTooltip(
                "This indicator shows options that affect video, audio, or performance.\n"
                "Click to show the options in use, and go to their controls.");

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                viewportModel->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    _p->options.display = value;
                    _indicatorUpdate();
                });

            p.fgOptionsObserver = ftk::Observer<tl::ForegroundOptions>::create(
                viewportModel->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
                {
                    _p->options.foreground = value;
                    _indicatorUpdate();
                });

            p.aspectRatioOptionsObserver = ftk::Observer<models::AspectRatioOptions>::create(
                viewportModel->observeAspectRatioOptions(),
                [this](const models::AspectRatioOptions& value)
                {
                    _p->options.aspectRatio = value;
                    _indicatorUpdate();
                });

            p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                colorModel->observeOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                    _p->options.ocio = value;
                    _indicatorUpdate();
                });

            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                colorModel->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                    _p->options.lut = value;
                    _indicatorUpdate();
                });

            p.audioSyncOffsetObserver = ftk::Observer<double>::create(
                audioModel->observeSyncOffset(),
                [this](double value)
                {
                    _p->options.audioOffset = value;
                    _indicatorUpdate();
                });

            p.audioChannelObserver = ftk::Observer<int>::create(
                filesModel->observeAudioChannel(),
                [this](int value)
                {
                    _p->options.audioChannel = value;
                    _indicatorUpdate();
                });

            p.button->setPressedCallback(
                [this]
                {
                    _showIndicatorPopup();
                });
        }

        StatusIndicator::StatusIndicator() :
            _p(new Private)
        {}

        StatusIndicator::~StatusIndicator()
        {}

        std::shared_ptr<StatusIndicator> StatusIndicator::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::AudioModel>& audioModel,
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ToolsModel>& toolsModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<StatusIndicator>(new StatusIndicator);
            out->_init(context, viewportModel, colorModel, audioModel, filesModel, toolsModel, parent);
            return out;
        }

        bool StatusIndicator::_hasIndicator() const
        {
            for (const auto& i : models::getStatus(_p->options))
            {
                if (i.second)
                {
                    return true;
                }
            }
            return false;
        }

        std::vector<std::pair<std::string, std::string> > StatusIndicator::_getIndicators() const
        {
            std::vector<std::pair<std::string, std::string> > out;
            for (const auto& item : models::getStatusItems())
            {
                out.push_back(std::make_pair(item.id, item.label));
            }
            return out;
        }

        std::map<std::string, bool> StatusIndicator::_getIndicatorValues() const
        {
            return models::getStatus(_p->options);
        }

        std::string StatusIndicator::_getIndicatorTool(const std::string& name) const
        {
            std::string out;
            if ("Channels" == name ||
                "Negative" == name ||
                "ClippingWarning" == name ||
                "Mirror" == name ||
                "AspectRatio" == name)
            {
                out = "View";
            }
            else if (
                "OCIO" == name ||
                "LUT" == name ||
                "Color" == name)
            {
                out = "Color";
            }
            else if (
                "AudioOffset" == name ||
                "AudioChannel" == name)
            {
                out = "Audio";
            }
            return out;
        }

        std::string StatusIndicator::_getIndicatorSection(const std::string& name) const
        {
            FTK_P();
            std::string out;
            if ("Channels" == name ||
                "Negative" == name ||
                "Mirror" == name)
            {
                out = "Options";
            }
            else if (
                "ClippingWarning" == name ||
                "AspectRatio" == name ||
                "OCIO" == name ||
                "LUT" == name)
            {
                out = name;
            }
            else if ("Color" == name)
            {
                // Levels has a section of its own; everything else the
                // indicator covers is in the Color section.
                out = "Color";
                if (auto viewportModel = p.viewportModel.lock())
                {
                    const tl::DisplayOptions& options = viewportModel->getDisplayOptions();
                    tl::Color color = options.color;
                    color.enabled = false;
                    tl::Levels levels = options.levels;
                    levels.enabled = false;
                    if (options.levels.enabled && levels != tl::Levels() &&
                        !(options.color.enabled && color != tl::Color()) &&
                        !(options.exposure.enabled && options.exposure.exposure != 0.F) &&
                        !(options.softClip.enabled && options.softClip.value > 0.F))
                    {
                        out = "Levels";
                    }
                }
            }
            return out;
        }

        void StatusIndicator::_indicatorUpdate()
        {
            FTK_P();
            // Checked, for the tint the other buttons use: the button is not
            // checkable, so clicking it still only opens the popup.
            p.button->setChecked(_hasIndicator());
            if (p.popup)
            {
                p.popup->setIndicators(_getIndicatorValues());
            }
        }

        void StatusIndicator::_showIndicatorPopup()
        {
            FTK_P();
            if (!p.popup)
            {
                std::weak_ptr<StatusIndicator> weak(std::dynamic_pointer_cast<StatusIndicator>(shared_from_this()));
                p.popup = StatusIndicatorPopup::create(getContext(), _getIndicators());
                p.popup->setIndicators(_getIndicatorValues());
                p.popup->setCallback(
                    [weak](const std::string& name)
                    {
                        if (auto widget = weak.lock())
                        {
                            const std::string tool = widget->_getIndicatorTool(name);
                            auto toolsModel = widget->_p->toolsModel.lock();
                            if (!tool.empty() && toolsModel)
                            {
                                toolsModel->showSection(
                                    tool,
                                    widget->_getIndicatorSection(name));
                            }
                            // Last: closing lets go of the popup this
                            // callback belongs to.
                            if (widget->_p->popup)
                            {
                                widget->_p->popup->close();
                            }
                        }
                    });
                p.popup->open(getWindow(), p.button->getGeometry());
                p.popup->setCloseCallback(
                    [weak]
                    {
                        if (auto widget = weak.lock())
                        {
                            widget->_p->popup.reset();
                        }
                    });
            }
            else
            {
                p.popup->close();
                p.popup.reset();
            }
        }
    }
}
