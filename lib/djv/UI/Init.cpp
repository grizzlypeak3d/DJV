// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/Init.h>

#include <djv_resource/IconResources.h>

#include <ftk/UI/IconSystem.h>

#include <ftk/Core/Context.h>

namespace djv
{
    namespace ui
    {
        void initIcons(const std::shared_ptr<ftk::Context>& context)
        {
            auto iconSystem = context->getSystem<ftk::IconSystem>();
            for (const auto& i : djv_resource::getIconResources())
            {
                iconSystem->add(i.first, *i.second);
            }
        }
    }
}
