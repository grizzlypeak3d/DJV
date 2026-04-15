// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/StatusIndicatorPopup.h>

#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/Icon.h>

namespace djv
{
    namespace ui
    {
        struct StatusIndicatorPopup::Private
        {
            std::map<std::string, std::shared_ptr<ftk::Icon> > icons;
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

            p.icons["OCIO"] = ftk::Icon::create(context, "MenuChecked");
            p.icons["LUT"] = ftk::Icon::create(context, "MenuChecked");
            p.icons["Channels"] = ftk::Icon::create(context, "MenuChecked");
            p.icons["Mirror"] = ftk::Icon::create(context, "MenuChecked");
            p.icons["Color"] = ftk::Icon::create(context, "MenuChecked");
            p.icons["AudioOffset"] = ftk::Icon::create(context, "MenuChecked");
            p.icons["OutputDevice"] = ftk::Icon::create(context, "MenuChecked");

            auto layout = ftk::FormLayout::create(context);
            layout->setMarginRole(ftk::SizeRole::MarginSmall);
            layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            layout->addRow("OCIO:", p.icons["OCIO"]);
            layout->addRow("LUT:", p.icons["LUT"]);
            layout->addRow("Image channels:", p.icons["Channels"]);
            layout->addRow("Mirror:", p.icons["Mirror"]);
            layout->addRow("Color controls:", p.icons["Color"]);
            layout->addRow("Audio offset:", p.icons["AudioOffset"]);
#if defined(TLRENDER_BMD)
            layout->addRow("Output device:", p.icons["OutputDevice"]);
#endif // TLRENDER_BMD
            setWidget(layout);
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
        }

        void StatusIndicatorPopup::setLUT(bool value)
        {
            _p->icons["LUT"]->setEnabled(value);
            _p->icons["LUT"]->setBackgroundRole(value ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
        }

        void StatusIndicatorPopup::setChannels(bool value)
        {
            _p->icons["Channels"]->setEnabled(value);
            _p->icons["Channels"]->setBackgroundRole(value ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
        }

        void StatusIndicatorPopup::setMirror(bool value)
        {
            _p->icons["Mirror"]->setEnabled(value);
            _p->icons["Mirror"]->setBackgroundRole(value ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
        }

        void StatusIndicatorPopup::setColor(bool value)
        {
            _p->icons["Color"]->setEnabled(value);
            _p->icons["Color"]->setBackgroundRole(value ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
        }

        void StatusIndicatorPopup::setAudioOffset(bool value)
        {
            _p->icons["AudioOffset"]->setEnabled(value);
            _p->icons["AudioOffset"]->setBackgroundRole(value ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
        }

        void StatusIndicatorPopup::setOutputDevice(bool value)
        {
            _p->icons["OutputDevice"]->setEnabled(value);
            _p->icons["OutputDevice"]->setBackgroundRole(value ?
                ftk::ColorRole::Checked :
                ftk::ColorRole::None);
        }
    }
}
