// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/SettingsKeysTest.h>

#include <djv/Models/SettingsKeys.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Format.h>

#include <set>

namespace djv
{
    namespace models_tests
    {
        SettingsKeysTest::SettingsKeysTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::SettingsKeysTest")
        {}

        std::shared_ptr<SettingsKeysTest> SettingsKeysTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<SettingsKeysTest>(new SettingsKeysTest(context));
        }

        void SettingsKeysTest::run()
        {
            const auto& keys = models::getSettingsKeys();
            FTK_CHECK(!keys.empty());
            std::set<std::string> seen;
            for (const auto& key : keys)
            {
                // A path into the document, and each one once.
                FTK_CHECK(!key.empty() && '/' == key[0]);
                FTK_CHECK(seen.insert(key).second);
            }
            // No key lies under another: one written as a whole would take
            // the other with it.
            for (const auto& a : keys)
            {
                for (const auto& b : keys)
                {
                    if (a != b && b.compare(0, a.size() + 1, a + "/") == 0)
                    {
                        _error(ftk::Format("\"{0}\" is under \"{1}\"").arg(b).arg(a));
                    }
                }
            }
        }
    }
}
