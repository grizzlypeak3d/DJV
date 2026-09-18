# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""Repair a djv-player wheel, leaving tlRender's and feather-tk's libraries
in their own wheels.

Usage: repair_wheel.py <wheel> <dest_dir> [<delocate_archs>]

The wheel links the libraries of the tlrender and feather_tk packages beside
it and names them by rpath. The repair tools would either fail to find them
or copy them in, and a second copy is what the wheel is built not to have:
one process would load two of each. So the tlRender wheel pyproject.toml
pins, and the feather-tk wheel it pins, are installed to a scratch
directory, the tool is shown where their libraries are, and told to leave
every one of them out.

For cibuildwheel's repair-wheel-command.
"""

import os
import subprocess
import sys
import tempfile
import tomllib

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.dirname(os.path.dirname(HERE))


def requirements():
    """The pinned tlRender and feather-tk, from the build requirements."""
    with open(os.path.join(PROJECT, "pyproject.toml"), "rb") as f:
        requires = tomllib.load(f)["build-system"]["requires"]
    out = []
    for name in ("tlrender", "feather-tk"):
        for requirement in requires:
            if requirement.lower().replace("_", "-").startswith(name):
                out.append(requirement)
                break
        else:
            raise RuntimeError(f"pyproject.toml does not require {name}")
    return out


def run(*args, env=None):
    print(" ".join(args), flush=True)
    subprocess.run(args, check=True, env=env)


def main():
    wheel, dest_dir = sys.argv[1], sys.argv[2]
    archs = sys.argv[3] if len(sys.argv) > 3 else None

    with tempfile.TemporaryDirectory() as scratch:
        run(sys.executable, "-m", "pip", "install", "--no-deps",
            "--target", scratch, *requirements())
        packages = [
            os.path.join(scratch, "tlrender"),
            os.path.join(scratch, "feather_tk")]

        if sys.platform == "darwin":
            lib_dirs = [os.path.join(p, "lib") for p in packages]
            env = dict(os.environ, DYLD_LIBRARY_PATH=os.pathsep.join(lib_dirs))
            args = ["delocate-wheel", "-v", "-w", dest_dir,
                    # The rpaths to the packages beside this one are what
                    # finds their libraries once installed.
                    "--no-sanitize-rpaths"]
            for package in packages:
                args += ["--exclude", package]
            if archs:
                args += ["--require-archs", archs]
            args.append(wheel)
            # delocate looks for an @rpath library in the library's rpaths
            # and then in /usr/local/lib and /usr/lib, the way the loader
            # falls back, before it reads DYLD_LIBRARY_PATH. The rpaths to
            # the other packages are nowhere in the wheel it unpacks, so on an
            # Intel Mac, where Homebrew installs to /usr/local, it would find
            # Homebrew's libraries there and copy them in. It is run here with
            # the packages' libraries in place of /usr/local/lib -- the list
            # its own tests replace the same way.
            code = (
                "import sys\n"
                "import delocate.libsana\n"
                f"delocate.libsana._default_paths_to_search = ({', '.join(repr(d) for d in lib_dirs)}, '/usr/lib')\n"
                "from delocate.cmd.delocate_wheel import main\n"
                f"sys.argv = {args!r}\n"
                "main()\n")
            run(sys.executable, "-c", code, env=env)

        elif sys.platform.startswith("linux"):
            lib_dirs = [os.path.join(p, "lib") for p in packages]
            env = dict(os.environ, LD_LIBRARY_PATH=os.pathsep.join(lib_dirs))
            args = ["auditwheel", "repair", "-w", dest_dir]
            for lib_dir in lib_dirs:
                for name in sorted(os.listdir(lib_dir)):
                    if ".so" in name:
                        args += ["--exclude", name]
            run(*args, wheel, env=env)

        elif sys.platform == "win32":
            # tlRender's DLLs are in bin, and OpenTimelineIO's in lib.
            dirs = [
                os.path.join(packages[0], "bin"),
                os.path.join(packages[0], "lib"),
                os.path.join(packages[1], "bin")]
            dirs = [d for d in dirs if os.path.isdir(d)]
            names = []
            for d in dirs:
                names += [n for n in sorted(os.listdir(d)) if n.lower().endswith(".dll")]
            # What the wheel has already is found through its own bin
            # directory, and the other packages' through theirs (see
            # djv/__init__.py); only the C++ runtime is added. Its name is
            # left alone, since the other DLLs import it too and are not
            # rewritten.
            run("delvewheel", "repair", "-v", "-w", dest_dir,
                "--ignore-existing",
                "--no-mangle-all",
                "--add-path", os.pathsep.join(dirs),
                # os.pathsep: delvewheel splits its lists the way the
                # platform splits PATH, which is ";" on Windows.
                "--exclude", os.pathsep.join(names),
                wheel)

        else:
            raise RuntimeError("No repair for " + sys.platform)


if __name__ == "__main__":
    main()
