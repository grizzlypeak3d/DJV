// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/SeparateAudioDialog.h>

namespace djv
{
    namespace ui
    {
        struct SeparateAudioDialog::Private
        {
            std::shared_ptr<SeparateAudioWidget> widget;
        };

        void SeparateAudioDialog::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            IDialog::_init(
                context,
                "djv::ui::SeparateAudioDialog",
                parent);
            FTK_P();

            p.widget = SeparateAudioWidget::create(
                context,
                shared_from_this());

            p.widget->setCancelCallback(
                [this]
                {
                    close();
                });
        }

        SeparateAudioDialog::SeparateAudioDialog() :
            _p(new Private)
        {}

        SeparateAudioDialog::~SeparateAudioDialog()
        {}

        std::shared_ptr<SeparateAudioDialog> SeparateAudioDialog::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SeparateAudioDialog>(new SeparateAudioDialog);
            out->_init(context, parent);
            return out;
        }

        void SeparateAudioDialog::setCallback(const std::function<void(
            const ftk::Path&,
            const ftk::Path&)>& value)
        {
            _p->widget->setCallback(value);
        }
    }
}
