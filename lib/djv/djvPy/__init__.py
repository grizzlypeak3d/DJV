# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""DJV is an open source application for playback and review of image
sequences.

The API is the C++ one, compiled into the _djv module and gathered here: the
models the application is built from and its widgets. The package also
carries the application itself, which the djv command runs (see main()).
"""

import os as _os
import sys as _sys

# The module takes its types from tlrender, which brings feather_tk and
# opentimelineio, and makes their libraries loadable on Windows.
import tlrender as _tlrender

_here = _os.path.dirname(_os.path.abspath(__file__))

# Python 3.8 stopped finding an extension module's DLLs through PATH on
# Windows, so the directories they are in are named here, before the module
# is imported: bin in a wheel, and the prefix's bin when the package is
# installed to <prefix>/lib/djv. The handles are kept, since closing one
# takes its directory away again.
_dll_directories = []
if _sys.platform == "win32":
    for _dir in (
            _os.path.join(_here, "bin"),
            _os.path.join(_here, _os.pardir, _os.pardir, "bin")):
        if _os.path.isdir(_dir):
            _dll_directories.append(_os.add_dll_directory(_dir))

from ._djv import *
from ._djv import __doc__

__version__ = VERSION_FULL


def _package_dir(module):
    return _os.path.dirname(_os.path.abspath(module.__file__))


def main():
    """Run the DJV application with the command line's arguments: the djv
    command a wheel installs. The application is the C++ one, in the
    package's bin directory in a wheel and the prefix's otherwise."""
    for bin_dir in (
            _os.path.join(_here, "bin"),
            _os.path.join(_here, _os.pardir, _os.pardir, "bin")):
        # The console build on Windows, so that the command line's output
        # reaches the terminal.
        exe = _os.path.join(
            bin_dir, "djv.com" if _sys.platform == "win32" else "djv")
        if _os.path.isfile(exe):
            break
    else:
        raise FileNotFoundError("The DJV application is not installed with the djv package")

    if _sys.platform == "win32":
        # A child process does not see os.add_dll_directory(), so the DLLs
        # of this package and the ones it depends on are found through PATH
        # instead.
        import subprocess
        import feather_tk
        tl = _package_dir(_tlrender)
        ftk = _package_dir(feather_tk)
        path = [
            bin_dir,
            _os.path.join(tl, "bin"),
            _os.path.join(tl, "lib"),
            _os.path.join(ftk, "bin")]
        env = dict(_os.environ)
        env["PATH"] = _os.pathsep.join(path + [env.get("PATH", "")])
        return subprocess.call([exe] + _sys.argv[1:], env=env)

    # Elsewhere the libraries are found by rpath, and the application
    # replaces this process.
    _os.execv(exe, [exe] + _sys.argv[1:])
