# Editable OTIO Playlist Model

`PlaylistModel` provides the core editable OTIO playlist contract for DJV.
It is designed for the existing player and timeline, not as a general OTIO
editor.

## User workflow

- Use **File > New OTIO Playlist** to start from one image or video.
- Use **File > New OTIO Playlist from Folder** to build a playlist from
  recursively discovered media before it is loaded.
- In the **OTIO Playlist** tool, add files or a filtered folder, drag items to
  reorder them, remove items, and save or save as `.otio`.

Folder filters are case-insensitive regular-expression terms. Prefix a term
with `-` to exclude it and target `name:`, `dir:`, or `path:`. Folder imports
are sorted deterministically, do not follow directory symlinks, and stop at
hard depth, entry, and result limits.

## Supported editable subset

- One video track containing clips.
- Optionally, one aligned audio track containing a clip or gap for every video
  item.
- Add, remove, and reorder media while preserving matching audio or silence.
- Save to `.otio`; a scratch playlist requires Save As before its first save.
- Numbered still-image paths are imported as separate one-frame clips rather
  than automatically grouped into an image sequence.

## Read-only timelines

DJV retains complex OTIO timelines for playback and Save As, but never flattens
them to make them editable. Multiple video tracks, nested compositions,
transitions, unaligned audio, and packaged `.otioz` timelines are read-only.
The model exposes the reason so the UI can communicate the editing boundary.

## Verification

`djvPlaylistModelTest` checks item order, aligned audio/gaps, round-trip OTIO
serialization, image-sequence handling, read-only complex timelines, filtered
folder selection, and folder scan limits.
