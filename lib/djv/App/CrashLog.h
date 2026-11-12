// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>

#include <filesystem>
#include <string>

namespace djv
{
    namespace app
    {
        //! A last line in the log when the program dies.
        //!
        //! The log says what the program did; without this it stops
        //! wherever the program did, and a crash looks like any other
        //! end. Installed, a crash (a bad memory access, an illegal
        //! instruction, a division by zero, an abort) or an exception
        //! nothing caught writes a line saying so, with a backtrace where
        //! the system can give one, and then goes to whatever handled it
        //! before: the system's crash reporter still runs. A program that
        //! ends normally writes a clean exit line last, so a log without
        //! one ended some other way.
        //!
        //! For the programs, not the libraries: the Python bindings leave
        //! the interpreter's handlers alone. It does nothing in a build
        //! with a sanitizer, which has handlers of its own.
        class DJV_APP_API_TYPE CrashLog
        {
        public:
            //! Install the handlers, writing to the log file. An empty
            //! path installs nothing.
            DJV_APP_API explicit CrashLog(const std::filesystem::path& logFile);

            //! Write the exit line and put the handlers back.
            DJV_APP_API ~CrashLog();

            CrashLog(const CrashLog&) = delete;
            CrashLog& operator = (const CrashLog&) = delete;

            //! Whether the handlers are installed.
            DJV_APP_API bool isInstalled() const;

            //! The error a program ends on, which the exit line gives in
            //! place of a clean exit.
            DJV_APP_API void setError(const std::string&);

        private:
            bool _installed = false;
            std::string _error;
        };
    }
}
