// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/FrameRangePopup.h>

#include <ftk/UI/FormLayout.h>
#include <ftk/UI/IntEdit.h>

namespace djv
{
    namespace ui
    {
        namespace
        {
            const int frameMin = -999999;
            const int frameMax = 999999;
        }

        struct FrameRangePopup::Private
        {
            std::shared_ptr<ftk::IntEdit> startEdit;
            std::shared_ptr<ftk::IntEdit> endEdit;
            std::function<void(const ftk::RangeI64&)> callback;
        };

        void FrameRangePopup::_init(
            const std::shared_ptr<ftk::Context>& context,
            const ftk::RangeI64& range,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidgetPopup::_init(context, "djv::ui::FrameRangePopup", parent);
            FTK_P();

            p.startEdit = ftk::IntEdit::create(context);
            p.startEdit->setTooltip(
                "Start frame of the sequence.\n"
                "\n"
                "Frames that are not there yet follow the\n"
                "missing frames setting, so a render can be\n"
                "opened over the range it will end up with.");

            p.endEdit = ftk::IntEdit::create(context);
            p.endEdit->setTooltip("End frame of the sequence.");

            // The ranges before the values: an edit holds its value inside
            // its range, and an edit starts out with a range of nowhere near
            // the numbers a render is numbered with, so a value given first
            // is clamped to something the file does not have.
            //
            // Each holds the other's end of the range, so neither can be taken
            // past it. A range that runs backwards is silently turned around
            // when it is built, which would leave the edits showing something
            // the file does not have.
            p.startEdit->setRange(frameMin, static_cast<int>(range.max()));
            p.endEdit->setRange(static_cast<int>(range.min()), frameMax);
            p.startEdit->setValue(static_cast<int>(range.min()));
            p.endEdit->setValue(static_cast<int>(range.max()));

            auto layout = ftk::FormLayout::create(context);
            layout->setMarginRole(ftk::SizeRole::MarginSmall);
            layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            layout->addRow("Start:", p.startEdit);
            layout->addRow("End:", p.endEdit);
            setWidget(layout);

            // Weak, so the two edits do not keep each other alive through
            // their callbacks.
            std::weak_ptr<ftk::IntEdit> startWeak = p.startEdit;
            std::weak_ptr<ftk::IntEdit> endWeak = p.endEdit;
            p.startEdit->setCallback(
                [this, endWeak](int value)
                {
                    if (auto end = endWeak.lock())
                    {
                        end->setRange(value, frameMax);
                        if (_p->callback)
                        {
                            _p->callback(ftk::RangeI64(value, end->getValue()));
                        }
                    }
                });
            p.endEdit->setCallback(
                [this, startWeak](int value)
                {
                    if (auto start = startWeak.lock())
                    {
                        start->setRange(frameMin, value);
                        if (_p->callback)
                        {
                            _p->callback(ftk::RangeI64(start->getValue(), value));
                        }
                    }
                });
        }

        FrameRangePopup::FrameRangePopup() :
            _p(new Private)
        {}

        FrameRangePopup::~FrameRangePopup()
        {}

        std::shared_ptr<FrameRangePopup> FrameRangePopup::create(
            const std::shared_ptr<ftk::Context>& context,
            const ftk::RangeI64& range,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<FrameRangePopup>(new FrameRangePopup);
            out->_init(context, range, parent);
            return out;
        }

        void FrameRangePopup::setCallback(
            const std::function<void(const ftk::RangeI64&)>& value)
        {
            _p->callback = value;
        }
    }
}
