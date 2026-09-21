# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""Report the signatures a type stub could not resolve.

Usage: check_stub.py <stub.pyi>...

nanobind writes a stub from the bindings themselves, and where a C++ type
was never bound it writes the C++ name as a string annotation:

    def setCloseCallback(self, arg: "std::function<void ()>", /) -> None: ...

That is not a type Python knows, so editors and type checkers make nothing
of it: the argument is as opaque as it was without the stub. Each one is a
binding to fix -- a caster header the translation unit did not include, an
enum or an observable never bound -- so they are printed as warnings, with
the count, rather than left in the file for someone to notice.

Warnings only: an unresolved signature is worth fixing, not worth stopping
a build over, and the stub is still correct for everything else.
"""

import re
import sys

# A quoted annotation holding a C++ name: "ftk::Foo", "std::function<...>".
# The stub quotes a forward reference to one of its own classes the same
# way, which is Python and resolves; the "::" is what marks the C++ ones.
UNRESOLVED = re.compile(r'"([^"]*::[^"]*)"')


def main():
    total = 0
    for path in sys.argv[1:]:
        found = []
        with open(path, encoding="utf-8") as f:
            for number, line in enumerate(f, 1):
                for match in UNRESOLVED.finditer(line):
                    found.append((number, match.group(1), line.strip()))
        if found:
            total += len(found)
            print(f"{path}: {len(found)} unresolved "
                  f"{'signature' if len(found) == 1 else 'signatures'}:")
            for number, name, line in found:
                print(f"  {path}:{number}: {line}")
    if total:
        print(f"{total} unresolved: each is a C++ type the bindings do not "
              f"expose to Python. See etc/Python/check_stub.py.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
