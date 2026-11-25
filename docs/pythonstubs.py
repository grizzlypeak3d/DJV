# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""Copy the Python modules' type stubs into the documentation, or check them.

Usage: pythonstubs.py [--check] <out dir> <search dir>...

The stubs a Python build writes are the API reference: every class and
method of feather_tk, tlrender and djv with its signature, and its
description from the C++ headers (see etc/Python/doc_stub.py). They are
copied here, one file a module, so that the website and the installed
documentation have them for people and agents who have not built anything.
They are named .pyi.txt so that a web server says they are text: a .pyi
it does not know is sent as a download.

The check compares what the stubs say rather than their text: the classes,
the functions and the properties, and their descriptions. The formatting
stubgen gives a signature can differ with the Python it runs under, and a
build with another Python is no reason to fail.
"""

import argparse
import ast
import os
import sys


# The published name, and where the stub is in a built package.
STUBS = [
    ("feather_tk.pyi.txt", "feather_tk/_feather_tk.pyi"),
    ("tlrender.pyi.txt", "tlrender/_tlrender/__init__.pyi"),
    ("tlrender.ui.pyi.txt", "tlrender/_tlrender/ui.pyi"),
    ("djv.models.pyi.txt", "djv/_djv/models.pyi"),
    ("djv.ui.pyi.txt", "djv/_djv/ui.pyi"),
    ("djv.app.pyi.txt", "djv/_djv/app.pyi"),
]

HEADER = '''# The Python API of {module}: generated from the bindings by nanobind's
# stubgen, with each description taken from the C++ headers. Do not edit:
# see docs/pythonstubs.py. Import it as "import {package}".

'''


def find(rel, search):
    for d in search:
        path = os.path.join(d, rel)
        if os.path.isfile(path):
            return path
    return None


def published(name, path):
    module = name[:-len(".pyi.txt")]
    with open(path, encoding="utf-8") as f:
        text = f.read()
    return HEADER.format(module=module, package=module.split(".")[0]) + text


def summary(text):
    """What a stub says: each class, function and property, and its
    description."""
    out = []

    def visit(node, prefix):
        for child in node.body:
            if isinstance(child, ast.ClassDef):
                name = prefix + child.name
                out.append((name, ast.get_docstring(child)))
                visit(child, name + ".")
            elif isinstance(child, ast.FunctionDef):
                out.append((prefix + child.name, ast.get_docstring(child)))
            elif isinstance(child, ast.Assign):
                for t in child.targets:
                    if isinstance(t, ast.Name):
                        out.append((prefix + t.id, None))
    visit(ast.parse(text), "")
    return sorted(set(out), key=lambda x: (x[0], x[1] or ""))


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--check", action="store_true",
                        help="Check the copies against the build's stubs.")
    parser.add_argument("out", help="The directory of the copies.")
    parser.add_argument("search", nargs="+", help="Where the built packages are.")
    args = parser.parse_args()

    failed = False
    for name, rel in STUBS:
        src = find(rel, args.search)
        if not src:
            print("pythonstubs.py: no %s in %s" % (rel, ", ".join(args.search)))
            failed = True
            continue
        text = published(name, src)
        dst = os.path.join(args.out, name)
        if args.check:
            have = open(dst, encoding="utf-8").read() if os.path.isfile(dst) else ""
            a = summary(have) if have else []
            b = summary(text)
            if a != b:
                missing = [x[0] for x in b if x not in a][:10]
                extra = [x[0] for x in a if x not in b][:10]
                print("pythonstubs.py: %s is out of date; build djvDocsPythonStubs" % dst)
                if missing:
                    print("  new or changed: " + ", ".join(missing))
                if extra:
                    print("  gone or changed: " + ", ".join(extra))
                failed = True
        else:
            os.makedirs(args.out, exist_ok=True)
            with open(dst, "w", encoding="utf-8", newline="\n") as f:
                f.write(text)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
