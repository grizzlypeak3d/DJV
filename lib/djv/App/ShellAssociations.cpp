// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ShellAssociations.h>

#include <djv/Models/ShellCommand.h>

#include <ftk/Core/Context.h>
#include <ftk/Core/OS.h>
#include <ftk/Core/Path.h>
#include <ftk/Core/String.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#include <shlobj.h>
#endif // _WIN32

#include <filesystem>
#include <string>
#include <vector>

// Why this exists: the installer writes the machine's registration for DJV
// in HKLM, and removes it on uninstall (cmake/Modules/Package.cmake). Windows
// also writes entries of its own, in the user's hive, when someone picks DJV
// in "Open with" -- Applications\djv.exe -- or ticks "Always use this app",
// which makes a <ext>_auto_file class. Those name the install directory as it
// was then, which carries the version, so they outlive an upgrade; and being
// in HKCU they win over anything the installer writes. An elevated installer
// cannot reach them: the hive it would open is the administrator's, not the
// user's. The application runs as the user, so it puts them right.
//
// Only commands naming a program with this executable's file name are
// changed, and only when that program is gone. Other applications' entries,
// and a copy of DJV still installed elsewhere, are left as they are. The
// MuiCache and AppCompatFlags caches Windows keeps are left too: they hold a
// label and a compatibility note, not a command, and are Windows' to manage.

namespace djv
{
    namespace app
    {
#if defined(_WIN32)
        namespace
        {
            const char* logPrefix = "djv::app::repairShellAssociations";

            bool readString(HKEY key, std::wstring& value, DWORD& type)
            {
                DWORD size = 0;
                if (RegQueryValueExW(key, nullptr, nullptr, &type, nullptr, &size) != ERROR_SUCCESS ||
                    (type != REG_SZ && type != REG_EXPAND_SZ))
                {
                    return false;
                }
                // Not necessarily terminated, so room for one more.
                std::vector<wchar_t> buf(size / sizeof(wchar_t) + 1, 0);
                size = static_cast<DWORD>(buf.size() * sizeof(wchar_t));
                if (RegQueryValueExW(
                    key,
                    nullptr,
                    nullptr,
                    &type,
                    reinterpret_cast<LPBYTE>(buf.data()),
                    &size) != ERROR_SUCCESS)
                {
                    return false;
                }
                value = buf.data();
                return true;
            }

            std::wstring expand(const std::wstring& value)
            {
                const DWORD size = ExpandEnvironmentStringsW(value.c_str(), nullptr, 0);
                if (0 == size)
                {
                    return value;
                }
                std::vector<wchar_t> buf(size, 0);
                ExpandEnvironmentStringsW(value.c_str(), buf.data(), size);
                return buf.data();
            }

            bool repair(
                const std::shared_ptr<ftk::Context>& context,
                const std::wstring& subKey,
                const std::filesystem::path& exe)
            {
                bool out = false;
                HKEY key = nullptr;
                if (RegOpenKeyExW(
                    HKEY_CURRENT_USER,
                    subKey.c_str(),
                    0,
                    KEY_QUERY_VALUE | KEY_SET_VALUE,
                    &key) != ERROR_SUCCESS)
                {
                    return out;
                }
                std::wstring value;
                DWORD type = 0;
                if (readString(key, value, type))
                {
                    const std::string command = ftk::fromWide(
                        REG_EXPAND_SZ == type ? expand(value) : value);
                    const auto repaired = models::repairShellCommand(
                        command,
                        exe,
                        [](const std::filesystem::path& path)
                        {
                            std::error_code ec;
                            return std::filesystem::exists(path, ec);
                        });
                    if (repaired)
                    {
                        const std::wstring w = ftk::toWide(*repaired);
                        const LSTATUS status = RegSetValueExW(
                            key,
                            nullptr,
                            0,
                            type,
                            reinterpret_cast<const BYTE*>(w.c_str()),
                            static_cast<DWORD>((w.size() + 1) * sizeof(wchar_t)));
                        const std::string name = "HKCU\\" + ftk::fromWide(subKey);
                        if (ERROR_SUCCESS == status)
                        {
                            context->log(
                                logPrefix,
                                name + ": " + command + " -> " + *repaired);
                            out = true;
                        }
                        else
                        {
                            context->log(
                                logPrefix,
                                name + ": cannot write, error " + std::to_string(status),
                                ftk::LogType::Warning);
                        }
                    }
                }
                RegCloseKey(key);
                return out;
            }

            bool endsWithInsensitive(const std::wstring& value, const std::wstring& suffix)
            {
                return value.size() > suffix.size() &&
                    0 == _wcsicmp(value.c_str() + value.size() - suffix.size(), suffix.c_str());
            }
        }
#endif // _WIN32

        void repairShellAssociations(const std::shared_ptr<ftk::Context>& context)
        {
#if defined(_WIN32)
            std::filesystem::path exe = ftk::getExePath();
            if (exe.empty())
            {
                return;
            }
            // The console build sits beside the application with the same
            // name. It is the application a file should open in, never the
            // console copy, so a ".com" answers for the ".exe" -- if it is
            // there.
            if (ftk::compare(
                ftk::fromFileSystem(exe.extension()),
                ".com",
                ftk::CaseCompare::Insensitive))
            {
                exe.replace_extension(".exe");
                std::error_code ec;
                if (!std::filesystem::exists(exe, ec))
                {
                    return;
                }
            }

            const std::wstring classes = L"Software\\Classes\\";
            std::vector<std::wstring> subKeys;
            subKeys.push_back(
                classes + L"Applications\\" + exe.filename().wstring() + L"\\shell\\open\\command");

            // Every "Always use this app" class in the user's hive. The names
            // are gathered first so that nothing is written while they are
            // being enumerated. Only the command is looked at, so one made for
            // another application is passed over by the check that follows.
            HKEY key = nullptr;
            if (RegOpenKeyExW(
                HKEY_CURRENT_USER,
                L"Software\\Classes",
                0,
                KEY_ENUMERATE_SUB_KEYS,
                &key) == ERROR_SUCCESS)
            {
                // 255 is the longest a key name can be.
                wchar_t name[256];
                for (DWORD i = 0;; ++i)
                {
                    DWORD size = 256;
                    const LSTATUS status = RegEnumKeyExW(
                        key, i, name, &size, nullptr, nullptr, nullptr, nullptr);
                    if (ERROR_NO_MORE_ITEMS == status)
                    {
                        break;
                    }
                    if (status != ERROR_SUCCESS)
                    {
                        continue;
                    }
                    const std::wstring subKey(name, size);
                    if (endsWithInsensitive(subKey, L"_auto_file"))
                    {
                        subKeys.push_back(classes + subKey + L"\\shell\\open\\command");
                    }
                }
                RegCloseKey(key);
            }

            bool changed = false;
            for (const auto& subKey : subKeys)
            {
                changed |= repair(context, subKey, exe);
            }
            if (changed)
            {
                // Explorer caches associations; tell it they moved.
                SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
            }
#else // _WIN32
            (void)context;
#endif // _WIN32
        }
    }
}
