#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""Make the version commits of a release, in feather-tk, tlRender and DJV.

Usage: etc/release.py release FTK TLRENDER DJV [--dry-run]
       etc/release.py dev FTK TLRENDER DJV [--dry-run]

Run from DJV's directory, with the submodules checked out on main.

"release" sets the three versions and clears "-dev", moves the pins in
tlRender's and DJV's pyproject.toml, and points each repository at the one
inside it:

    etc/release.py release 0.17.0 0.25.0 3.8.0

"dev" starts the next version in all three, after the tags:

    etc/release.py dev 0.18.0 0.26.0 3.9.0

Nothing is pushed and nothing is tagged: the commands for those are printed.
The change log is not written either; "release" says so when it has no entry
for the version. See etc/Release.md for the rest of a release.
"""

import argparse
import pathlib
import re
import subprocess
import sys

REPOS = [
    # Innermost first: name, directory, version header, macro prefix, and
    # the name its wheel is pinned by.
    ("feather-tk", "deps/tlRender/deps/ftk", "lib/ftk/Core/Version.h", "FTK", "feather-tk"),
    ("tlRender", "deps/tlRender", "lib/tlRender/Core/Version.h", "TLRENDER", "tlRender"),
    ("DJV", ".", "lib/djv/Models/Version.h", "DJV", None),
]


def git(directory, *args, check=True):
    r = subprocess.run(
        ["git", "-C", str(directory)] + list(args),
        capture_output=True, text=True)
    if check and r.returncode != 0:
        sys.exit("git %s failed in %s:\n%s" % (" ".join(args), directory, r.stderr))
    return r.stdout.strip()


def parse_version(value):
    m = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", value)
    if not m:
        sys.exit('Not a version: "%s" (expected, e.g., 3.8.0)' % value)
    return tuple(int(i) for i in m.groups())


def set_version(path, prefix, version, dev):
    text = path.read_text()
    for name, value in zip(("MAJOR", "MINOR", "PATCH"), version):
        text, n = re.subn(
            r"(#define %s_VERSION_%s) \d+" % (prefix, name),
            r"\g<1> %d" % value, text)
        if n != 1:
            sys.exit("%s: no %s_VERSION_%s" % (path, prefix, name))
    text, n = re.subn(
        r'(#define %s_VERSION_DEV) "[^"]*"' % prefix,
        r'\g<1> "%s"' % dev, text)
    if n != 1:
        sys.exit("%s: no %s_VERSION_DEV" % (path, prefix))
    path.write_text(text)


def set_pin(path, name, version):
    text = path.read_text()
    text, n = re.subn(
        r'("%s==)[^"]*(")' % re.escape(name),
        r"\g<1>%s\g<2>" % version, text)
    if n < 1:
        sys.exit('%s: no pin of %s' % (path, name))
    path.write_text(text)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("kind", choices=["release", "dev"])
    parser.add_argument("ftk")
    parser.add_argument("tlrender")
    parser.add_argument("djv")
    parser.add_argument("--dry-run", action="store_true",
                        help="check, and say what would be done")
    args = parser.parse_args()

    root = pathlib.Path(".")
    if not (root / "lib/djv/Models/Version.h").exists():
        sys.exit("Run this from DJV's directory")
    versions = [args.ftk, args.tlrender, args.djv]
    parsed = [parse_version(v) for v in versions]
    dev = "-dev" if "dev" == args.kind else ""

    # Nothing is changed unless everything can be: a release half made in
    # three repositories is worse than one not started.
    for name, directory, header, _, _ in REPOS:
        d = root / directory
        if not (d / header).exists():
            sys.exit("%s is not checked out in %s" % (name, d))
        branch = git(d, "branch", "--show-current")
        if branch != "main":
            sys.exit("%s is on %s, not main" % (name, branch or "no branch"))
        # The pointer to a submodule shows as a change, and is expected: it
        # is one of the things committed here.
        dirty = [
            line for line in git(d, "status", "--porcelain").splitlines()
            if line[3:] not in ("deps/ftk", "deps/tlRender")]
        if dirty:
            sys.exit("%s has changes that are not committed:\n%s" %
                     (name, "\n".join(dirty)))
        behind = git(d, "rev-list", "--count", "HEAD..origin/main", check=False)
        if behind not in ("", "0"):
            sys.exit("%s is %s commit(s) behind origin/main; pull first" %
                     (name, behind))

    commits = []

    def commit(directory, message, paths):
        if args.dry_run:
            print("  %-10s %s" % (directory if directory != "." else "DJV", message))
            return
        git(root / directory, "add", *paths)
        # A pointer that already names the commit has nothing to add.
        if git(root / directory, "diff", "--cached", "--name-only"):
            git(root / directory, "commit", "-q", "-m", message)
            commits.append((
                directory,
                git(root / directory, "rev-parse", "--short", "HEAD"),
                message))

    if args.dry_run:
        print("Would commit:")

    pins = []
    for (name, directory, header, prefix, pin), version, numbers in zip(
            REPOS, versions, parsed):
        d = root / directory
        inner = {"deps/tlRender": "deps/ftk", ".": "deps/tlRender"}.get(directory)
        inner_name = {"deps/tlRender": "feather-tk", ".": "tlRender"}.get(directory)

        if "release" == args.kind:
            # Built against the release inside it, then its own version.
            if inner:
                if not args.dry_run:
                    for pin_name, pin_version in pins:
                        set_pin(d / "pyproject.toml", pin_name, pin_version)
                commit(
                    directory,
                    "%s builds against %s %s" % (name, inner_name, pins[-1][1]),
                    [inner, "pyproject.toml"])
            if not args.dry_run:
                set_version(d / header, prefix, numbers, dev)
            commit(directory, "Version %s" % version, [header])
        else:
            if not args.dry_run:
                set_version(d / header, prefix, numbers, dev)
            commit(directory, "Version %s-dev" % version, [header])
            if inner:
                commit(directory, "Update %s" % inner_name, [inner])

        if pin:
            pins.append((pin, version))

    if args.dry_run:
        return

    for directory, commit_hash, message in commits:
        print("%-10s %s  %s" % (
            directory if directory != "." else "DJV", commit_hash, message))

    print()
    if "release" == args.kind:
        changelog = (root / "ChangeLog.md").read_text()
        if not changelog.startswith("## %s\n" % args.djv):
            print("ChangeLog.md has no entry for %s yet." % args.djv)
            print()
        print("Push, innermost first, and then tag:")
    else:
        print("Push, innermost first:")
    for name, directory, _, _, _ in REPOS:
        print("    git -C %s push origin main" % (root / directory).resolve())
    if "release" == args.kind:
        print()
        print("The tags, which can be pushed together -- each wheel build "
              "waits for the wheel it pins:")
        for (name, directory, _, _, _), version in zip(REPOS, versions):
            d = (root / directory).resolve()
            print("    git -C %s tag %s && git -C %s push origin %s" %
                  (d, version, d, version))


if __name__ == "__main__":
    main()
