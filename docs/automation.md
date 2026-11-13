# Automation

DJV's functionality is exposed as *commands* — named operations that menus and keyboard shortcuts invoke internally, and that can also be executed from the command line for scripting and automation.

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

Command names follow the menu structure, and are the same names used for the keyboard shortcuts in the **Settings** tool.

## Executing commands

Use the **-command** option to execute a command after startup:

```
djv render.mov -command "Playback/Forward"
```

Some commands take JSON arguments, given after the command name. Quote the whole thing as a single argument; on Linux and macOS, single quotes also protect the JSON's double quotes:

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

When files are given on the command line, commands wait until the files have been opened before executing.

Errors, such as unknown commands or malformed arguments, are reported in the log (see [Troubleshooting](troubleshooting.html)).

## Exiting

`File/Exit` is itself a command, so it can be used as the final command to run DJV as a batch process:

```
djv -command 'Timeline/WaveformSizeLarge' -command 'File/Exit'
```

Since settings are saved on exit, this example changes the timeline waveform size for future sessions and then exits.

## Exporting

`Export/Movie` exports the current file as a movie with the **Export** tool's settings, changing any that are given first:

```
djv render.#.exr -offscreen \
    -command 'Export/Movie { "dir": "/tmp", "fileName": "review", "preset": "APV 422", "exit": true }'
```

The arguments are all optional: **dir**, **fileName**, **ext**, **preset** (as the **Preset** menu names it), **audioCodec** and **burnAnnotations**, which writes the review drawings into the picture. An existing file is written over only with **"overwrite": true**, since there is nobody to answer the question. The export takes as long as it takes, so `File/Exit` after it would stop it part way; **"exit": true** quits once the movie is written or has failed, and the log says which. **-offscreen** keeps the window from being shown while it runs.

## Color settings

Settings changed from the command line are saved like any others, so a later run starts from them. A script that wants the same color every time, whatever the user changed last, resets them first. With the **OCIO** environment variable set, a reset turns OCIO on with that configuration, so this is a clean start with the site's configuration:

```
djv -command 'Color/Reset' "$@"
```

The rest of what the script wants is set after the reset:

```
djv "$@" \
    -command 'Color/Reset' \
    -command 'Color/OCIO { "display": "sRGB", "view": "ACES 1.0 - SDR Video" }'
```

`Color/OCIO` takes **config** (**Built In**, **Environment Variable**, or **File**), **fileName**, **input**, **display**, **view**, and **look**, and `Color/LUT` takes **fileName** and **order**; either turns its section on unless **"value": false** is given. `Color/Reset` leaves a section as it is when given **"ocio"**, **"lut"**, **"color"**, or **"levels"** as false. The **-ocio** and **-lut** options are applied before any command, so a `Color/Reset` would undo them; use the commands after it instead. The user can change the settings during the session, and the next run of the script starts from its own again.

## Command line basics

One or more files, directories, or timelines can be given on the command line:

```
djv render.mov
```

Image sequences can be opened by specifying the first frame or using the "#" wildcard (see [File formats](formats.html#partial-image-sequences)):

```
djv render.#.exr
```

The tour that is shown on the first run is automatically hidden when **-command** is used, and stays available for the next interactive run. The **-hideSetup** option hides it for other automated runs.

The [Command line](command-line.html) page lists every option, as **-h** prints them, including playback, comparison, color, and window options.

## Debugging and introspection {#debugging}

```
djv -screenshot window.png
djv -widgetDump widgets.json
```

**-screenshot** writes an image of the window to a file and exits. **-widgetDump** writes the window's widget tree to a file as JSON — the type, position, size, state, and text of every widget — and exits. Both work in any application built on the UI library, including the Python application.

`Debug/State` is a command that writes the live model state as JSON — the display, color, and audio options, the open files, and the player — to standard output, or to a file given as an argument:

```
djv render.mov -command 'Debug/State {"file": "state.json"}' -command 'File/Exit'
```

Setting the environment variable **FTK_TRACE_EVENTS** logs keyboard and mouse button dispatch: each widget an event is offered to, in order, and which one accepted it.

The log's **Library** line records which library file the application loaded and when it was last modified, which tells a stale build from a fresh one.

## Widgets from JSON

The reverse of the widget dump: a widget tree can be created from JSON, with the structure and the properties in the data and the behavior in code. From the Python bindings:

```
import feather_tk as ftk

widget, errors = ftk.widgetLoad(context, open("panel.json").read())
ftk.findWidget(widget, "apply").setClickedCallback(callback)
```

See the feather-tk documentation for the format. Its **preview** example renders a layout file and reloads it as the file is edited.
