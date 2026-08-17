# DJV release checklist

The repositories nest: DJV -> tlRender -> ftk. Anything touching more than
one of them is done innermost first.

DJV Studio is released separately and from its own checklist. Its version is
always >= DJV's, so a DJV release is also a deadline for one there.

## Before building anything

- [ ] **Move `etc/Config/local.cmake` aside.** A package build now refuses to
      configure while it is there, so this is a reminder rather than
      something to verify afterwards. Only this one matters: `sbuild` passes
      the top level `-C` file to every stage, so the copies in tlRender and
      ftk are never read.
- [ ] Working trees clean in all three repositories. A package built from a
      dirty tree records a commit that identifies nothing, which is what a
      bug report has to be traced through.

## Versions and change log

DJV drives feather-tk and tlRender releases, so all three are versioned
together and each keeps its own number.

- [ ] Set `VERSION_MAJOR` / `MINOR` / `PATCH` in each `Version.h`:
      `deps/tlRender/deps/ftk/lib/ftk/Core/Version.h`,
      `deps/tlRender/lib/tlRender/Core/Version.h` and
      `lib/djv/Models/Version.h`. Every project regex parses its own header
      from its `CMakeLists.txt`, so those three files are the whole edit.
- [ ] Clear `VERSION_DEV` (`"-dev"` -> `""`) **and** drop the suffix from
      `VERSION_FULL`, which is a separate define rather than something
      derived, and is what names the package files. Clearing `VERSION_DEV` is
      also what drops the date and commit from the window title.
- [ ] `ChangeLog.md`: "Changes" is for user visible changes; "Fixes" is for
      problems someone reported, not every bug fixed on the way.

## Submodules and CI

- [ ] Submodule pointers match each repository's HEAD, from ftk outwards.
- [ ] Push innermost first: ftk, tlRender, DJV. Pushing a superproject first
      leaves it pointing at commits the remote lacks.
- [ ] CI green.

## Documentation

- [ ] Regenerate the screenshots (`etc/Screenshots/build_screenshots.py`) so
      they show the release. The assets carry no version text, so this can be
      done before or after tagging.
- [ ] The Pages deploy runs on a push to main touching `docs/**`.
- [ ] The docs describe the release rather than main: Pages serves whatever
      was last pushed, so anyone on an older version reads about features
      they do not have.

## Packages

- [ ] The configure log must **not** say "etc/Config/local.cmake is in use".
- [ ] Check the FFmpeg split in the configure output. This decides what is
      shipped, so it is a licensing question rather than a build option:
      `MINIMAL=ON PLUGIN=ON CMD=ON` -- the codecs that need no license, the
      plugin for limited exports, and the command line tool for bringing your
      own.
- [ ] macOS and Windows are built here, with `package-macos.sh` and
      `package-win.bat`. They take the source directory and build type, and
      choose the config themselves. Both are built locally because signing
      needs credentials that are not in CI.
- [ ] Linux comes from the CI artifact, which is built in a Rocky Linux
      container so that it starts on the distributions people run. Building
      it here with `package-linux.sh` produces something linked against
      whatever this machine has.
- [ ] Confirm the **expected** artifact appeared, not merely that one did:
      `.dmg` on macOS, an `.exe` installer on Windows, `.tar.gz` on Linux.
      With the platform packaging off a package build still succeeds and
      produces the generic relocatable layout -- a zip on macOS, an installer
      whose sample data and legal documents sit under `share/` on Windows --
      which is a real artifact, just not the one being released. The package
      configs ask for the right one; this is the check that they did.
- [ ] On Windows, that an installer exists at all: the script has to `call`
      sbuild, or the line that makes the package is never reached.
- [ ] Install the package and open About: bare version, no `-dev`, no
      `-dirty`, and a commit that exists on the remote.

## Tags

- [ ] **Last, once the packages are built and one has been installed and
      checked.** Nothing in the build reads a tag -- `BuildInfo.cmake` asks
      for `git rev-parse HEAD` and the versions come from `Version.h` -- so
      tagging earlier buys nothing, while a package build is the first time
      the release configuration runs end to end. Whatever it turns up means
      new commits and moved submodule pointers, and a tag pushed beforehand
      would then name something that is not what shipped. Moving a published
      tag is the one thing tags exist to make unnecessary.
- [ ] Tag all three, at the commits the packages were built from. DJV's tag
      records the submodule SHAs, so checking it out recovers tlRender, ftk
      and the FFmpeg pin together; ftk and tlRender are tagged with their own
      version numbers.

## Worth knowing

- Editing `.github/workflows/ci-workflow.yml` busts the dependency cache and
  costs about a 20 minute rebuild, because the cache key hashes it. Batch CI
  edits into one change.
- FFmpeg is pinned in
  `deps/tlRender/etc/SuperBuild/cmake/Modules/BuildFFmpeg.cmake`. Moving that
  pin also means updating the library versions written out in the macOS
  install names and in the packaging: check them against
  `libavutil/version.h` and its siblings at the new tag. In this tree they
  are in `BuildFFmpeg.cmake`, `deps/tlRender/cmake/Modules/Package.cmake` and
  `cmake/Modules/Package.cmake`.
