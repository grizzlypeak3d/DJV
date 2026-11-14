# Actions reference

Every menu item, with its default keys and the name of its command, as the
application reports them. The descriptions are the items' tooltips. This
page is made from the application itself, so it lists exactly what the
version it was made from has.

**Ctrl/Cmd** is Command on macOS and Control elsewhere; **Ctrl** is Control
everywhere. The keys can be changed in the **Shortcuts** section of the
**Settings** tool.

The commands are how the menu items are run from outside the window: with
`-command` on the [command line](command-line.html), and from
[Python](python.html). Commands that take a value say so.

## File

| Menu item | Keys | Command | Description |
|---|---|---|---|
| Open | <kbd>Ctrl/Cmd+O</kbd> | `File/Open` | Open a file. |
| Open With Audio | <kbd>Ctrl/Cmd+Shift+O</kbd> | `File/OpenAudio` | Open a file with a separate audio file. |
| Recent Files | <kbd>Ctrl/Cmd+Shift+R</kbd> | `File/RecentFiles` | Show the recent files, to search and open from. |
| Close | <kbd>Ctrl/Cmd+E</kbd> | `File/Close` | Close the current file. |
| Close All | <kbd>Ctrl/Cmd+Shift+E</kbd> | `File/CloseAll` | Close all files. |
| Reload | <kbd>Ctrl/Cmd+R</kbd> | `File/Reload` | Reload the current file. |
| Export Still | <kbd>Ctrl/Cmd+Shift+S</kbd> | `File/ExportStill` | Write a still of the current frame, as the viewport shows it, into the Export tool's directory in its image format, named after the file and the time. |
| Open Playlist (.otio) |  | `File/OpenPlaylist` | Open a playlist into the file list. Opening a ".otio" file normally plays it as a timeline. |
| Save Playlist (.otio) |  | `File/SavePlaylist` | Save the file list as a ".otio" playlist. |
| Next | <kbd>Ctrl+Page Down</kbd> | `File/Next` | Change to the next file. |
| Previous | <kbd>Ctrl+Page Up</kbd> | `File/Prev` | Change to the previous file. |
| Next Layer | <kbd>Ctrl+=</kbd> | `File/NextLayer` | Change to the next layer. |
| Previous Layer | <kbd>Ctrl+-</kbd> | `File/PrevLayer` | Change to the previous layer. |
| Next Media Reference | <kbd>Shift+M</kbd> | `File/NextMediaReference` | Change to the next media reference. |
| Exit | <kbd>Ctrl/Cmd+Q</kbd> | `File/Exit` | Exit the application. |
| Recent > | | | A list the application fills in: files, layers, sizes. |
| Recent Playlists > | | | A list the application fills in: files, layers, sizes. |
| Current > | | | A list the application fills in: files, layers, sizes. |
| Layers > | | | A list the application fills in: files, layers, sizes. |
| Media References > | | | A list the application fills in: files, layers, sizes. |

## Review

| Menu item | Keys | Command | Description |
|---|---|---|---|
| Open (.djvr) | <kbd>Ctrl/Cmd+Alt+O</kbd> | `Review/Open` | Open a review, replacing the current session. |
| Save | <kbd>Ctrl/Cmd+Alt+S</kbd> | `Review/Save` | Save the current session as a review. |
| Save As (.djvr)... |  | `Review/SaveAs` | Save the current session as a new review. |
| Import... |  | `Review/Import` | Import an OTIO timeline as a review: it becomes the review's first file, and its markers become the feedback. |
| Export Markers (.otio)... |  | `Review/Export` | Write the review's markers to an OTIO file. |
| Close |  | `Review/Close` | Close the review and reset to the startup state. |
| Select |  | `Review/Select` | Select a stroke: click it, and drag to move it. Handles: move a point of a shape. Color and sizes: change it. Delete: remove it. Double click or Return: write text again. Click again to stop. |
| Pen |  | `Review/Draw` | Draw strokes. Click again to stop drawing. |
| Line |  | `Review/Line` | Draw lines. Shift: a line at a multiple of forty-five degrees. Click again to stop drawing. |
| Arrow |  | `Review/Arrow` | Draw arrows, the head where the drag ends. Shift: an arrow at a multiple of forty-five degrees. Click again to stop drawing. |
| Rectangle |  | `Review/Rectangle` | Draw rectangles. Shift: a square. Click again to stop drawing. |
| Ellipse |  | `Review/Ellipse` | Draw ellipses. Shift: a circle. Click again to stop drawing. |
| Text |  | `Review/Text` | Write text: click where it starts and type. Return: keep it. Escape: let it go. Click again to stop. |
| Eraser |  | `Review/Erase` | Erase the strokes you touch. Click again to stop. |
| Onion Skin |  | `Review/OnionSkin` | Show the drawings on the frames before and after, faded, behind this frame's. |
| Square and Straight Shapes |  | `Review/Constrain` | Hold rectangles square, ellipses round, and lines and arrows to multiples of 45 degrees, as Shift does. |
| Undo Drawing | <kbd>Ctrl/Cmd+Z</kbd> | `Review/Undo` | Undo drawing. |
| Redo Drawing | <kbd>Ctrl/Cmd+Shift+Z</kbd>, <kbd>Ctrl+Y</kbd> (not macOS) | `Review/Redo` | Redo drawing. |
| Clear Drawing |  | `Review/ClearDrawing` | Remove every stroke on this frame. |
| Add Marker |  | `Review/AddNote` | Add a marker about the current frame, written in place. |
| Add Range |  | `Review/AddRange` | Add a marker for the timeline in/out points, written in place. |
| Previous Marker | <kbd>Alt+Left</kbd> | `Review/PrevFrame` | Go to the previous frame with a marker or a drawing. |
| Next Marker | <kbd>Alt+Right</kbd> | `Review/NextFrame` | Go to the next frame with a marker or a drawing. |
| Recent > | | | A list the application fills in: files, layers, sizes. |

## Compare

| Menu item | Keys | Command | Description |
|---|---|---|---|
| Next | <kbd>Shift+Page Down</kbd> | `Compare/Next` | Go to the next B file. |
| Previous | <kbd>Shift+Page Up</kbd> | `Compare/Prev` | Go to the previous B file. |
| B | <kbd>Ctrl+B</kbd> | `Compare/B` | Show the B file. |
| Wipe | <kbd>Ctrl+W</kbd> | `Compare/Wipe` | Wipe between the A and B files. |
| Butterfly |  | `Compare/Butterfly` | Show the same half of the A and B files, one of them mirrored, so that they meet on the same picture. |
| Overlay |  | `Compare/Overlay` | Overlay the A and B files. |
| Difference |  | `Compare/Difference` | Show the difference between the A and B files. |
| Horizontal |  | `Compare/Horizontal` | Show the A and B files in a horizontal layout. |
| Vertical |  | `Compare/Vertical` | Show the A and B files in a vertical layout. |
| Tile | <kbd>Ctrl+T</kbd> | `Compare/Tile` | Show the A and B files in a tiled layout. |
| Hold Frame | <kbd>Shift+H</kbd> | `Compare/HoldFrame` | Hold the current frame and compare against it: it becomes the comparison's B, wiped against A if no other comparison is chosen, whatever the time or the file, until it is let go. Holding again holds the frame then current. |
| Clear Held Frame | <kbd>Alt+Shift+H</kbd> | `Compare/ClearHeld` | Let go of the held frame. |
| B > | | | A list the application fills in: files, layers, sizes. |
| Sync by > Start of File |  | `Compare/Relative` | Compare files using relative time. |
| Sync by > Timecode |  | `Compare/Absolute` | Compare files using absolute time. |

## Playback

| Menu item | Keys | Command | Description |
|---|---|---|---|
| Stop | <kbd>K</kbd> | `Playback/Stop` | Stop playback. |
| Forward | <kbd>L</kbd> | `Playback/Forward` | Start forward playback. |
| Reverse | <kbd>J</kbd> | `Playback/Reverse` | Start reverse playback. |
| Toggle Playback | <kbd>Space</kbd> | `Playback/Toggle` | Toggle playback. |
| Jump Back 1s | <kbd>Shift+J</kbd> | `Playback/JumpBack1s` | Jump back 1 second. |
| Jump Back 10s | <kbd>Ctrl+J</kbd> | `Playback/JumpBack10s` | Jump back 10 seconds. |
| Jump Forward 1s | <kbd>Shift+L</kbd> | `Playback/JumpForward1s` | Jump forward 1 second. |
| Jump Forward 10s | <kbd>Ctrl+L</kbd> | `Playback/JumpForward10s` | Jump forward 10 seconds. |
| Playback Loop |  | `Playback/Loop` | Loop playback continuously. |
| Playback Once |  | `Playback/Once` | Playback once and stop. |
| Playback Ping-Pong |  | `Playback/PingPong` | Playback forward and reverse continuously. |
| Set In Point | <kbd>I</kbd> | `Playback/SetInPoint` | Set the playback in point to the current frame. |
| Reset In Point | <kbd>Shift+I</kbd> | `Playback/ResetInPoint` | Reset the playback in point. |
| Set Out Point | <kbd>O</kbd> | `Playback/SetOutPoint` | Set the playback out point to the current frame. |
| Reset Out Point | <kbd>Shift+O</kbd> | `Playback/ResetOutPoint` | Reset the playback out point. |

## Frame

| Menu item | Keys | Command | Description |
|---|---|---|---|
| Go To Start | <kbd>Home</kbd> | `Frame/Start` | Go to the start frame. |
| Go To End | <kbd>End</kbd> | `Frame/End` | Go to the end frame. |
| Previous Frame | <kbd>Left</kbd> | `Frame/Prev` | Go to the previous frame. |
| Previous Frame X10 | <kbd>Shift+Left</kbd> | `Frame/PrevX10` | Go to the previous frame X10. |
| Previous Frame X100 | <kbd>Ctrl+Left</kbd> | `Frame/PrevX100` | Go to the previous frame X100. |
| Next Frame | <kbd>Right</kbd> | `Frame/Next` | Go to the next frame. |
| Next Frame X10 | <kbd>Shift+Right</kbd> | `Frame/NextX10` | Go to the next frame X10. |
| Next Frame X100 | <kbd>Ctrl+Right</kbd> | `Frame/NextX100` | Go to the next frame X100. |
| Focus Current Frame | <kbd>Ctrl+F</kbd> | `Frame/FocusCurrent` | Set the keyboard focus to the current frame editor. |

## Timeline

| Menu item | Keys | Command | Description |
|---|---|---|---|
| Minimize |  | `Timeline/Minimize` | Minimize the timeline. |
| Frame View |  | `Timeline/FrameView` | Frame the timeline view. |
| Scroll Bars |  | `Timeline/ScrollBars` | Toggle the scroll bars. |
| Auto Scroll |  | `Timeline/AutoScroll` | Automatically scroll the timeline to the current frame. |
| Stop Playback When Scrubbing |  | `Timeline/StopOnScrub` | Stop playback when scrubbing the timeline, so that it stays stopped after the drag; otherwise it is paused while the button is held and resumes from where the drag ends. |
| Hover Preview |  | `Timeline/Preview` | Show a preview of the frame under the cursor when hovering the timeline. |
| Compare Timelines |  | `Timeline/CompareTimelines` | Show the timeline of each file being compared, under the current file's. |
| Track Media | <kbd>C</kbd> | `Timeline/TrackMedia` | Toggle the timeline video thumbnails and audio waveforms. |
| Video Thumbnails |  | `Timeline/Thumbnails` | Toggle the timeline video thumbnails. |
| Audio Waveforms |  | `Timeline/Waveforms` | Toggle the timeline audio waveforms. |
| Detect Cuts |  | `Timeline/DetectCuts` | Find the cuts in the current file, over the in/out range, and add a marker at each, except where there is one already. |
| Video Thumbnail Size > Small |  | `Timeline/ThumbnailSizeSmall` | Small timeline thumbnails. |
| Video Thumbnail Size > Medium |  | `Timeline/ThumbnailSizeMedium` | Medium timeline thumbnails. |
| Video Thumbnail Size > Large |  | `Timeline/ThumbnailSizeLarge` | Large timeline thumbnails. |
| Audio Waveform Size > Small |  | `Timeline/WaveformSizeSmall` | Small timeline audio waveforms. |
| Audio Waveform Size > Medium |  | `Timeline/WaveformSizeMedium` | Medium timeline audio waveforms. |
| Audio Waveform Size > Large |  | `Timeline/WaveformSizeLarge` | Large timeline audio waveforms. |

## Audio

| Menu item | Keys | Command | Description |
|---|---|---|---|
| Volume Up | <kbd>.</kbd> | `Audio/VolumeUp` | Increase the audio volume. |
| Volume Down | <kbd>,</kbd> | `Audio/VolumeDown` | Decrease the audio volume. |
| Mute | <kbd>M</kbd> | `Audio/Mute` | Toggle the audio mute. |

## View

| Menu item | Keys | Command | Description |
|---|---|---|---|
| Frame | <kbd>Backspace</kbd> | `View/Frame` | Frame the view to fit the image. |
| Zoom Reset | <kbd>0</kbd> | `View/ZoomReset` | Reset the view zoom to 1:1. |
| Zoom In | <kbd>=</kbd> | `View/ZoomIn` | Zoom the view in. |
| Zoom Out | <kbd>-</kbd> | `View/ZoomOut` | Zoom the view out. |
| Center | <kbd>\</kbd> | `View/Center` | Center the view. |
| Red Channel | <kbd>R</kbd> | `View/Red` | Show the red channel. |
| Green Channel | <kbd>G</kbd> | `View/Green` | Show the green channel. |
| Blue Channel | <kbd>B</kbd> | `View/Blue` | Show the blue channel. |
| Alpha Channel | <kbd>A</kbd> | `View/Alpha` | Show the alpha channel. |
| Negative | <kbd>Ctrl+I</kbd> | `View/Negative` | Show the image as a negative. |
| Mirror Horizontal | <kbd>H</kbd> | `View/MirrorHorizontal` | Mirror the image horizontally. |
| Mirror Vertical | <kbd>V</kbd> | `View/MirrorVertical` | Mirror the image vertically. |
| Grid | <kbd>Ctrl+G</kbd> | `View/Grid` | Toggle the grid. |
| Outline |  | `View/Outline` | Toggle the outline. |
| Center Marker |  | `View/CenterMarker` | Toggle the center marker. |
| Clipping Warning |  | `View/ClippingWarning` | Cover pixels outside the display range with a warning color: red above, magenta below. |
| HUD / Information Display | <kbd>Ctrl+H</kbd> | `View/HUD` | Toggle the HUD / information display. |
| Aspect Ratio > Default |  | `View/AspectRatio_0` | Set the aspect ratio to the default. |
| Aspect Ratio > 16:9 DAR |  | `View/AspectRatio_1` | Set the aspect ratio to 16:9 display aspect ratio (DAR). |
| Aspect Ratio > 1.85:1 DAR |  | `View/AspectRatio_2` | Set the aspect ratio to 1.85:1 display aspect ratio (DAR). |
| Aspect Ratio > 2.39:1 DAR |  | `View/AspectRatio_3` | Set the aspect ratio to 2.39:1 display aspect ratio (DAR). |
| Mouse Tool > None |  | `View/MouseTool_0` | Set the mouse tool: None. |
| Mouse Tool > Pan View |  | `View/MouseTool_1` | Set the mouse tool: Pan View. |
| Mouse Tool > Compare Wipe |  | `View/MouseTool_2` | Set the mouse tool: Compare Wipe. |
| Mouse Tool > Pick |  | `View/MouseTool_3` | Set the mouse tool: Pick. |
| Mouse Tool > Frame Shuttle |  | `View/MouseTool_4` | Set the mouse tool: Frame Shuttle. |

## Window

| Menu item | Keys | Command | Description |
|---|---|---|---|
| Full Screen | <kbd>U</kbd> | `Window/FullScreen` | Toggle the window full screen. |
| Presentation | <kbd>Ctrl+P</kbd> | `Window/PresentMode` | Toggle presentation mode. |
| Float On Top |  | `Window/FloatOnTop` | Toggle the window floating on top. |
| Secondary | <kbd>Y</kbd> | `Window/Secondary` | Toggle the secondary window. |
| File Tool Bar |  | `Window/FileToolBar` | Toggle the file tool bar. |
| Compare Tool Bar |  | `Window/CompareToolBar` | Toggle the compare tool bar. |
| View Tool Bar |  | `Window/ViewToolBar` | Toggle the view tool bar. |
| Window Tool Bar |  | `Window/WindowToolBar` | Toggle the window tool bar. |
| Tools Tool Bar |  | `Window/ToolsToolBar` | Toggle the tools tool bar. |
| Tab Bar |  | `Window/TabBar` | Toggle the tab bar. |
| Timeline |  | `Window/Timeline` | Toggle the timeline. |
| Bottom Tool Bar |  | `Window/BottomToolBar` | Toggle the bottom tool bar. |
| Status Tool Bar |  | `Window/StatusToolBar` | Toggle the status tool bar. |
| Tools |  | `Window/Tools` | Toggle the tools panel. |
| Resize > | | | A list the application fills in: files, layers, sizes. |

## Color

| Menu item | Keys | Command | Description |
|---|---|---|---|
| Enable OCIO | <kbd>Ctrl+N</kbd> | `Color/OCIO` | Toggle whether OCIO is enabled. Also takes "config" ("Built In", "Environment Variable", or "File"), "fileName", "input", "display", "view", and "look". |
| Enable LUT | <kbd>Ctrl+K</kbd> | `Color/LUT` | Toggle whether the LUT is enabled. Also takes "fileName" and "order" ("Post-Config" or "Pre-Config"). |
| Enable Color |  | `Color/Enabled` | Turn all of the color settings off, to see the image unaltered, and back on again as they were. |
| Reset Color |  | `Color/Reset` | Reset the color settings to their defaults. Takes "ocio", "lut", "color", and "levels" to leave a section as it is with false; e.g., { "lut": false }. |

## Tools

| Menu item | Keys | Command | Description |
|---|---|---|---|
| Files | <kbd>F1</kbd> | `Tools/Files` | Toggle the Files tool. |
| Export | <kbd>F2</kbd> | `Tools/Export` | Toggle the Export tool. |
| View | <kbd>F3</kbd> | `Tools/View` | Toggle the View tool. |
| Color | <kbd>F4</kbd> | `Tools/Color` | Toggle the Color tool. |
| Color Picker | <kbd>F5</kbd> | `Tools/Color Picker` | Toggle the Color Picker tool. |
| Magnify | <kbd>F6</kbd> | `Tools/Magnify` | Toggle the Magnify tool. |
| Information | <kbd>F7</kbd> | `Tools/Information` | Toggle the Information tool. |
| Audio | <kbd>F8</kbd> | `Tools/Audio` | Toggle the Audio tool. |
| Review | <kbd>F9</kbd> | `Tools/Review` | Toggle the Review tool. |
| Settings | <kbd>F10</kbd> | `Tools/Settings` | Toggle the Settings tool. |
| Messages | <kbd>F11</kbd> | `Tools/Messages` | Toggle the Messages tool. |
| System Log | <kbd>F12</kbd> | `Tools/System Log` | Toggle the System Log tool. |
| Diagnostics |  | `Tools/Diagnostics` | Toggle the Diagnostics tool. |

## Help

| Menu item | Keys | Command | Description |
|---|---|---|---|
| Tour |  | `Help/Tour` | Show the tour of the window: each part of it, what it does, and how it is worked. |
| Documentation |  | `Help/Documentation` | Open the documentation in a web browser. |
| Get DJV Studio |  | `Help/Studio` | Open the web site for DJV Studio, the commercial version of DJV. |
| About |  | `Help/About` | Show the about dialog. |
| System Information |  | `Help/SysInfo` | Show the system information dialog. |

## Commands without a menu item

For scripts and the command line.

| Command | Description |
|---|---|
| `Debug/State` | Write the live model state as JSON, to a file or standard output; e.g., { "file": "state.json" }. |
| `Export/Movie` | Export the current file as a movie with the Export tool's settings, changing any given first; e.g., { "dir": "/tmp", "fileName": "out", "ext": ".mov", "preset": "APV 422", "audioCodec": "Auto", "overwrite": true, "exit": true }. "exit" quits once the movie is written or has failed. |
| `File/Layer` | Set a file's layer by name or index, e.g., { "layer": "diffuse" }. It is the "A" file's unless "file" gives the index of another in the file list. |
| `File/MediaReference` | Set a file's media reference by name, e.g., { "key": "Proxy" }; "As Authored" or an empty key is the references the clips were authored with. It is the "A" file's unless "file" gives the index of another in the file list. |
| `Playback/InOutRange` | Set the playback in/out range from inclusive frames relative to the timeline start; e.g., { "in": 10, "out": 50 }. |
| `Playback/Seek` | Seek to a frame, relative to the timeline start; e.g., { "frame": 100 }. |
| `Timeline/Zoom` | Zoom the timeline, in pixels per second, keeping the current frame in place; e.g., { "value": 100 }. |
