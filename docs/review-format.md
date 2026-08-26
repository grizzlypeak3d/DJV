---
title: Review file format
layout: default
nav_order: 16
---

# Review file format

A review is a JSON document with the `.djvr` extension. This page describes
version 1 of that format: what a document contains, and what a program reading
one is expected to do. For how reviews are used in DJV, see
[Files]({{ site.baseurl }}/files.html#reviews).

## Version

Every document carries an integer under `djvReview`:

```json
{ "djvReview": 1 }
```

The version is incremented **only** when a document can no longer be read
correctly by an older DJV. Adding a key, or a whole new section, does not
change it: a reader ignores what it does not recognize, so those changes are
already safe.

A reader that finds a version higher than the one it knows **refuses the
document** and says so. It does not read what it can and leave the rest: the
next save would write that loss back over the author's own file. DJV reports
this in the **Messages** tool and leaves the current session untouched.

A document with no `djvReview` key is read as version 1.

## What a reader must do

These four rules are what make a review safe to pass between people running
different versions of DJV.

**Keep an unknown section.** A top-level section the reader does not know is
carried through a load/save cycle untouched. Opening a review in an older DJV
and saving it does not strip what a newer one wrote.

**Read each section on its own.** A section that cannot be read costs that
section and nothing else. In particular it must never cost the `annotations`
and `notes`, which exist nowhere but in this file — the rest of a review is a
convenience that can be rebuilt by hand.

**Do not overwrite what you could not read.** A section that failed to load is
written back exactly as it was found, rather than replaced by the defaults the
application fell back to.

**Skip, but keep, what you cannot place.** An annotation or a stroke in a
coordinate space the reader does not know is not drawn — and not deleted
either. It is written back as it was found, after the items that were
understood.

## The document

| Key | Contents |
|---|---|
| `djvReview` | Format version, integer. |
| `app` | The application and version that wrote the document, informational. |
| `created` | When it was written, ISO 8601 UTC. |
| `files` | The open sources, in tab order. |
| `compare` | The A/B setup: `aId`, `bIds`, comparison options and time mode. |
| `view` | Viewport framing, pan and zoom. |
| `color` | OCIO, LUT, display, background, foreground, aspect ratio and HUD. |
| `ui` | The active tool panel and the window layout. |
| `annotations` | Drawings, by source and frame. |
| `notes` | Timestamped comments. |
| `ranges` | Named frame ranges. |

`compare`, `view`, `color` and `ui` delegate to serializers shared with the
application settings, and those require every key they know. They are therefore
the sections most likely to fail against a document written by a different
version — which is why the rules above exist.

### Files

```json
{
  "id": "ca708ac647f6600300000000",
  "path": "shots/sh010/sh010_comp_v004.mov",
  "pathAbsolute": "P:/show/shots/sh010/sh010_comp_v004.mov",
  "audioPath": "shots/sh010/sh010_mix.wav",
  "videoLayer": 0,
  "speed": 24.0,
  "currentTime": { "value": 168.0, "rate": 24.0 },
  "inOutRange": {
    "start":    { "value": 0.0,   "rate": 24.0 },
    "duration": { "value": 476.0, "rate": 24.0 }
  }
}
```

`id` is what the rest of the document refers to a source by — `compare.aId`,
`compare.bIds`, `annotations[].sourceId`. Indices are not used anywhere,
because they do not survive closing or reordering a file.

Paths are stored twice. `path` is relative to the `.djvr` and is tried first,
so a review stays valid when it travels with its media; `pathAbsolute` is the
fallback for media that lives outside the review's folder. Separate audio uses
the same pair, `audioPath` and `audioPathAbsolute`. **All four are stored with
forward slashes**, on every platform, so a document moves between them
unchanged.

When neither form resolves, DJV reports the missing sources and offers to look
for them in a folder you choose.

### Annotations

```json
{
  "id": "0eecb7e26602f91b00000000",
  "sourceId": "ca708ac647f6600300000000",
  "space": "image",
  "time": { "value": 168.0, "rate": 24.0 },
  "author": "matthieu",
  "created": "2026-08-26T09:01:00Z",
  "strokes": [
    {
      "color": [1.0, 0.365, 0.02, 1.0],
      "width": 4.0,
      "widthSpace": "image",
      "points": [120.5, 340.0, 121.0, 341.5]
    }
  ]
}
```

An annotation belongs to one source and is visible on one frame only.

`space` and `widthSpace` are both `"image"` in version 1: points and widths are
expressed in the pixels of the source image, so a drawing keeps its position
and its weight whatever the zoom, the pan or the comparison mode. A reader that
meets any other value must not draw the item — the coordinates would land
somewhere else entirely — and must keep it, per the rules above. An absent
space means `"image"`.

`points` is a flat `[x, y, x, y, ...]` array, which keeps long strokes compact.
A trailing odd value is ignored.

### Notes and ranges

```json
{
  "id": "b1c2d3e4f5a6b7c800000001",
  "time": { "value": 96.0, "rate": 24.0 },
  "created": "2026-08-26T09:00:00Z",
  "author": "matthieu",
  "text": "Grade is too cold from here."
}
```

A note belongs to the review rather than to a source: the frame locates it. A
range is an `id`, a free-text `name` and a `range`; selecting one sets the
timeline in and out points.

### Attribution

`author` is optional on both notes and annotations, and is empty when it is not
known. DJV has no accounts: it fills the field from the environment —
`USERNAME` on Windows, `USER` elsewhere — when the item is created. Treat it as
a label saying who wrote something when a session comes back from someone else,
not as an identity, and never as a permission.

Identifiers are 64 random bits plus a counter, so items written independently
by two people do not collide.

## What is not stored

Keyboard shortcuts, style, cache size and the audio device are deliberately
absent: opening a review someone sends you must not reconfigure your
installation.
