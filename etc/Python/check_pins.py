# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""Whether the wheels can be built from this checkout.

Usage: check_pins.py [--pyproject FILE] [--submodule PATH] [--header PATH]

The wheels are built against the tlRender wheel pinned in pyproject.toml,
taking its headers and libraries from that package. Between releases main
moves past the pin: the tlRender submodule is a development version with
an API the published wheel does not have, and a build on main then fails
on the first missing symbol, which says nothing anyone did not know. So a
build that is not a release first asks whether the pin and the submodule
agree, and is skipped with a notice when they do not; the tag that
publishes still builds, and still fails if the pins are wrong, which is
when that matters.

The submodule is not checked out for a wheel build. Its commit is in the
tree, and the version header is fetched from GitHub at that commit.

Prints build=true or build=false in the form GITHUB_OUTPUT takes.
"""

import argparse
import pathlib
import re
import subprocess
import sys
import tomllib
import urllib.request

RAW = "https://raw.githubusercontent.com/grizzlypeak3d/tlRender/{commit}/{path}"


def pin(pyproject, name):
    with open(pyproject, "rb") as f:
        data = tomllib.load(f)
    for requirement in data["build-system"]["requires"]:
        if requirement.lower().startswith(name.lower() + "=="):
            return requirement.split("==", 1)[1].strip()
    raise KeyError(f"{name} is not pinned in {pyproject}")


def submodule_commit(path):
    out = subprocess.run(
        ["git", "ls-tree", "HEAD", path],
        check=True, capture_output=True, text=True).stdout.split()
    # mode type commit path
    return out[2]


def submodule_version(commit, header):
    with urllib.request.urlopen(RAW.format(commit=commit, path=header), timeout=60) as r:
        text = r.read().decode()
    numbers = {}
    for key in ("MAJOR", "MINOR", "PATCH"):
        m = re.search(r"VERSION_" + key + r" ([0-9]+)", text)
        if not m:
            raise ValueError(f"VERSION_{key} not found in {header}")
        numbers[key] = m.group(1)
    dev = re.search(r'VERSION_DEV "([^"]*)"', text)
    return "{MAJOR}.{MINOR}.{PATCH}".format(**numbers) + (dev.group(1) if dev else "")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--pyproject", default="pyproject.toml")
    parser.add_argument("--submodule", default="deps/tlRender")
    parser.add_argument("--header", default="lib/tlRender/Core/Version.h")
    args = parser.parse_args()

    pinned = pin(args.pyproject, "tlRender")
    commit = submodule_commit(args.submodule)
    version = submodule_version(commit, args.header)
    build = pinned == version
    if build:
        print(f"The tlRender pin {pinned} is the submodule's version", file=sys.stderr)
    else:
        # A GitHub notice: visible on the run without failing it.
        print(
            f"::notice title=Wheels not built::The tlRender pin {pinned} is "
            f"behind the submodule ({version}); the wheels are built at the "
            "release, once that tlRender is on PyPI.")
    print(f"build={'true' if build else 'false'}")


if __name__ == "__main__":
    main()
