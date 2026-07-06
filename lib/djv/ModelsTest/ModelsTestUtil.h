// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/Settings.h>

#include <filesystem>
#include <memory>

namespace djv
{
    namespace models_tests
    {
        //! Create in-memory settings for testing.
        //!
        //! An empty path makes the settings purely in-memory: nothing is loaded
        //! and nothing is saved, not even on destruction. This keeps model tests
        //! isolated from the user's real settings and from each other, and leaves
        //! nothing behind on disk. For tests that need to verify save/restore,
        //! create ftk::Settings with a temporary file path instead.
        inline std::shared_ptr<ftk::Settings> createTestSettings(
            const std::shared_ptr<ftk::Context>& context)
        {
            return ftk::Settings::create(context, std::filesystem::path());
        }
    }
}
