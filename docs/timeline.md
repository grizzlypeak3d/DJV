# Timeline

The timeline is the strip under the view: the frames of the current file, where the current frame is, the in and out points, what is in the cache, and the markers. Clicking or dragging in it changes the current frame.

![Timeline](assets/timeline.svg)

The current frame is the red vertical line. The in/out range is represented by the blue line at the top of the timeline. The cache is represented by the green and purple lines. Markers, from a review or from an OTIO file, are drawn at their frames; see [Markers](reviews.html#markers).

## Moving around

- **Change the current frame** — Left mouse button, clicking or dragging
- **Zoom** — Mouse wheel, or the <kbd>-</kbd> and <kbd>=</kbd> keys
- **Frame the whole timeline** — <kbd>Backspace</kbd>, or **Timeline > Frame View**
- **Pan** — Middle mouse button

**Timeline > Auto Scroll** keeps the current frame in sight while playing, scrolling the timeline as it goes. **Timeline > Scroll Bars** shows scroll bars for a zoomed timeline.

Locations: **Timeline** menu

## Scrubbing

Scrubbing while playing pauses playback for as long as the mouse button is held, so the picture follows the pointer, and playback resumes from where the drag ends. **Timeline > Stop Playback When Scrubbing** stops playback instead, so that it stays stopped after the drag.

Resting the cursor on the timeline shows a preview of the frame under it, drawn the way the view would show it, with the frame's time. **Timeline > Hover Preview** turns it off. The preview is shown while playback is stopped.

Locations: **Timeline** menu

## Tracks and media

By default, the timeline is minimized and shows only the first video and audio track. **Timeline > Minimize** turns that off to show every track.

The video thumbnails and audio waveforms can be shown and sized independently: **Timeline > Video Thumbnails**, **Timeline > Audio Waveforms**, and their sizes under **Timeline > Video Thumbnail Size** and **Timeline > Audio Waveform Size**. **Timeline > Track Media** (<kbd>C</kbd>) turns both on or off at once. Turning them off saves vertical space and improves performance; with both off, the timeline is a compact strip with just the current frame, in/out range, and cache.

Locations: **Timeline** menu

## Comparing timelines

**Timeline > Compare Timelines** shows the timeline of each file being compared under the current file's, which is useful when the files are two cuts of the same edit. It is off by default. See [A/B comparison](compare.html).

Locations: **Timeline** menu
