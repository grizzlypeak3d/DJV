# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""Write a type stub for a module that is not installed yet.

Usage: make_stub.py --dll-dir <dir>... -- <stubgen.py> <stubgen arguments>...

nanobind's stubgen imports the module to read its bindings from it, and in
the build tree the libraries it links are not beside it: on Windows there
is no rpath, and since Python 3.8 an extension module's dependencies are
found through os.add_dll_directory() alone -- not PATH. So the directories
they are in are named here, before stubgen imports anything. The same
reason tests/run-python-tests.py exists.

Elsewhere the rpath recorded in the module covers it and the directories
are ignored.
"""

import os
import runpy
import sys


def main():
    dll_dirs = []
    argv = sys.argv[1:]
    while argv and argv[0] == "--dll-dir":
        dll_dirs.append(argv[1])
        argv = argv[2:]
    if not argv or argv[0] != "--":
        raise SystemExit("make_stub.py: expected -- before the stubgen command")
    argv = argv[1:]
    if not argv:
        raise SystemExit("make_stub.py: no stubgen script given")

    handles = []
    searched = []
    if hasattr(os, "add_dll_directory"):
        for directory in dll_dirs:
            if os.path.isdir(directory):
                # Kept: closing a handle takes its directory away again.
                handles.append(os.add_dll_directory(directory))
                searched.append(directory)

    stubgen, sys.argv = argv[0], argv
    try:
        runpy.run_path(stubgen, run_name="__main__")
    except ImportError as e:
        # A missing DLL on Windows says only that something could not be
        # found, never what: the directories that were searched are the
        # place to start, so they are printed rather than guessed at.
        print(f"make_stub.py: {e}", file=sys.stderr)
        for directory in searched:
            print(f"  searched: {directory}", file=sys.stderr)
        for directory in dll_dirs:
            if directory not in searched:
                print(f"  not there: {directory}", file=sys.stderr)
        raise


if __name__ == "__main__":
    main()
