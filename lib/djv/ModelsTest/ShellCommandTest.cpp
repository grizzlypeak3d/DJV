// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/ShellCommandTest.h>

#include <djv/Models/ShellCommand.h>

#include <ftk/Core/Assert.h>

namespace djv
{
    namespace models_tests
    {
        ShellCommandTest::ShellCommandTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::ShellCommandTest")
        {}

        std::shared_ptr<ShellCommandTest> ShellCommandTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<ShellCommandTest>(new ShellCommandTest(context));
        }

        void ShellCommandTest::run()
        {
            _parse();
            _repair();
        }

        void ShellCommandTest::_parse()
        {
            {
                const auto c = models::parseShellCommand(
                    "\"C:/Program Files/DJV 3.4.2/bin/djv.exe\" \"%1\"");
                FTK_ASSERT(c);
                FTK_CHECK("C:/Program Files/DJV 3.4.2/bin/djv.exe" == c->program);
                FTK_CHECK("\"%1\"" == c->arguments);
            }
            {
                const auto c = models::parseShellCommand("  djv.exe   %1");
                FTK_ASSERT(c);
                FTK_CHECK("djv.exe" == c->program);
                FTK_CHECK("%1" == c->arguments);
            }
            {
                const auto c = models::parseShellCommand("\"C:/DJV/djv.exe\"");
                FTK_ASSERT(c);
                FTK_CHECK("C:/DJV/djv.exe" == c->program);
                FTK_CHECK(c->arguments.empty());
            }
            FTK_CHECK(!models::parseShellCommand(""));
            FTK_CHECK(!models::parseShellCommand("   "));
            FTK_CHECK(!models::parseShellCommand("\"C:/DJV/djv.exe %1"));
            FTK_CHECK(!models::parseShellCommand("\"\" %1"));
        }

        void ShellCommandTest::_repair()
        {
            const std::filesystem::path exe = "C:/Program Files/DJV 3.7.0/bin/djv.exe";
            const auto missing = [](const std::filesystem::path&) { return false; };
            const auto present = [](const std::filesystem::path&) { return true; };

            // A removed install is pointed at this one, keeping the
            // arguments.
            {
                const auto r = models::repairShellCommand(
                    "\"C:/Program Files/DJV 3.4.2/bin/djv.exe\" \"%1\"", exe, missing);
                FTK_ASSERT(r);
                FTK_CHECK("\"C:/Program Files/DJV 3.7.0/bin/djv.exe\" \"%1\"" == *r);
            }

            // The file name is compared without regard to case, as Windows
            // does.
            {
                const auto r = models::repairShellCommand(
                    "\"C:/Old/DJV.EXE\" \"%1\"", exe, missing);
                FTK_CHECK(r.has_value());
            }

            // Another program's command is never touched, even if it is
            // gone too.
            FTK_CHECK(!models::repairShellCommand(
                "\"C:/Program Files/GIMP/bin/gimp.exe\" \"%1\"", exe, missing));

            // A program whose name only contains ours is another program.
            FTK_CHECK(!models::repairShellCommand(
                "\"C:/Program Files/DJV Studio/bin/djv-studio.exe\" \"%1\"", exe, missing));

            // A copy still installed elsewhere is a choice, not a leftover.
            FTK_CHECK(!models::repairShellCommand(
                "\"C:/Program Files/DJV 3.4.2/bin/djv.exe\" \"%1\"", exe, present));

            // Already right.
            FTK_CHECK(!models::repairShellCommand(
                "\"C:/Program Files/DJV 3.7.0/bin/djv.exe\" \"%1\"", exe, missing));

            // Nothing to parse.
            FTK_CHECK(!models::repairShellCommand("", exe, missing));
        }
    }
}
