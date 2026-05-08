#!/usr/bin/env python3
"""Generate and decode small media compatibility samples with FFmpeg."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


VIDEO_SOURCE = "testsrc2=size={size}:rate={rate}"
AUDIO_SOURCE = "sine=frequency=440:sample_rate=48000"


@dataclass(frozen=True)
class Case:
    name: str
    extension: str
    encoders: tuple[str, ...]
    streams: tuple[tuple[str, str], ...]
    args: tuple[str, ...]


CASES: tuple[Case, ...] = (
    Case(
        "mp4_h264_aac",
        ".mp4",
        ("libx264", "aac"),
        (("video", "h264"), ("audio", "aac")),
        (
            "-f",
            "lavfi",
            "-i",
            VIDEO_SOURCE.format(size="640x360", rate="24"),
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:v",
            "libx264",
            "-pix_fmt",
            "yuv420p",
            "-c:a",
            "aac",
            "-shortest",
        ),
    ),
    Case(
        "mov_h264_aac",
        ".mov",
        ("libx264", "aac"),
        (("video", "h264"), ("audio", "aac")),
        (
            "-f",
            "lavfi",
            "-i",
            VIDEO_SOURCE.format(size="640x360", rate="24"),
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:v",
            "libx264",
            "-pix_fmt",
            "yuv420p",
            "-c:a",
            "aac",
            "-shortest",
        ),
    ),
    Case(
        "m4v_h264_aac",
        ".m4v",
        ("libx264", "aac"),
        (("video", "h264"), ("audio", "aac")),
        (
            "-f",
            "lavfi",
            "-i",
            VIDEO_SOURCE.format(size="640x360", rate="24"),
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:v",
            "libx264",
            "-pix_fmt",
            "yuv420p",
            "-c:a",
            "aac",
            "-shortest",
        ),
    ),
    Case(
        "mov_prores_pcm",
        ".mov",
        ("prores_ks", "pcm_s16le"),
        (("video", "prores"), ("audio", "pcm_s16le")),
        (
            "-f",
            "lavfi",
            "-i",
            VIDEO_SOURCE.format(size="640x360", rate="24"),
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:v",
            "prores_ks",
            "-profile:v",
            "3",
            "-pix_fmt",
            "yuv422p10le",
            "-c:a",
            "pcm_s16le",
            "-shortest",
        ),
    ),
    Case(
        "mxf_dnxhr_pcm",
        ".mxf",
        ("dnxhd", "pcm_s16le"),
        (("video", "dnxhd"), ("audio", "pcm_s16le")),
        (
            "-f",
            "lavfi",
            "-i",
            VIDEO_SOURCE.format(size="1280x720", rate="24"),
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:v",
            "dnxhd",
            "-profile:v",
            "dnxhr_lb",
            "-pix_fmt",
            "yuv422p",
            "-c:a",
            "pcm_s16le",
            "-shortest",
        ),
    ),
    Case(
        "webm_vp9_opus",
        ".webm",
        ("libvpx-vp9", "libopus"),
        (("video", "vp9"), ("audio", "opus")),
        (
            "-f",
            "lavfi",
            "-i",
            VIDEO_SOURCE.format(size="640x360", rate="24"),
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:v",
            "libvpx-vp9",
            "-b:v",
            "0",
            "-crf",
            "40",
            "-c:a",
            "libopus",
            "-shortest",
        ),
    ),
    Case(
        "mkv_h264_aac",
        ".mkv",
        ("libx264", "aac"),
        (("video", "h264"), ("audio", "aac")),
        (
            "-f",
            "lavfi",
            "-i",
            VIDEO_SOURCE.format(size="640x360", rate="24"),
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:v",
            "libx264",
            "-pix_fmt",
            "yuv420p",
            "-c:a",
            "aac",
            "-shortest",
        ),
    ),
    Case(
        "mp4_hevc_aac",
        ".mp4",
        ("libx265", "aac"),
        (("video", "hevc"), ("audio", "aac")),
        (
            "-f",
            "lavfi",
            "-i",
            VIDEO_SOURCE.format(size="640x360", rate="24"),
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:v",
            "libx265",
            "-x265-params",
            "log-level=error",
            "-pix_fmt",
            "yuv420p",
            "-c:a",
            "aac",
            "-shortest",
        ),
    ),
    Case(
        "mkv_av1_opus",
        ".mkv",
        ("libaom-av1", "libopus"),
        (("video", "av1"), ("audio", "opus")),
        (
            "-f",
            "lavfi",
            "-i",
            VIDEO_SOURCE.format(size="320x180", rate="12"),
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.2",
            "-c:v",
            "libaom-av1",
            "-cpu-used",
            "8",
            "-crf",
            "45",
            "-b:v",
            "0",
            "-c:a",
            "libopus",
            "-shortest",
        ),
    ),
    Case(
        "m4a_aac",
        ".m4a",
        ("aac",),
        (("audio", "aac"),),
        (
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:a",
            "aac",
        ),
    ),
    Case(
        "aac_adts",
        ".aac",
        ("aac",),
        (("audio", "aac"),),
        (
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:a",
            "aac",
            "-f",
            "adts",
        ),
    ),
    Case(
        "flac_audio",
        ".flac",
        ("flac",),
        (("audio", "flac"),),
        (
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:a",
            "flac",
        ),
    ),
    Case(
        "opus_audio",
        ".opus",
        ("libopus",),
        (("audio", "opus"),),
        (
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:a",
            "libopus",
        ),
    ),
    Case(
        "mp3_audio",
        ".mp3",
        ("libmp3lame",),
        (("audio", "mp3"),),
        (
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:a",
            "libmp3lame",
        ),
    ),
    Case(
        "wav_pcm",
        ".wav",
        ("pcm_s16le",),
        (("audio", "pcm_s16le"),),
        (
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:a",
            "pcm_s16le",
        ),
    ),
    Case(
        "aiff_pcm",
        ".aiff",
        ("pcm_s16be",),
        (("audio", "pcm_s16be"),),
        (
            "-f",
            "lavfi",
            "-i",
            AUDIO_SOURCE,
            "-t",
            "0.5",
            "-c:a",
            "pcm_s16be",
        ),
    ),
)


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )


def get_encoders(ffmpeg: str) -> set[str]:
    result = run([ffmpeg, "-hide_banner", "-v", "quiet", "-encoders"])
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "Unable to list FFmpeg encoders")

    encoders: set[str] = set()
    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 2 and parts[0][0] in {"V", "A", "S"}:
            encoders.add(parts[1])
    return encoders


def probe_streams(ffprobe: str, path: Path) -> list[dict[str, object]]:
    result = run(
        [
            ffprobe,
            "-v",
            "error",
            "-show_entries",
            "stream=codec_type,codec_name,width,height,channels,sample_rate",
            "-of",
            "json",
            str(path),
        ]
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "ffprobe failed")
    return json.loads(result.stdout).get("streams", [])


def validate_streams(case: Case, streams: list[dict[str, object]]) -> None:
    for stream_type, codec_name in case.streams:
        matches = [
            stream
            for stream in streams
            if stream.get("codec_type") == stream_type
            and stream.get("codec_name") == codec_name
        ]
        if not matches:
            found = ", ".join(
                f"{stream.get('codec_type')}:{stream.get('codec_name')}"
                for stream in streams
            )
            raise RuntimeError(
                f"Missing expected {stream_type}:{codec_name}; found {found or 'none'}"
            )


def decode_stream(ffmpeg: str, path: Path, stream_type: str) -> None:
    if stream_type == "video":
        command = [
            ffmpeg,
            "-hide_banner",
            "-nostdin",
            "-v",
            "error",
            "-i",
            str(path),
            "-map",
            "0:v:0",
            "-frames:v",
            "1",
            "-f",
            "rawvideo",
            "-pix_fmt",
            "rgb24",
            os.devnull,
        ]
    else:
        command = [
            ffmpeg,
            "-hide_banner",
            "-nostdin",
            "-v",
            "error",
            "-i",
            str(path),
            "-map",
            "0:a:0",
            "-t",
            "0.1",
            "-f",
            "s16le",
            os.devnull,
        ]

    result = run(command)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or f"{stream_type} decode failed")


def unique_stream_types(case: Case) -> Iterable[str]:
    seen: set[str] = set()
    for stream_type, _ in case.streams:
        if stream_type not in seen:
            seen.add(stream_type)
            yield stream_type


def run_case(ffmpeg: str, ffprobe: str, case: Case, output_dir: Path, encoders: set[str]) -> dict[str, object]:
    missing = [encoder for encoder in case.encoders if encoder not in encoders]
    if missing:
        return {
            "name": case.name,
            "status": "skipped",
            "reason": f"missing encoder(s): {', '.join(missing)}",
        }

    output = output_dir / f"{case.name}{case.extension}"
    command = [
        ffmpeg,
        "-hide_banner",
        "-nostdin",
        "-y",
        "-v",
        "error",
        *case.args,
        str(output),
    ]
    result = run(command)
    if result.returncode != 0:
        return {
            "name": case.name,
            "status": "failed",
            "path": str(output),
            "stage": "generate",
            "reason": result.stderr.strip() or "ffmpeg generation failed",
        }

    try:
        streams = probe_streams(ffprobe, output)
        validate_streams(case, streams)
        for stream_type in unique_stream_types(case):
            decode_stream(ffmpeg, output, stream_type)
    except Exception as exc:  # noqa: BLE001 - report the exact smoke-test failure.
        return {
            "name": case.name,
            "status": "failed",
            "path": str(output),
            "stage": "decode",
            "reason": str(exc),
        }

    return {
        "name": case.name,
        "status": "passed",
        "path": str(output),
        "streams": [
            {
                "type": stream.get("codec_type"),
                "codec": stream.get("codec_name"),
                "width": stream.get("width"),
                "height": stream.get("height"),
                "sample_rate": stream.get("sample_rate"),
                "channels": stream.get("channels"),
            }
            for stream in streams
        ],
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ffmpeg", default=os.environ.get("FFMPEG", "ffmpeg"))
    parser.add_argument("--ffprobe", default=os.environ.get("FFPROBE", "ffprobe"))
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--json", type=Path, help="Write a JSON result manifest")
    parser.add_argument("--case", action="append", choices=[case.name for case in CASES])
    parser.add_argument("--list", action="store_true", help="List available cases and exit")
    parser.add_argument("--keep", action="store_true", help="Keep a temporary output directory")
    parser.add_argument(
        "--strict-skips",
        action="store_true",
        help="Return a failure if any case is skipped because an encoder is missing",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.list:
        for case in CASES:
            print(case.name)
        return 0

    ffmpeg = shutil.which(args.ffmpeg) or args.ffmpeg
    ffprobe = shutil.which(args.ffprobe) or args.ffprobe
    selected = [case for case in CASES if not args.case or case.name in args.case]

    temp_dir: tempfile.TemporaryDirectory[str] | None = None
    if args.output_dir:
        output_dir = args.output_dir
        output_dir.mkdir(parents=True, exist_ok=True)
    else:
        temp_dir = tempfile.TemporaryDirectory(prefix="djv-media-compat-")
        output_dir = Path(temp_dir.name)

    try:
        encoders = get_encoders(ffmpeg)
        results = [run_case(ffmpeg, ffprobe, case, output_dir, encoders) for case in selected]
    finally:
        if temp_dir and not args.keep:
            temp_dir.cleanup()

    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")

    for item in results:
        line = f"{item['status'].upper():7} {item['name']}"
        if item["status"] == "failed":
            line += f" ({item.get('stage')}): {item.get('reason')}"
        elif item["status"] == "skipped":
            line += f": {item.get('reason')}"
        print(line)

    failed = [item for item in results if item["status"] == "failed"]
    skipped = [item for item in results if item["status"] == "skipped"]
    if failed or (args.strict_skips and skipped):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
