// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/StatusIndicator.h>

#include <djv/UI/StatusIndicatorPopup.h>
#include <djv/Models/AudioModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/UI/ToolButton.h>
#include <ftk/Core/Context.h>

namespace djv
{
    namespace ui
    {
        struct StatusIndicator::Private
        {
            bool channelsEnabled = false;
            bool negativeEnabled = false;
            bool clippingWarningEnabled = false;
            bool mirrorEnabled = false;
            bool aspectRatioEnabled = false;
            bool ocioEnabled = false;
            bool lutEnabled = false;
            bool colorEnabled = false;
            bool audioOffsetEnabled = false;

            std::shared_ptr<ftk::ToolButton> button;
            std::shared_ptr<StatusIndicatorPopup> popup;

            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > fgOptionsObserver;
            std::shared_ptr<ftk::Observer<models::AspectRatioOptions> > aspectRatioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<double> > audioSyncOffsetObserver;
        };

        void StatusIndicator::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::AudioModel>& audioModel,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(
                context,
                "djv::ui::StatusIndicator",
                parent);
            FTK_P();

            p.button = ftk::ToolButton::create(context);

            _setWidget(p.button);
            p.button->setIcon("MenuChecked");
            p.button->setPopupIcon(true);
            p.button->setTooltip(
                "This indicator shows options that affect video, audio, or performance.\n"
                "Click to show which options are in use.");

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                viewportModel->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    FTK_P();
                    p.channelsEnabled =
                        value.channels != ftk::ChannelDisplay::Color;
                    p.negativeEnabled = value.negative;
                    p.mirrorEnabled =
                        value.mirror.x ||
                        value.mirror.y;
                    // Only an adjustment that changes the picture: enabled at
                    // its defaults it is neutral, and the renderer skips it or
                    // nearly so, so there is nothing to be reminded of.
                    tl::Color color = value.color;
                    color.enabled = false;
                    tl::Levels levels = value.levels;
                    levels.enabled = false;
                    p.colorEnabled =
                        (value.color.enabled && color != tl::Color()) ||
                        (value.levels.enabled && levels != tl::Levels()) ||
                        (value.exposure.enabled && value.exposure.exposure != 0.F) ||
                        (value.softClip.enabled && value.softClip.value > 0.F);
                    _indicatorUpdate();
                });

            p.fgOptionsObserver = ftk::Observer<tl::ForegroundOptions>::create(
                viewportModel->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
                {
                    _p->clippingWarningEnabled = value.clippingWarning.enabled;
                    _indicatorUpdate();
                });

            p.aspectRatioOptionsObserver = ftk::Observer<models::AspectRatioOptions>::create(
                viewportModel->observeAspectRatioOptions(),
                [this](const models::AspectRatioOptions& value)
                {
                    FTK_P();
                    p.aspectRatioEnabled = value.index != 0;
                    _indicatorUpdate();
                });

            p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                colorModel->observeOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                    // As the renderer decides: a display and a view to
                    // transform to.
                    _p->ocioEnabled =
                        value.enabled &&
                        !value.display.empty() &&
                        !value.view.empty();
                    _indicatorUpdate();
                });

            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                colorModel->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                    _p->lutEnabled = value.enabled && !value.fileName.empty();
                    _indicatorUpdate();
                });

            p.audioSyncOffsetObserver = ftk::Observer<double>::create(
                audioModel->observeSyncOffset(),
                [this](double value)
                {
                    _p->audioOffsetEnabled = value != 0.0;
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
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<StatusIndicator>(new StatusIndicator);
            out->_init(context, viewportModel, colorModel, audioModel, parent);
            return out;
        }

        bool StatusIndicator::_hasIndicator() const
        {
            FTK_P();
            return
                p.channelsEnabled    ||
                p.negativeEnabled    ||
                p.clippingWarningEnabled ||
                p.mirrorEnabled      ||
                p.aspectRatioEnabled ||
                p.ocioEnabled        ||
                p.lutEnabled         ||
                p.colorEnabled       ||
                p.audioOffsetEnabled;
        }

        std::vector<std::pair<std::string, std::string> > StatusIndicator::_getIndicators() const
        {
            return
            {
                { "Channels", "Image channels" },
                { "Negative", "Negative" },
                { "ClippingWarning", "Clipping warning" },
                { "Mirror", "Mirror" },
                { "AspectRatio", "Aspect ratio" },
                { "OCIO", "OCIO" },
                { "LUT", "LUT" },
                { "Color", "Color controls" },
                { "AudioOffset", "Audio offset" }
            };
        }

        std::map<std::string, bool> StatusIndicator::_getIndicatorValues() const
        {
            FTK_P();
            return
            {
                { "Channels", p.channelsEnabled },
                { "Negative", p.negativeEnabled },
                { "ClippingWarning", p.clippingWarningEnabled },
                { "Mirror", p.mirrorEnabled },
                { "AspectRatio", p.aspectRatioEnabled },
                { "OCIO", p.ocioEnabled },
                { "LUT", p.lutEnabled },
                { "Color", p.colorEnabled },
                { "AudioOffset", p.audioOffsetEnabled }
            };
        }

        void StatusIndicator::_indicatorUpdate()
        {
            FTK_P();
            p.button->setBackgroundRole(
                _hasIndicator() ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
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
                p.popup = StatusIndicatorPopup::create(
                    getContext(),
                    _getIndicators());
                _indicatorUpdate();
                p.popup->open(getWindow(), p.button->getGeometry());
                std::weak_ptr<StatusIndicator> weak(std::dynamic_pointer_cast<StatusIndicator>(shared_from_this()));
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
