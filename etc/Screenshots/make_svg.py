#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.
"""Turn a capture sidecar (PNG + JSON) into an annotated SVG.

Annotations are laid out as two columns of captions, one in each side gutter,
with a leader line from each caption to the widget it describes. Each widget is
assigned to the nearer gutter; widgets that span (nearly) the full width are
balanced between the columns. Within a column, captions are sorted by target
height and stacked without overlap, so leader lines never cross.

Gutter labels are kept short: the caption text up to an em dash is used as the
label, and the full caption is written to the companion .callouts.md list. The
figure is sized so it renders close to 1:1 in a typical docs content column,
which keeps the caption text near body-text size instead of being shrunk down.

Usage:
    make_svg.py SIDECAR.json [--out DIR] [--callouts DIR]
                [--max-width N] [--format png|webp|jpeg]
                [--gutter N] [--font N] [--no-outline]
"""

import argparse
import base64
import io
import json
import pathlib
import sys
from xml.sax.saxutils import escape

from PIL import Image, ImageFont

ACCENT = "#c85a3c"   # DJV UI accent, matches the hand-made callouts
BG = "#1e1e1e"       # figure background (gutters); matches the dark UI
TEXT = "#e8e8e8"

FORMATS = {
    "png":  ("image/png",  dict(format="PNG", optimize=True),         "RGBA"),
    "webp": ("image/webp", dict(format="WEBP", quality=85, method=6), "RGBA"),
    "jpeg": ("image/jpeg", dict(format="JPEG", quality=92),           "RGB"),
}

# Layout constants (in user units).
PAD = 22          # canvas padding top/bottom for the label stacks
PAD_V = 7         # vertical padding inside a label
GAP = 18          # gap between stacked labels
EDGE_GAP = 10     # gap between a gutter's leader end and the image edge
TEXT_GAP = 10     # gap between the leader end and the caption text
OUTER_PAD = 16    # gap between the caption and the outer canvas edge
DOT = 7.0         # leader attach-dot radius
LEADER_W = 3.2    # leader line width
OUTLINE_W = 2.5   # widget-rect outline width (was thin next to the leader)
INSET = 0         # leader/dot land on the widget edge (0 = centred on the line)
CHIP_PAD_Y = 4    # vertical padding of a label chip (transparent bg)
CHAR_W = 0.55     # rough average glyph width as a fraction of font size
TEXT_SCALE = 0.87 # label text size relative to the pill (smaller = more padding)
CROP_FIT_PAD = 16 # px kept below the lowest content widget when fitting a crop
PILL_DROP = 0.85  # vertical pill offset (x pill height) so leaders stay diagonal
H_ASPECT = 2.5    # crop wider than this (w/h) auto-uses the horizontal layout
H_STANDOFF = 1.0  # horizontal-mode leader gap from the image, x pill height


def load_sidecar(path):
    data = json.loads(path.read_text())
    png = (path.parent / data["image"]).resolve()
    if not png.exists():
        sys.exit(f"error: image not found: {png}")
    boxes = {}
    for w in data.get("widgets", []):
        boxes.setdefault(w["id"], []).append(w["box"])
    annotate = []
    # A missing "annotate" key auto-labels every tagged widget by id (a debug
    # convenience); an explicit empty list means "no labels".
    src = data.get("annotate")
    if src is None:
        src = [{"id": w["id"]} for w in data.get("widgets", [])]
    for entry in src:
        if isinstance(entry, str):
            annotate.append({"id": entry, "caption": entry})
        else:
            annotate.append({"id": entry["id"],
                             "caption": entry.get("caption", entry["id"])})
    # Device-pixel ratio: the capture's display scale. The PNG and the box
    # coordinates are in device pixels; dpr lets us recover the logical (CSS)
    # size so a 2x capture renders crisp at the same on-page size as a 1x one.
    dpr = data.get("dpr", 1) or 1
    return (data["shot"], png, boxes, annotate, data.get("crop"), dpr,
            data.get("layout"))


def encode_image(png, max_width, fmt, crop=None):
    """Downscale to max_width and re-encode. Optionally crop first (in original
    pixel space). Returns (bytes, mime, (w,h), scale, (off_x, off_y))."""
    im = Image.open(png)
    off_x, off_y = 0, 0
    if crop:
        cx, cy, cw, ch = crop
        iw, ih = im.size
        x0, y0 = max(0, round(cx)), max(0, round(cy))
        x1, y1 = min(iw, round(cx + cw)), min(ih, round(cy + ch))
        if x1 > x0 and y1 > y0:
            im = im.crop((x0, y0, x1, y1))
            off_x, off_y = x0, y0
    iw, ih = im.size
    scale = 1.0
    if max_width and iw > max_width:
        scale = max_width / iw
        im = im.resize((round(iw * scale), round(ih * scale)), Image.LANCZOS)
    mime, save_kw, mode = FORMATS[fmt]
    buf = io.BytesIO()
    try:
        im.convert(mode).save(buf, **save_kw)
    except Exception as e:  # e.g. Pillow built without libwebp
        sys.exit(f"error: could not encode {fmt.upper()} "
                 f"(is Pillow built with support for it?): {e}")
    return buf.getvalue(), mime, im.size, scale, (off_x, off_y)


_FONT_CACHE = {}
_FONT_CANDIDATES = (
    "Arial.ttf", "LiberationSans-Regular.ttf", "DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/Library/Fonts/Arial.ttf",
    "/System/Library/Fonts/Supplemental/Arial.ttf",
)


def text_width(s, px):
    """Advance width of s at px using an Arial-metrics font when available, so
    chips fit the browser's Arial; falls back to a rough estimate otherwise."""
    if px not in _FONT_CACHE:
        font = None
        for cand in _FONT_CANDIDATES:
            try:
                font = ImageFont.truetype(cand, px)
                break
            except Exception:
                continue
        _FONT_CACHE[px] = font
    font = _FONT_CACHE[px]
    if font is not None:
        try:
            return font.getlength(s)
        except Exception:
            pass
    return len(s) * px * CHAR_W


def assign_columns(items, w):
    """Split into (left, right). Nearer side wins; full-width strips (both
    edges near the border) are balanced to keep the columns even."""
    left, right, ambiguous = [], [], []
    for it in items:
        x, _, bw, _ = it["box"]
        ld, rd = x, w - (x + bw)
        if ld < 0.10 * w and rd < 0.10 * w:
            ambiguous.append(it)
        elif ld <= rd:
            left.append(it)
        else:
            right.append(it)
    for it in sorted(ambiguous, key=lambda it: it["box"][1]):
        (left if len(left) <= len(right) else right).append(it)
    return left, right


def layout_column(entries, top, bottom):
    """Assign each entry a non-overlapping vertical position near its target."""
    entries.sort(key=lambda e: e["desired"])
    for e in entries:
        e["top"] = e["desired"] - e["h"] / 2
    if entries and entries[0]["top"] < top:
        entries[0]["top"] = top
    for i in range(1, len(entries)):
        floor = entries[i - 1]["top"] + entries[i - 1]["h"] + GAP
        entries[i]["top"] = max(entries[i]["top"], floor)
    if entries:
        over = (entries[-1]["top"] + entries[-1]["h"]) - bottom
        if over > 0:
            for e in entries:
                e["top"] = max(top, e["top"] - over)
            for i in range(1, len(entries)):
                floor = entries[i - 1]["top"] + entries[i - 1]["h"] + GAP
                entries[i]["top"] = max(entries[i]["top"], floor)
    for e in entries:
        e["cy"] = e["top"] + e["h"] / 2


def _pack_band(band):
    """Place x-sorted pills at their ideal x (centred on the target), dropping
    to a new row only when that position collides in every existing row. Keeps
    leaders near-vertical and non-crossing. Returns the row count."""
    rows = []  # rows[r] = right edge of the last pill placed in row r
    for e in band:
        ideal = e["tcx"] - e["pill_w"] / 2
        r = next((i for i, right in enumerate(rows) if ideal >= right + GAP), None)
        if r is None:
            r = len(rows)
            rows.append(0.0)
        e["chip_x"] = ideal
        e["row"] = r
        rows[r] = ideal + e["pill_w"]
    return len(rows)


def layout_horizontal(entries, iw, ih, chip_h):
    """Above/below layout for a wide, short crop (e.g. a toolbar): split the
    labels into a top and a bottom band, pack each horizontally into rows, and
    drop a near-vertical leader to each target's top/bottom edge. Sets per-entry
    geometry and returns (ox, image_y, cw, ch)."""
    for e in entries:
        bx, _, bw, _ = e["box"]
        e["tcx"] = bx + bw / 2
    entries.sort(key=lambda e: e["tcx"])
    # Assign each target to a band by vertical position: clearly-upper targets
    # go above, clearly-lower go below, and mid-height targets (a toolbar that
    # fills its crop) alternate by x so the two bands stay balanced and uncrowded.
    third = ih / 3
    top, bottom, mids = [], [], []
    for e in entries:
        _, by, _, bh = e["box"]
        tcy = by + bh / 2
        (top if tcy < third else bottom if tcy > 2 * third else mids).append(e)
    for i, e in enumerate(mids):
        (top if i % 2 == 0 else bottom).append(e)
    top.sort(key=lambda e: e["tcx"])
    bottom.sort(key=lambda e: e["tcx"])
    top_rows, bot_rows = _pack_band(top), _pack_band(bottom)

    pitch = chip_h + GAP
    # Stand the bands off the image by ~a pill height (not the bare EDGE_GAP,
    # which the column-fit would shrink to a few px), so leaders have length.
    standoff = max(EDGE_GAP, round(H_STANDOFF * chip_h))
    top_h = (top_rows * pitch - GAP + standoff) if top else 0
    bot_h = (bot_rows * pitch - GAP + standoff) if bottom else 0
    image_y = top_h

    min_x = min((e["chip_x"] for e in entries), default=0)
    max_x = max((e["chip_x"] + e["pill_w"] for e in entries), default=iw)
    ox = max(0.0, -min_x)                     # pad for pills overhanging the left
    cw = round(ox + max(iw, max_x))
    ch = round(top_h + ih + bot_h)

    for e in top:
        e["chip_x"] += ox
        e["cy"] = image_y - standoff - chip_h / 2 - e["row"] * pitch
        e["lead_start"] = (e["chip_x"] + e["pill_w"] / 2, e["cy"] + chip_h / 2)
        e["anchors"] = [(ox + tx + tbw / 2, image_y + ty)
                        for (tx, ty, tbw, tbh) in e["targets"]]
    for e in bottom:
        e["chip_x"] += ox
        e["cy"] = image_y + ih + standoff + chip_h / 2 + e["row"] * pitch
        e["lead_start"] = (e["chip_x"] + e["pill_w"] / 2, e["cy"] - chip_h / 2)
        e["anchors"] = [(ox + tx + tbw / 2, image_y + ty + tbh)
                        for (tx, ty, tbw, tbh) in e["targets"]]
    return ox, image_y, cw, ch


def build_svg(img_bytes, mime, iw, ih, boxes, annotate, scale, gutter, font,
              target_font, page_width, outline, bg, dpr=1, layout=None):
    entries, missing = [], []
    for item in annotate:
        blist = boxes.get(item["id"])
        if not blist:
            missing.append(item["id"])
            continue
        targets = [tuple(v * scale for v in b) for b in blist]
        x0 = min(t[0] for t in targets)
        y0 = min(t[1] for t in targets)
        x1 = max(t[0] + t[2] for t in targets)
        y1 = max(t[1] + t[3] for t in targets)
        # "box" is the bounding box of all targets (drives column + layout); the
        # individual targets each get their own leader and dot.
        entries.append({"box": (x0, y0, x1 - x0, y1 - y0), "targets": targets,
                        "label": item["caption"], "desired": (y0 + y1) / 2})

    # Pick the layout. Horizontal (above/below) suits a wide, short crop like a
    # toolbar -- the column model would splay long crossing diagonals there;
    # columns suit tall subjects. "auto" decides from the image aspect.
    horizontal = (layout == "horizontal") or (
        layout in (None, "auto") and iw / max(1, ih) >= H_ASPECT)
    left, right = ([], []) if horizontal else assign_columns(entries, iw)

    def gutters_for(f):
        """Left/right gutter widths for a candidate font (px)."""
        if gutter:
            return gutter, gutter
        tf = max(8, round(f * TEXT_SCALE))
        c = (f + 2 * CHIP_PAD_Y) / 2
        def side(col):
            widest = max((text_width(e["label"], tf) + 2 * c for e in col),
                         default=0)
            return int(widest + EDGE_GAP + OUTER_PAD)
        lg, rg = (side(left) if left else 0), (side(right) if right else 0)
        if left and right:
            lg = rg = max(lg, rg)
        return lg, rg

    def fig_width_for(f):
        """Figure width at a candidate internal font, used to fit the column.
        Horizontal mode has no side gutters, but edge pills can overhang."""
        if horizontal:
            tf = max(8, round(f * TEXT_SCALE))
            c = (f + 2 * CHIP_PAD_Y) / 2
            widest = max((text_width(e["label"], tf) + 2 * c for e in entries),
                         default=0)
            return iw + widest + 2 * OUTER_PAD
        lg, rg = gutters_for(f)
        return iw + lg + rg

    # Hold the on-page font at target_font and fit THIS shot to the page column
    # (never upscaled); dpr keeps a 2x capture the same on-page size from twice
    # the pixels, since the figure is authored large and squashed by the width
    # attribute. Iterate: the gutter/overhang depends on the internal font, which
    # depends on the fit. An explicit --font keeps a fixed internal size (native).
    if font is None:
        sd = scale * dpr
        M = 1.0
        for _ in range(5):
            f_int = max(8.0, target_font * sd / M)
            M = min(1.0, page_width * sd / fig_width_for(f_int))
        display_scale = M / sd                     # embedded px -> on-page px
        font = max(8, int(round(target_font / display_scale)))
    else:
        display_scale = 1.0 / (scale * dpr)

    text_font = max(8, round(font * TEXT_SCALE))   # text smaller than the pill
    chip_h = font + 2 * CHIP_PAD_Y                 # pill height (set by font)
    cap = chip_h / 2                               # pill end radius
    for e in entries:
        e["pill_w"] = text_width(e["label"], text_font) + 2 * cap
        e["h"] = chip_h

    if horizontal:
        ox, image_y, cw, ch = layout_horizontal(entries, iw, ih, chip_h)
    else:
        # Offset each pill off its widget's row so the leader is always a
        # diagonal; nudge toward the figure's vertical centre so pills stay
        # within the image and don't grow it.
        drop = PILL_DROP * chip_h
        mid = ih / 2
        for e in entries:
            cyb = e["box"][1] + e["box"][3] / 2
            e["desired"] = cyb + (drop if cyb <= mid else -drop)
        for col in (left, right):
            layout_column(col, PAD, ih - PAD)
        lg, rg = gutters_for(font)
        ox, image_y = lg, 0
        extent = max([e["top"] + e["h"] for e in entries] + [ih]) if entries else ih
        cw, ch = lg + iw + rg, max(ih, round(extent + PAD))
        transparent = (bg == "transparent")
        for side, col in (("L", left), ("R", right)):
            for e in col:
                if side == "L":
                    e["edge_x"] = lg - EDGE_GAP
                    e["chip_x"] = e["edge_x"] - e["pill_w"]
                    cap_cx = e["chip_x"] + e["pill_w"] - cap   # inner cap centre
                    e["anchors"] = [(ox + tx + INSET, ty + tbh / 2)
                                    for (tx, ty, tbw, tbh) in e["targets"]]
                else:
                    e["edge_x"] = lg + iw + EDGE_GAP
                    e["chip_x"] = e["edge_x"]
                    cap_cx = e["chip_x"] + cap
                    e["anchors"] = [(ox + tx + tbw - INSET, ty + tbh / 2)
                                    for (tx, ty, tbw, tbh) in e["targets"]]
                # leader exits the pill's end-cap (transparent) or image edge.
                e["lead_start"] = ((cap_cx if transparent else e["edge_x"]),
                                   e["cy"])

    b64 = base64.b64encode(img_bytes).decode("ascii")
    p = [
        f'<svg xmlns="http://www.w3.org/2000/svg" '
        f'xmlns:xlink="http://www.w3.org/1999/xlink" '
        f'viewBox="0 0 {cw} {ch}" width="{cw * display_scale:.1f}" '
        f'height="{ch * display_scale:.1f}">',
        (f'<rect x="0" y="0" width="{cw}" height="{ch}" fill="{BG}"/>'
         if bg == "dark" else ""),
        f'<image x="{ox}" y="{image_y}" width="{iw}" height="{ih}" '
        f'xlink:href="data:{mime};base64,{b64}"/>',
    ]

    # Outlines + leaders. Each pill emits a leader from its near edge to every
    # target that shares its tag (one pill may fan out to several widgets).
    p.append(f'<g stroke="{ACCENT}" fill="none">')
    for e in entries:
        if outline:
            for (tx, ty, tbw, tbh) in e["targets"]:
                p.append(f'<rect x="{ox + tx:.0f}" y="{image_y + ty:.0f}" '
                         f'width="{tbw:.0f}" height="{tbh:.0f}" rx="2" '
                         f'stroke-opacity="0.45" stroke-width="{OUTLINE_W}"/>')
        sx, sy = e["lead_start"]
        for (ax, ay) in e["anchors"]:
            p.append(f'<path d="M {sx:.1f} {sy:.1f} '
                     f'L {ax:.1f} {ay:.1f}" stroke-width="{LEADER_W}"/>')
    p.append("</g>")

    p.append(f'<g fill="{ACCENT}">')
    for e in entries:
        for (ax, ay) in e["anchors"]:
            p.append(f'<circle cx="{ax:.1f}" cy="{ay:.1f}" r="{DOT}"/>')
    p.append("</g>")

    txt_fill = "#ffffff" if bg == "transparent" else TEXT
    p.append(f'<g font-family="Arial, sans-serif" font-size="{text_font}" '
             f'fill="{txt_fill}" text-anchor="middle">')
    for e in entries:
        if bg == "transparent":
            p.append(f'<rect x="{e["chip_x"]:.1f}" y="{e["cy"] - cap:.1f}" '
                     f'width="{e["pill_w"]:.1f}" height="{chip_h:.1f}" '
                     f'rx="{cap:.1f}" ry="{cap:.1f}" fill="{ACCENT}"/>')
        cx = e["chip_x"] + e["pill_w"] / 2
        p.append(f'<text x="{cx:.1f}" y="{e["cy"]:.1f}" '
                 f'dominant-baseline="central">{escape(e["label"])}</text>')
    p.append("</g></svg>")
    return "\n".join(p), missing


def build_markdown(annotate):
    """A bare label list, in figure order, as a scaffold for the page's
    description list. The descriptions themselves are authored in the docs."""
    return "\n".join(f"- {escape(it['caption'])}" for it in annotate)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("sidecar", type=pathlib.Path)
    ap.add_argument("--out", type=pathlib.Path, default=None)
    ap.add_argument("--callouts", type=pathlib.Path, default=None)
    ap.add_argument("--max-width", type=int, default=1280,
                    help="downscale embedded image to this width (0 = full size)")
    ap.add_argument("--format", choices=sorted(FORMATS), default="png")
    ap.add_argument("--gutter", type=int, default=None,
                    help="side gutter width in px (default: auto-fit to labels)")
    ap.add_argument("--font", type=int, default=None,
                    help="caption font size in px (default: auto from "
                         "--target-font and --page-width)")
    ap.add_argument("--target-font", type=int, default=15,
                    help="desired on-page label size in px once the browser "
                         "scales the figure to the content column")
    ap.add_argument("--page-width", type=int, default=800,
                    help="docs content-column width in px (figures wider than "
                         "this are scaled down to fit)")
    ap.add_argument("--crop", default=None,
                    help="tagged widget id to crop the image to (e.g. a tool "
                         "panel); overrides the sidecar's 'crop' field")
    ap.add_argument("--crop-margin", type=int, default=0,
                    help="px of margin to keep around the crop region")
    ap.add_argument("--no-crop-fit", dest="crop_fit", action="store_false",
                    help="keep the full crop height instead of trimming the "
                         "empty tail below the content")
    ap.add_argument("--layout", choices=["auto", "columns", "horizontal"],
                    default="auto", help="callout placement: side columns, "
                    "above/below (for wide toolbars), or auto by aspect")
    ap.add_argument("--no-outline", action="store_true",
                    help="don't outline the widget regions")
    ap.add_argument("--bg", choices=["dark", "transparent"], default="transparent",
                    help="figure background: solid dark panel, or transparent "
                         "with a chip behind each label")
    args = ap.parse_args()

    shot, png, boxes, annotate, crop_id, dpr, shot_layout = load_sidecar(
        args.sidecar)
    crop_id = args.crop or crop_id   # explicit --crop overrides the sidecar
    # An explicit per-shot "layout" wins; else the CLI default ("auto").
    layout = shot_layout or args.layout
    crop = None
    if crop_id:
        if crop_id in boxes:
            cx, cy, cw, ch = boxes[crop_id][0]
            m = args.crop_margin
            left, top = cx - m, cy - m
            right, bottom = cx + cw + m, cy + ch + m
            if args.crop_fit:
                # Trim a panel's empty tail: pull the bottom up to the lowest
                # annotated widget. Docked panels stretch taller than their
                # content, leaving dead space below; this removes it (and grows
                # again automatically when a mode reveals more controls).
                content = [b for a in annotate if a["id"] != crop_id
                           for b in boxes.get(a["id"], [])]
                if content:
                    content_bottom = max(b[1] + b[3] for b in content)
                    bottom = min(bottom, content_bottom + CROP_FIT_PAD)
            crop = (left, top, right - left, bottom - top)
        else:
            print(f"warning: crop id '{crop_id}' has no box; using full image",
                  file=sys.stderr)
    img_bytes, mime, (w, h), scale, (ox, oy) = encode_image(
        png, args.max_width, args.format, crop)
    if crop:
        # Move boxes into the cropped frame and drop any now fully outside it.
        cw_px, ch_px = round(w / scale), round(h / scale)
        moved = {}
        for k, blist in boxes.items():
            kept = []
            for bx, by, bw, bh in blist:
                bx, by = bx - ox, by - oy
                if bx + bw > 0 and bx < cw_px and by + bh > 0 and by < ch_px:
                    kept.append([bx, by, bw, bh])
            if kept:
                moved[k] = kept
        boxes = moved
    svg, missing = build_svg(img_bytes, mime, w, h, boxes, annotate, scale,
                             args.gutter, args.font, args.target_font,
                             args.page_width, not args.no_outline, args.bg, dpr,
                             layout)

    out_dir = args.out or args.sidecar.parent
    out_dir.mkdir(parents=True, exist_ok=True)
    callouts_dir = args.callouts or out_dir
    callouts_dir.mkdir(parents=True, exist_ok=True)
    # The output is named after the shot id, which is already lowercase and
    # hyphenated (URL-friendly): main-window -> main-window.svg, and the
    # companion captions list -> main-window.callouts.md.
    svg_path = out_dir / f"{shot}.svg"
    svg_path.write_text(svg)
    (callouts_dir / f"{shot}.callouts.md").write_text(
        build_markdown(annotate) + "\n")

    for mid in missing:
        print(f"warning: no box for annotated id '{mid}' (hidden or untagged?)",
              file=sys.stderr)
    print(f"wrote {svg_path}  ({len(svg)//1024} KB, {args.format})")


if __name__ == "__main__":
    main()
