---
title: Automation
layout: default
nav_order: 14
---

# Automation

DJV's functionality is exposed as *commands* — named operations that menus and
keyboard shortcuts invoke internally, and that can also be executed from the
command line for scripting and automation.

## Listing the commands

```
djv -listCommands
```

This prints every command with a short description:

```
Playback/Forward - Start forward playback.
Playback/InOutRange - Set the playback in/out range from inclusive frames relative to the timeline start; e.g., { "in": 10, "out": 50 }.
Playback/Seek - Seek to a frame, relative to the timeline start; e.g., { "frame": 100 }.
Tools/Export - Toggle the Export tool; e.g., { "value": true }.
...
```

Command names follow the menu structure, and are the same names used for the
keyboard shortcuts in the **Settings** tool.

## Executing commands

Use the **-command** option to execute a command after startup:

```
djv render.mov -command "Playback/Forward"
```

Some commands take JSON arguments, given after the command name. Quote the
whole thing as a single argument; on Linux and macOS, single quotes also
protect the JSON's double quotes:

```
djv render.mov -command 'Playback/Seek { "frame": 100 }'
```

On Windows, use double quotes and escape the inner quotes:

```
djv render.mov -command "Playback/Seek { \"frame\": 100 }"
```

Toggle commands take a boolean value:

```
djv render.mov -command 'Tools/Export { "value": true }'
```

The **-command** option can be repeated to execute multiple commands in order:

```
djv render.mov \
    -command 'Playback/InOutRange { "in": 100, "out": 200 }' \
    -command 'Playback/Seek { "frame": 100 }' \
    -command 'Playback/Forward'
```

When files are given on the command line, commands wait until the files have
been opened before executing.

Errors, such as unknown commands or malformed arguments, are reported in the
log (see [Troubleshooting]({{ '/troubleshooting' | relative_url }})).

## Exiting

**File/Exit** is itself a command, so it can be used as the final command to
run DJV as a batch process:

```
djv -command 'Timeline/WaveformSizeLarge' -command 'File/Exit'
```

Since settings are saved on exit, this example changes the timeline waveform
size for future sessions and then exits.

## Command line basics

One or more files, directories, or timelines can be given on the command line:

```
djv render.mov
```

Image sequences can be opened by specifying the first frame or using the "#"
wildcard (see [Files]({{ '/files' | relative_url }})):

```
djv render.#.exr
```

The setup dialog that is shown on the first run is automatically hidden when
**-command** is used, and stays available for the next interactive run. The
**-hideSetup** option hides it for other automated runs.

Use the **-h** option to see the complete list of command line options,
including playback, comparison, color, and window options.
