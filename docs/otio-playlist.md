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

Folder filters are compiled case-insensitive regular-expression terms. Prefix
a term with `-` to exclude it and target `name:`, `ext:`, `dir:`, or `path:`.
Folder imports are sorted deterministically, stay confined to the selected
root, skip directory links by default, and fail atomically at hard traversal
or result limits rather than publishing a partial playlist. Scanning runs on
an owned cancellable worker so a slow folder does not block the UI.

## Command line and Windows batch

Create the playlist on demand while DJV starts:

```console
djv -playlistFolder . -playlistFilter "ext:^(mov|mp4)$ -dir:(cache|proxy)"
```

The generated playlist opens in DJV immediately. Add
`-playlistOutput review.otio` to save it, `-playlistTopLevel` to scan only the
folder root, or `-playlistFrames` to keep numbered image frames separate.
Normal playback options can be combined with the playlist options:

```console
djv -playlistFolder . -playlistFilter "ext:^mov$" -playback Forward -loop Loop
```

On Windows, `etc\Windows\djv-playlist-folder.bat` uses the current directory as
the playlist root and forwards additional options:

```bat
set "DJV_EXE=C:\path\to\djv.exe"
C:\path\to\DJV\etc\Windows\djv-playlist-folder.bat -playlistOutput review.otio
```

This is a fresh, dynamic construction at launch, not a persistent filesystem
watcher: rerun the command to include later folder changes.

## Supported editable subset

- One video track containing clips.
- Optionally, one aligned audio track containing a clip or gap for every video
  item.
- Add, remove, and reorder media while preserving matching audio or silence.
- Save to `.otio`; a scratch playlist requires Save As before its first save.
- Numbered still-image paths added directly remain separate one-frame clips.
  Folder imports collapse eligible sequences by default; the
  `-playlistFrames` command-line flag keeps every frame separate.

## Read-only timelines

DJV retains complex OTIO timelines for playback and Save As, but never flattens
them to make them editable. Multiple video tracks, nested compositions,
transitions, unaligned audio, and packaged `.otioz` timelines are read-only.
The model exposes the reason so the UI can communicate the editing boundary.

## Verification

`djvPlaylistModelTest` checks item order, aligned audio/gaps, round-trip OTIO
serialization, image-sequence handling, read-only complex timelines, and
filtered folder selection. `djvFileFilterTest` and
`djvFolderScanServiceTest` cover bounded parsing, root confinement,
deterministic traversal, sequence collapsing, cancellation, queue saturation,
and atomic limit failures.
