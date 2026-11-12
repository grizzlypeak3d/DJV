# DJV release checklist

The repositories nest: DJV -> tlRender -> ftk. Anything touching more than
one of them is done innermost first.

DJV Studio is released separately and from its own checklist. Its version is
always >= DJV's, so a DJV release is also a deadline for one there. The two
share major and minor numbers -- a Studio license covers a major version, and
"Studio 3.8" says which DJV it is built on -- and Studio may make patch
releases of its own. A version number is never used twice across the two:
after a Studio-only 3.7.7, DJV's next release is 3.7.8 or 3.8.0, not a
different 3.7.7.

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

`etc/release.py` makes the commits the next items and the pins under PyPI
describe, in all three repositories, and prints what to push and tag:

    etc/release.py release 0.17.0 0.25.0 3.8.0

It refuses unless all three are on main, clean and up to date, and it
pushes and tags nothing. The change log is still written by hand.

feather-tk and tlRender are also released between DJV releases, for what
their own users need. A DJV version of `-` does that: the two are released,
and DJV keeps its version and its PyPI pins, so the DJV wheel already
published goes on installing the releases it was built against; only DJV's
pointer to tlRender moves. The rest of this checklist then applies to the
two of them, the wheels built by hand before tagging included.

    etc/release.py release 0.18.0 0.26.0 -
    etc/release.py dev 0.19.0 0.27.0 -

Their numbers: the minor is bumped for a release with changes, which every
DJV release is; the patch for a release that only fixes what the one before
got wrong, so "0.18.1 fixes 0.18.0" is what pip users read. tlRender releases
with feather-tk either way, since its wheel pins feather-tk exactly and a
feather-tk fix reaches `pip install tlRender` only through tlRender's pin:

    etc/release.py release 0.18.1 0.26.1 -
    etc/release.py dev 0.19.0 0.27.0 -

tlRender can be released alone, when feather-tk has nothing new: a
feather-tk version of `-` keeps the feather-tk release tlRender already
pins, and points tlRender at that release's tag for the release commit, so
tlRender's tag records the feather-tk its wheel is built with; `dev` points
it back at feather-tk's main. feather-tk is never released without
tlRender.

    etc/release.py release - 0.26.2 -
    etc/release.py dev - 0.27.0 -

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
- [ ] The VFX Reference Platform table (`docs/install.html`) against
      vfxplatform.com and the versions in tlRender's
      `etc/SuperBuild/cmake/Modules/Build*.cmake`: a new platform year is
      final each August, and a dependency bump changes DJV's column.
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
- [ ] Continuous integration builds every package, on every push: Linux,
      Windows (installer and zip), and macOS for both architectures, the
      Intel one cross compiled. They are the artifacts of the CI workflow's
      `*-package` jobs. Linux is built in a Rocky Linux container so that
      it starts on the distributions people run; building it here with
      `package-linux.sh` produces something linked against whatever this
      machine has.
- [ ] A tag's CI run also makes a **draft release** on GitHub, with the
      Linux and Windows packages attached and the change log's entry for
      the version as its notes. Nothing is published until the draft is.
- [ ] The macOS packages are built unsigned, since the signing identity is
      not in CI. Download the two `.dmg` artifacts of the tag's run, and
      sign and notarize them here:

      sh etc/macOS/sign-release.sh --upload <tag> djv-<tag>-macos-arm64.dmg djv-<tag>-macos-x86_64.dmg

      It takes the application out of each, signs it, makes the disk image
      again under the same name, notarizes and staples it, asks Gatekeeper,
      and adds the result to the draft (`--upload` needs the GitHub CLI;
      without it the signed images are in `signed/`). It needs
      `DJV_MACOS_TEAM_ID`, and for notarization either
      `DJV_MACOS_NOTARY_PROFILE` or `DJV_MACOS_USER_ID` and
      `DJV_MACOS_USER_PASSWORD`.
- [ ] The **Windows packages of a tag are signed** in CI, by Azure Artifact
      Signing: the programs, the uninstaller (as NSIS compiles the
      installer; see `cmake/CPackProjectConfig.cmake`) and then the
      installer. The job installs the package on the runner and checks the
      uninstaller it wrote. The `windows-package`
      job waits in the `signing` environment for a reviewer's approval
      before it starts, so approve it in the run's page. Its "Check the
      signature" step lists the installer as `Valid`; check on a Windows
      machine as well (Properties > Digital Signatures on the installer,
      and on `djv.exe` once installed). A push that is not a tag is built
      unsigned. See "Signing (Windows)" below.
- [ ] To build a package here all the same: `package-macos.sh` and
      `package-win.bat` take the source directory and build type, and
      choose the config themselves. The Intel Mac package is a second run
      of `package-macos.sh`, with `x64` as its third argument, in a
      directory of its own (`etc/Config/package-macos-x64.cmake`).
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

## Signing (Windows)

The key stays in Azure and GitHub holds nothing secret. The Windows package
job proves who it is with a token GitHub issues for the run (OpenID Connect),
which Azure accepts only from this repository's `signing` environment. Only a
tag, or a run started by hand with "Sign the Windows packages" ticked, uses
the environment; a pull request from a fork cannot.

To make a signed package without a release: Actions > CI > Run workflow, with
the box ticked. To find out in a few minutes whether signing is set up at
all: Actions > Signing check > Run workflow.

One-time setup. In the Azure portal, on the app registration that already
signs DJV Studio:

- [ ] Certificates & secrets > Federated credentials > Add credential, with
      the scenario **Other issuer** (the GitHub scenario saves a subject
      GitHub does not send): issuer
      `https://token.actions.githubusercontent.com`, subject identifier
      `repo:grizzlypeak3d/DJV:environment:signing`, audience left as
      `api://AzureADTokenExchange`.

The role on the Artifact Signing account belongs to the app registration,
so it is already there.

In GitHub, Settings > Environments > New environment, named `signing`:

- [ ] Required reviewers: on, with whoever releases.
- [ ] Deployment branches and tags: `main`, and tags.
- [ ] Environment variables (not secrets -- none is one), the same values
      as DJV Studio's: `AZURE_CLIENT_ID`, `AZURE_TENANT_ID`,
      `AZURE_SUBSCRIPTION_ID`, `AZURE_SIGNING_ENDPOINT`,
      `AZURE_SIGNING_ACCOUNT`, `AZURE_SIGNING_PROFILE`.

The actions the job uses are pinned to commits: whatever runs in that job
can ask for the token that signs, and a tag can be moved to different code.

## Tags

- [ ] **Last, once the packages are built and one has been installed and
      checked** -- the packages of the release commit's own CI run, which a
      push to main builds before any tag exists. Nothing in the build reads a tag -- `BuildInfo.cmake` asks
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
- [ ] The three tags can be pushed together. A wheel build fetches the
      wheels it pins from PyPI, so tlRender's needs feather-tk's published
      and DJV's needs tlRender's; each build asks for them first
      (`etc/Python/wait_for_pins.py`) and a tag waits up to an hour. Each
      publish job still needs approving in turn, and the next build goes
      on once the one before is published. This replaced tagging one at a
      time and watching the index, which lags the release by minutes and
      whose servers do not catch up together: tags pushed minutes too soon
      failed a wheel job, and the sdist.
- [ ] **Build the wheels by hand before tagging**: Actions > Wheels > Run
      workflow on main, with "Publish to TestPyPI" left off. A wheel build
      on main only runs when the packaging files change, so a change to the
      C++ can break the wheel without anything noticing until the tag; the
      CI workflow compiles against tlRender's source tree, a wheel against
      the installed tlRender wheel, which lacks the optional formats'
      headers. 3.7.5's first tag failed this way, on an unguarded include
      of `tlRender/IO/USD.h`, and the tag had to be moved. The pins have to
      be on PyPI for this, so for DJV it runs once feather-tk's and
      tlRender's tags have published; tag DJV after it is green.
- [ ] Should a job fail that way, wait for the rest of the run to finish and
      use "Re-run failed jobs".
- [ ] Should only the publish job fail, say from a publisher set up wrongly
      on PyPI, fix it there and use "Re-run failed jobs": the wheels already
      built are kept.

## After the release

- [ ] Start the next version in all three: the next minor number and
      `VERSION_DEV` back to `"-dev"` ("Version X.Y.Z-dev"), with
      `etc/release.py dev 0.18.0 0.26.0 3.9.0`.

## Worth knowing

- Each CI job caches its installed dependencies (`install-<type>`, a few
  hundred megabytes) and skips building them when its key matches: the
  superbuild files, `etc/Config/**`, the platform's `sbuild` script, and on
  Linux the setup scripts, plus the job's config. Editing the workflow does
  not bust it any more; changing any of those does, for about a 20 minute
  rebuild of the jobs affected. The `sbuild` scripts take
  `SBUILD_STAGES=deps` or `project` to build one half.
- FFmpeg is pinned in
  `deps/tlRender/etc/SuperBuild/cmake/Modules/BuildFFmpeg.cmake`. Moving that
  pin also means updating the library versions written out in the macOS
  install names and in the packaging: check them against
  `libavutil/version.h` and its siblings at the new tag. In this tree they
  are in `BuildFFmpeg.cmake` and `cmake/Modules/Package.cmake`.
