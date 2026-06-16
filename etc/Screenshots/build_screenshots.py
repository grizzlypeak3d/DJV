#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.
"""Build all documentation screenshots from a manifest.

One process per shot (state isolation by process boundary), then turn each
PNG + JSON sidecar into an annotated SVG with make_svg.py.

The djv processes are run with their working directory set to the repository
root (--root), so the relative sample paths in the manifest (e.g.
"etc/SampleData/Counter.otio") resolve regardless of where this script is
invoked from.

    etc/Screenshots/build_screenshots.py etc/Screenshots/shots.json --djv ./build/djv

On headless CI, run under a virtual display, e.g.:
    xvfb-run -a etc/Screenshots/build_screenshots.py ... --djv ./build/djv
"""

import argparse
import json
import pathlib
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent # etc/Screenshots
REPO_ROOT = HERE.parent.parent                 # repository root


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("manifest", type=pathlib.Path)
    ap.add_argument("--djv", required=True, help="path to the djv executable")
    ap.add_argument("--root", type=pathlib.Path, default=REPO_ROOT,
                    help="working directory for djv; manifest sample paths "
                         "resolve against this (default: repo root)")
    ap.add_argument("--assets", type=pathlib.Path, default=None,
                    help="docs/assets directory for the SVGs "
                         "(default: <root>/docs/assets)")
    ap.add_argument("--shots-dir", type=pathlib.Path, default=None,
                    help="where to put intermediate PNG/JSON/callouts "
                         "(default: temp)")
    ap.add_argument("--only", nargs="*", help="capture only these shot ids")
    ap.add_argument("--max-width", type=int, default=1280,
                    help="downscale embedded images to this width (0 = full size)")
    ap.add_argument("--format", choices=["png", "webp", "jpeg"], default="png",
                    help="embedded image format (default: png; webp is smallest)")
    ap.add_argument("--page-width", type=int, default=800,
                    help="docs content-column width in px, for label sizing")
    ap.add_argument("--target-font", type=int, default=15,
                    help="desired on-page label size in px")
    args = ap.parse_args()

    # Resolve to absolutes so they survive the subprocess cwd change.
    manifest = args.manifest.resolve()
    root = args.root.resolve()
    djv = str(pathlib.Path(args.djv).resolve())
    assets = (args.assets or (root / "docs" / "assets")).resolve()
    shots_dir = (args.shots_dir.resolve() if args.shots_dir
                 else pathlib.Path(tempfile.mkdtemp(prefix="djv-shots-")))
    assets.mkdir(parents=True, exist_ok=True)
    shots_dir.mkdir(parents=True, exist_ok=True)

    data = json.loads(manifest.read_text())
    by_id = {s["id"]: s for s in data["shots"]}
    ids = [s["id"] for s in data["shots"]]
    if args.only:
        ids = [i for i in ids if i in args.only]

    failures = []
    # Redirect settings to a throwaway file so a local run never reads or
    # overwrites the user's real settings. -resetSettings still guarantees each
    # shot starts from defaults regardless of what a prior shot wrote here.
    settings_file = shots_dir / "capture-settings.json"
    for shot_id in ids:
        cmd = [
            djv,
            "-resetSettings",
            "-settingsFile", str(settings_file),
            "-captureManifest", str(manifest),
            "-captureShot", shot_id,
            "-captureOutput", str(shots_dir),
        ]
        print(f"[capture] {shot_id} (cwd={root})")
        if subprocess.run(cmd, cwd=root).returncode != 0:
            failures.append(shot_id)
            print(f"  ! capture failed for {shot_id}", file=sys.stderr)
            continue

        sidecar = shots_dir / f"{shot_id}.json"
        if not sidecar.exists():
            failures.append(shot_id)
            print(f"  ! no sidecar produced for {shot_id}", file=sys.stderr)
            continue

        gen = [
            sys.executable, str(HERE / "make_svg.py"), str(sidecar),
            "--out", str(assets),
            "--callouts", str(shots_dir),   # keep the .md helper out of assets
            "--max-width", str(args.max_width),
            "--format", args.format,
            "--page-width", str(args.page_width),
            "--target-font", str(args.target_font),
            "--bg", "transparent"
        ]
        crop = by_id.get(shot_id, {}).get("crop")
        if crop:
            gen += ["--crop", crop]
        layout = by_id.get(shot_id, {}).get("layout")
        if layout:
            gen += ["--layout", layout]
        # A shot can keep the full crop region instead of fitting the crop down
        # to the annotated widget rects (e.g. the timeline, where the empty tail
        # below the tracks is wanted). Defaults to fitting.
        if not by_id.get(shot_id, {}).get("cropFit", True):
            gen += ["--no-crop-fit"]
        if subprocess.run(gen).returncode != 0:
            failures.append(shot_id)

    if failures:
        sys.exit(f"\n{len(failures)} shot(s) failed: {', '.join(failures)}")
    print(f"\nDone: {len(ids)} shot(s) -> {assets}")


if __name__ == "__main__":
    main()
