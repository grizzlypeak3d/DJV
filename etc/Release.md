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

## Version and change log

- [ ] Set `VERSION_MAJOR` / `MINOR` / `PATCH` in `lib/djv/Models/Version.h`.
- [ ] Clear `VERSION_DEV` (`"-dev"` -> `""`).
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
- [ ] Confirm an installer was actually produced, on Windows especially: the
      script has to `call` sbuild, or the line that makes the package is
      never reached.
- [ ] Install the package and open About: bare version, no `-dev`, no
      `-dirty`, and a commit that exists on the remote.

## Tags

- [ ] Tag each repository at the commit the packages were built from. The
      tag records the submodule SHAs, so checking it out recovers tlRender,
      ftk and the FFmpeg pin together.

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
