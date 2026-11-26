#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""Write a moving bar image sequence, for judging playback by eye.

Usage: etc/SampleData/make-bar.py OUTPUT_DIR [-size WxH] [-depth 8|16]
                                  [-frames N]

A white bar moves across the picture at a constant speed, so a frame shown
late or twice makes it jump. The background alternates dark gray and dark
blue every frame: at 60 fps the two blend into one color to the eye, and a
frame that misses its refresh shows as a flash, while a picture made from
two frames (tearing) shows a seam where the color changes. Yellow blocks
along the top give the frame number in binary, lowest bit at the left, for
reading a photo of the screen.

The defaults are the hard case, 4K 16-bit, about 50 MB a frame to upload:

    etc/SampleData/make-bar.py bar-4k
    etc/SampleData/make-bar.py bar-1080 -size 1920x1080 -depth 8

Then play bar.0000.png at 24 fps and at 60 fps, with each renderer.

The PNG files are written here rather than through tlRender or feather-tk,
so that the test media does not depend on the code being tested. Only the
Python standard library is used.
"""

import argparse
import os
import struct
import sys
import zlib


def writePNG(path, width, height, depth, rows):
    raw = b"".join(b"\x00" + row for row in rows)

    def chunk(kind, data):
        crc = zlib.crc32(kind + data) & 0xffffffff
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", crc)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, depth, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 1)))
        f.write(chunk(b"IEND", b""))


def main():
    parser = argparse.ArgumentParser(
        description="Write a moving bar image sequence, for judging playback by eye.")
    parser.add_argument("output", help="the directory to write bar.NNNN.png into")
    parser.add_argument("-size", default="3840x2160", help="WxH (default 3840x2160)")
    parser.add_argument("-depth", type=int, choices=(8, 16), default=16,
                        help="bits per channel (default 16)")
    parser.add_argument("-frames", type=int, default=48, help="frame count (default 48)")
    args = parser.parse_args()

    try:
        width, height = (int(i) for i in args.size.lower().split("x"))
    except ValueError:
        sys.exit("make-bar.py: -size must be WxH, e.g. 1920x1080")
    if width < 320 or height < 180:
        sys.exit("make-bar.py: -size must be at least 320x180")
    if args.frames < 1:
        sys.exit("make-bar.py: -frames must be at least 1")

    # Colors as 16-bit values; 8-bit is the same color.
    def pixel(r, g, b):
        if args.depth == 16:
            return struct.pack(">HHH", r, g, b)
        return bytes(round(i / 257) for i in (r, g, b))

    white = pixel(60000, 60000, 60000)
    yellow = pixel(60000, 60000, 0)
    backgrounds = (pixel(9000, 9000, 9000), pixel(4000, 6000, 16000))

    # Sizes in proportion to the width: at 4K, a 96 pixel bar moving 80
    # pixels a frame, across the picture in two seconds at 24 fps.
    bar = width // 40
    step = width // 48
    block = width // 24
    codeHeight = height // 18
    bits = min(8, width // block)

    os.makedirs(args.output, exist_ok=True)
    for frame in range(args.frames):
        background = backgrounds[frame % 2]
        x = (frame * step) % (width - bar)
        row = background * x + white * bar + background * (width - bar - x)
        code = b""
        for bit in range(bits):
            code += (yellow if (frame >> bit) & 1 else background) * block
        code += background * (width - bits * block)
        rows = [code] * codeHeight + [row] * (height - codeHeight)
        writePNG(os.path.join(args.output, "bar.%04d.png" % frame),
                 width, height, args.depth, rows)
    print("Wrote %d frames to %s" % (args.frames, args.output))


if __name__ == "__main__":
    main()
