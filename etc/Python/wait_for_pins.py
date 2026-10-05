# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""Wait for the wheels pyproject.toml pins to be on PyPI.

Usage: wait_for_pins.py [--timeout MINUTES] [--pyproject FILE]

A release tags the projects it is made of one after another, and each is
built against the wheel of the one before, which is on PyPI only once that
one's own build has finished and been published. Tagged a few minutes too
soon, this build failed where it first asked for the wheel. So it asks here
first, and waits: the tags can then be pushed together.

Asked through pip, for this machine, since that is what the build then does:
the index has listed a file before every one of its servers would hand it
over.

Only a tag waits. Any other build asks once, since the pins of an unreleased
version are on main before the release they name, and waiting would not
bring it.
"""

import argparse
import os
import pathlib
import subprocess
import sys
import tempfile
import time
import tomllib

# Seconds between asking, and after an answer that had to be waited for: the
# index's servers do not catch up together.
INTERVAL = 30
SETTLE = 120


def pins(pyproject):
    """The requirements the build needs at one exact version."""
    with open(pyproject, "rb") as f:
        requires = tomllib.load(f)["build-system"]["requires"]
    return [r.strip() for r in requires if "==" in r]


def available(requirement):
    """Whether pip can get a wheel of the requirement for this machine."""
    with tempfile.TemporaryDirectory() as tmp:
        r = subprocess.run(
            [
                sys.executable, "-m", "pip", "download",
                "--quiet", "--disable-pip-version-check", "--no-cache-dir",
                "--no-deps", "--only-binary", ":all:",
                "--dest", tmp, requirement,
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL)
    return 0 == r.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--timeout", type=float, default=60.0,
                        help="minutes to wait on a tag (default: 60)")
    parser.add_argument("--pyproject", type=pathlib.Path,
                        default=pathlib.Path("pyproject.toml"))
    args = parser.parse_args()

    wait = "tag" == os.environ.get("GITHUB_REF_TYPE")
    end = time.monotonic() + args.timeout * 60
    waited = False
    for requirement in pins(args.pyproject):
        while not available(requirement):
            if not wait:
                sys.exit(
                    "%s is not on PyPI. That is expected on a branch whose "
                    "pins name a release not made yet; the tag's build waits "
                    "for it." % requirement)
            if time.monotonic() > end:
                sys.exit(
                    "%s was not on PyPI after %g minutes" %
                    (requirement, args.timeout))
            print("Waiting for %s" % requirement, flush=True)
            waited = True
            time.sleep(INTERVAL)
        print("%s is on PyPI" % requirement, flush=True)
    if waited:
        time.sleep(SETTLE)


if __name__ == "__main__":
    main()
