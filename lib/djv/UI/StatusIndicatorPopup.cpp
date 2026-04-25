// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/StatusIndicatorPopup.h>

#include <ftk/UI/GridLayout.h>
#include <ftk/UI/Icon.h>
#include <ftk/UI/Label.h>

namespace djv
{
    namespace ui
    {
        struct StatusIndicatorPopup::Private
        {
            std::map<std::string, std::shared_ptr<ftk::Icon> > icons;
            std::map<std::string, std::shared_ptr<ftk::Label> > labels;
        };

        void StatusIndicatorPopup::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidgetPopup::_init(
                context,
                "djv::ui::StatusIndicatorPopup",
                parent);
            FTK_P();

            auto layout = ftk::GridLayout::create(context);
            layout->setMarginRole(ftk::SizeRole::MarginSmall);
            layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            setWidget(layout);

            const std::vector<std::pair<std::string, std::string> > labels =
            {
                { "OCIO", "OCIO" },
                { "LUT", "LUT" },
                { "Channels", "Image channels" },
                { "Mirror", "Mirror" },
                { "Color", "Color controls" },
                { "AudioOffset", "Audio offset" },
#if defined(TLRENDER_BMD)
                { "OutputDevice", "Output device" }
#endif // TLRENDER_BMD
            };

            int row = 0;
            for (const auto& i : labels)
            {
                p.icons[i.first] = ftk::Icon::create(context, "MenuChecked", layout);
                p.labels[i.first] = ftk::Label::create(context, i.second, layout);
                layout->setGridPos(p.icons[i.first], row, 0);
                layout->setGridPos(p.labels[i.first], row, 1);
                ++row;
            }
        }

        StatusIndicatorPopup::StatusIndicatorPopup() :
            _p(new Private)
        {}

        StatusIndicatorPopup::~StatusIndicatorPopup()
        {}

        std::shared_ptr<StatusIndicatorPopup> StatusIndicatorPopup::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<StatusIndicatorPopup>(new StatusIndicatorPopup);
            out->_init(context, parent);
            return out;
        }

        void StatusIndicatorPopup::setOCIO(bool value)
        {
            _p->icons["OCIO"]->setEnabled(value);
            _p->icons["OCIO"]->setBackgroundRole(value ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
            _p->labels["OCIO"]->setEnabled(value);
        }

        void StatusIndicatorPopup::setLUT(bool value)
        {
            _p->icons["LUT"]->setEnabled(value);
            _p->icons["LUT"]->setBackgroundRole(value ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
            _p->labels["LUT"]->setEnabled(value);
        }

        void StatusIndicatorPopup::setChannels(bool value)
        {
            _p->icons["Channels"]->setEnabled(value);
            _p->icons["Channels"]->setBackgroundRole(value ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
            _p->labels["Channels"]->setEnabled(value);
        }

        void StatusIndicatorPopup::setMirror(bool value)
        {
            _p->icons["Mirror"]->setEnabled(value);
            _p->icons["Mirror"]->setBackgroundRole(value ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
            _p->labels["Mirror"]->setEnabled(value);
        }

        void StatusIndicatorPopup::setColor(bool value)
        {
            _p->icons["Color"]->setEnabled(value);
            _p->icons["Color"]->setBackgroundRole(value ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
            _p->labels["Color"]->setEnabled(value);
        }

        void StatusIndicatorPopup::setAudioOffset(bool value)
        {
            _p->icons["AudioOffset"]->setEnabled(value);
            _p->icons["AudioOffset"]->setBackgroundRole(value ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
            _p->labels["AudioOffset"]->setEnabled(value);
        }

        void StatusIndicatorPopup::setOutputDevice(bool value)
        {
#if defined(TLRENDER_BMD)
            _p->icons["OutputDevice"]->setEnabled(value);
            _p->icons["OutputDevice"]->setBackgroundRole(value ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
            _p->labels["OutputDevice"]->setEnabled(value);
#endif // TLRENDER_BMD
        }
    }
}
