# SPDX-License-Identifier: BSD-3-Clause

"""Run the Python tests with the shared libraries findable.

Python 3.8 stopped resolving an extension module's dependencies through PATH
on Windows. It searches the directory the module itself is in, the system
directories, and whatever os.add_dll_directory() has been told about, and
nothing else. The bindings link FFmpeg and LibRaw, which install to bin, so
importing them without this fails with

    ImportError: DLL load failed while importing tlRenderPy:
    The specified module could not be found.

and no amount of PATH helps. Any directories given on the command line are
added before the tests are discovered; elsewhere the loader follows the
rpath recorded in the libraries and there is nothing to do.
"""

import os
import sys
import unittest

for directory in sys.argv[1:]:
    if hasattr(os, "add_dll_directory") and os.path.isdir(directory):
        os.add_dll_directory(directory)

start = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, start)
suite = unittest.defaultTestLoader.discover(start)
result = unittest.TextTestRunner(verbosity=2).run(suite)
sys.exit(0 if result.wasSuccessful() else 1)
