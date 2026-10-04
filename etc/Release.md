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
- [ ] Clear `VERSION_DEV` (`"-dev"` -> `""`). `VERSION_FULL`, which names
      the package files, derives from the numbers and the suffix, so those
      four defines are the whole edit. Clearing `VERSION_DEV` is also what
      drops the date and commit from the window title.
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
      `MINIMAL=ON PLUGIN=ON` -- the codecs that need no license, with the
      plugin's command line fallback for bringing your own.
- [ ] The Intel Mac package is a second run of `package-macos.sh`, with
      `x64` as its third argument, in a directory of its own
      (`etc/Config/package-macos-x64.cmake`). It is cross-compiled, so it
      runs here under Rosetta, which is a check that it starts and not a
      test on an Intel Mac. It is signed and notarized as the other is.
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

## PyPI

Pushing a version tag builds that repository's wheels and publishes them
(feather-tk, tlRender, DJV-viewer), once the publish job is approved in the
`pypi` environment on GitHub.

- [ ] Move the pins with the versions: tlRender's `pyproject.toml` pins
      feather-tk, and DJV's pins tlRender and feather-tk, each to an exact
      version. They go in with the `Version.h` edits (the "builds against"
      commits), so that the tags name what was released.
- [ ] Expect the wheel builds on main to fail once the pins are pushed: a
      change to `pyproject.toml` builds the wheels, and the versions they pin
      are not on PyPI until the tags are. The tag's build is the one that
      counts; "CI green" above means the CI workflow.
- [ ] Tag innermost first, one at a time, since a wheel build fetches the
      wheels it pins from PyPI: ftk, then tlRender, then DJV. Before each
      next tag, wait until `https://pypi.org/simple/<name>/` lists the new
      files -- the wheels and the sdist, not their provenance entries -- and
      then a few minutes more. The index lags the release by minutes and its
      servers do not catch up together: a tlRender tag pushed three minutes
      after feather-tk published failed a wheel job, and one pushed a minute
      after the index listed every file here failed the sdist, which needs
      the pinned wheel in its first seconds.
- [ ] Should a job fail that way, wait for the rest of the run to finish and
      use "Re-run failed jobs".
- [ ] Should only the publish job fail, say from a publisher set up wrongly
      on PyPI, fix it there and use "Re-run failed jobs": the wheels already
      built are kept.

## After the release

- [ ] Start the next version in all three: the next minor number and
      `VERSION_DEV` back to `"-dev"` ("Version X.Y.Z-dev").

## Worth knowing

- Editing `.github/workflows/ci-workflow.yml` busts the dependency cache and
  costs about a 20 minute rebuild, because the cache key hashes it. Batch CI
  edits into one change.
- FFmpeg is pinned in
  `deps/tlRender/etc/SuperBuild/cmake/Modules/BuildFFmpeg.cmake`. Moving that
  pin also means updating the library versions written out in the macOS
  install names and in the packaging: check them against
  `libavutil/version.h` and its siblings at the new tag. In this tree they
  are in `BuildFFmpeg.cmake` and `cmake/Modules/Package.cmake`.
