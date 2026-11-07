// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

//! \file Models.h
//! A C interface to DJV's models, over tlRender's C interface to the
//! engine: the color, audio and viewport models, with their settings,
//! applied to a view and a player. For Swift and any other language that
//! speaks C. Everything here is called on the main thread.
//!
//! Strings returned are the models' and stay valid until the next call
//! that changes the model they came from. Changes are counted: a caller
//! that polls compares the generation and reads again when it moved.

#include <tlRender/C/Engine.h>

//! djvC is a static library, linked into the one application: nothing
//! to export or import, whether tlRender's libraries are shared or not.
#define DJV_C_API

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! The models: created on an engine with a settings file, bound to the
//! view they are drawn with and the player they apply to.
typedef struct djv_models djv_models;

DJV_C_API djv_models* djv_models_create(tl_engine*, const char* settingsPath);
DJV_C_API void djv_models_destroy(djv_models*);

//! Write the settings.
DJV_C_API void djv_models_save(djv_models*);

//! The view the color and viewport options are drawn with; NULL unbinds.
DJV_C_API void djv_models_bind_view(djv_models*, tl_view*);

//! A second view, as the desktop's secondary window is one: on a second
//! display, a television or a projector, drawn with the same options and
//! the drawings, but without the selection's frame, the text's caret or
//! the wipe's handles, which are for whoever is drawing. Where its
//! picture is, is the caller's. NULL unbinds.
DJV_C_API void djv_models_bind_output(djv_models*, tl_view*);

//! The player the audio applies to and the file the color is resolved
//! for; NULL for none.
DJV_C_API void djv_models_set_player(djv_models*, tl_player*);

//! Moves on every change to any model, from any side.
DJV_C_API uint64_t djv_models_get_generation(djv_models*);

//! \name OpenColorIO
///@{

//! Whether OpenColorIO is built in: the rest of this section does
//! nothing without it.
DJV_C_API int djv_ocio_is_available(void);
DJV_C_API int djv_ocio_is_enabled(djv_models*);
DJV_C_API void djv_ocio_set_enabled(djv_models*, int);

//! Where the configuration comes from.
typedef enum
{
    DJV_OCIO_CONFIG_BUILT_IN,
    DJV_OCIO_CONFIG_ENV,
    DJV_OCIO_CONFIG_FILE
} djv_ocio_config;
DJV_C_API djv_ocio_config djv_ocio_get_config(djv_models*);
DJV_C_API void djv_ocio_set_config(djv_models*, djv_ocio_config);
DJV_C_API const char* djv_ocio_get_file_name(djv_models*);
DJV_C_API void djv_ocio_set_file_name(djv_models*, const char*);
//! The configuration's own name.
DJV_C_API const char* djv_ocio_get_name(djv_models*);

//! The choices the configuration offers, and which is chosen.
DJV_C_API size_t djv_ocio_get_input_count(djv_models*);
DJV_C_API const char* djv_ocio_get_input(djv_models*, size_t);
DJV_C_API size_t djv_ocio_get_input_index(djv_models*);
DJV_C_API void djv_ocio_set_input_index(djv_models*, size_t);
DJV_C_API size_t djv_ocio_get_display_count(djv_models*);
DJV_C_API const char* djv_ocio_get_display(djv_models*, size_t);
DJV_C_API size_t djv_ocio_get_display_index(djv_models*);
DJV_C_API void djv_ocio_set_display_index(djv_models*, size_t);
DJV_C_API size_t djv_ocio_get_view_count(djv_models*);
DJV_C_API const char* djv_ocio_get_view(djv_models*, size_t);
DJV_C_API size_t djv_ocio_get_view_index(djv_models*);
DJV_C_API void djv_ocio_set_view_index(djv_models*, size_t);
DJV_C_API size_t djv_ocio_get_look_count(djv_models*);
DJV_C_API const char* djv_ocio_get_look(djv_models*, size_t);
DJV_C_API size_t djv_ocio_get_look_index(djv_models*);
DJV_C_API void djv_ocio_set_look_index(djv_models*, size_t);

//! The input color space resolved for the player's file and where it
//! came from, for display: "sRGB (extension)"; empty when nothing
//! resolves and the chosen input is used.
DJV_C_API const char* djv_color_get_resolved_input(djv_models*);

///@}

//! \name Status
//! What changes the picture or the sound from the media as it is: the
//! options the desktop's status indicator reminds of, those that are on,
//! in its order, by an id ("LUT", "Negative", ...) and a name. Read again
//! on each count.
///@{

DJV_C_API size_t djv_status_get_count(djv_models*);
DJV_C_API const char* djv_status_get_id(djv_models*, size_t index);
DJV_C_API const char* djv_status_get_label(djv_models*, size_t index);

///@}

//! \name Media references
//! A timeline's clips may carry several versions of their media, chosen by
//! key: a proxy and the full resolution, say. As the desktop's File > Media
//! References has them: "As Authored", the empty key, first, which leaves
//! each clip on the reference it was made with, then the timeline's keys.
//! The choice is the review's, kept with it for the file shown, and put
//! back when the review is opened.
///@{

//! How many there are to choose from: none when the timeline has no
//! versions to choose between, its clips each on OTIO's default key alone.
//! The keys are read again here.
DJV_C_API size_t djv_media_reference_get_count(djv_models*);
DJV_C_API const char* djv_media_reference_get_key(djv_models*, size_t index);
//! A key's name as the desktop shows it: "As Authored" for the empty key,
//! "Default" for OTIO's default key, the key itself otherwise.
DJV_C_API const char* djv_media_reference_get_label(djv_models*, const char* key);
//! The key the player uses.
DJV_C_API const char* djv_media_reference_get_current(djv_models*);
//! Use a key: the media is read again, and the review keeps it.
DJV_C_API void djv_media_reference_set_current(djv_models*, const char* key);

///@}

//! \name LUT
///@{

DJV_C_API int djv_lut_is_enabled(djv_models*);
DJV_C_API void djv_lut_set_enabled(djv_models*, int);
DJV_C_API const char* djv_lut_get_file_name(djv_models*);
DJV_C_API void djv_lut_set_file_name(djv_models*, const char*);
typedef enum
{
    DJV_LUT_ORDER_POST_CONFIG,
    DJV_LUT_ORDER_PRE_CONFIG
} djv_lut_order;
DJV_C_API djv_lut_order djv_lut_get_order(djv_models*);
DJV_C_API void djv_lut_set_order(djv_models*, djv_lut_order);
typedef enum
{
    DJV_LUT_DIRECTION_FORWARD,
    DJV_LUT_DIRECTION_INVERSE
} djv_lut_direction;
DJV_C_API djv_lut_direction djv_lut_get_direction(djv_models*);
DJV_C_API void djv_lut_set_direction(djv_models*, djv_lut_direction);
//! The extensions of the LUT files this build reads, through OpenColorIO:
//! a space-separated list, lowercase, without the dot ("cube 3dl ...");
//! empty without OpenColorIO. The same string each time.
DJV_C_API const char* djv_lut_get_extensions(void);

///@}

//! \name Audio
///@{

DJV_C_API float djv_audio_get_volume(djv_models*);
DJV_C_API void djv_audio_set_volume(djv_models*, float);
//! A step up or down, as DJV's Volume Up and Volume Down take it.
DJV_C_API void djv_audio_volume_up(djv_models*);
DJV_C_API void djv_audio_volume_down(djv_models*);
DJV_C_API int djv_audio_is_muted(djv_models*);
DJV_C_API void djv_audio_set_muted(djv_models*, int);
//! Seconds the audio is moved by against the video.
DJV_C_API double djv_audio_get_sync_offset(djv_models*);
DJV_C_API void djv_audio_set_sync_offset(djv_models*, double);

///@}

//! \name Viewport
///@{

//! How the picture is shown: the viewport model's display options, as
//! plain numbers.
typedef struct djv_display_options
{
    //! 0 color, 1 red, 2 green, 3 blue, 4 alpha.
    int channels;
    int mirrorX;
    int mirrorY;
    int negative;

    int colorEnabled;
    float add[3];
    float brightness[3];
    float contrast[3];
    float saturation[3];
    float hue;

    int levelsEnabled;
    float levelsInLow;
    float levelsInHigh;
    float levelsGamma;
    float levelsOutLow;
    float levelsOutHigh;

    int exposureEnabled;
    float exposure;

    int softClipEnabled;
    float softClip;
} djv_display_options;
DJV_C_API djv_display_options djv_viewport_get_display(djv_models*);
//! The options as they start, which a slider is reset to.
DJV_C_API djv_display_options djv_viewport_get_default_display(void);
DJV_C_API void djv_viewport_set_display(djv_models*, djv_display_options);

//! Whether any color adjustment is on (the color values, levels,
//! exposure, soft clip), and putting them back to their defaults: the
//! color values, exposure and soft clip with color, the levels with
//! levels. The channels, mirroring and negative are left alone.
DJV_C_API int djv_viewport_has_color_adjustments(djv_models*);
DJV_C_API void djv_viewport_reset_color_adjustments(djv_models*, int color, int levels);

//! How the image is sampled and blended.
typedef struct djv_image_options
{
    //! 0 from the file, 1 full range, 2 legal range.
    int videoLevels;
    //! 0 none, 1 straight, 2 premultiplied.
    int alphaBlend;
    //! 0 nearest, 1 linear, 2 high quality.
    int minify;
    int magnify;
} djv_image_options;
DJV_C_API djv_image_options djv_viewport_get_image(djv_models*);
DJV_C_API void djv_viewport_set_image(djv_models*, djv_image_options);

//! What is behind the picture.
typedef struct djv_background_options
{
    //! 0 solid, 1 checkers, 2 gradient.
    int type;
    float solid[4];
    float checkers0[4];
    float checkers1[4];
    int checkersSize;
    float gradient0[4];
    float gradient1[4];
    //! The outline around the picture.
    int outlineEnabled;
    int outlineWidth;
    float outlineColor[4];
} djv_background_options;
DJV_C_API djv_background_options djv_viewport_get_background(djv_models*);
DJV_C_API void djv_viewport_set_background(djv_models*, djv_background_options);

//! What is drawn over the picture: the grid, the center marker, the
//! indicator for a missing frame, the clipping warning.
typedef struct djv_foreground_options
{
    int gridEnabled;
    //! 0 cells of a size, 1 a count of cells.
    int gridCellMode;
    int gridCellSize;
    int gridCellCountX;
    int gridCellCountY;
    int gridLineWidth;
    float gridColor[4];
    //! 0 none, 1 pixels, 2 alphanumeric.
    int gridLabels;

    int centerMarkerEnabled;
    int centerMarkerSize;
    int centerMarkerWidth;
    float centerMarkerColor[4];

    int missingEnabled;
    int missingWidth;
    float missingColor[4];

    int clippingEnabled;
    //! 0 any channel, 1 all channels, 2 luminance.
    int clippingMode;
    float clippingLow;
    float clippingHigh;

    //! The frame of another aspect ratio within the picture, the rest
    //! masked, and the action and title safe areas of the frame.
    int frameGuideEnabled;
    float frameGuideAspect;
    float frameGuideMaskOpacity;
    int safeAreasEnabled;
    float safeAreasAction;
    float safeAreasTitle;
} djv_foreground_options;
DJV_C_API djv_foreground_options djv_viewport_get_foreground(djv_models*);
DJV_C_API void djv_viewport_set_foreground(djv_models*, djv_foreground_options);

//! The aspect ratios offered, as labels, and which is chosen; the first
//! is the picture's own.
DJV_C_API size_t djv_viewport_get_aspect_ratio_count(djv_models*);
DJV_C_API const char* djv_viewport_get_aspect_ratio(djv_models*, size_t);
DJV_C_API size_t djv_viewport_get_aspect_ratio_index(djv_models*);
DJV_C_API void djv_viewport_set_aspect_ratio_index(djv_models*, size_t);

///@}

//! \name HUD
///@{

//! The HUD's items, as DJV's viewport model has them.
typedef enum
{
    DJV_HUD_FILE_NAME,
    DJV_HUD_INFO,
    DJV_HUD_CACHE,
    DJV_HUD_TIME,
    DJV_HUD_VIEW_ZOOM,
    DJV_HUD_COLOR_PICKER,
    DJV_HUD_RENDER,
    DJV_HUD_LUT,
    DJV_HUD_COUNT
} djv_hud_item;

//! The items in the order they are listed and stacked, and each one's
//! name.
DJV_C_API size_t djv_hud_get_item_count(void);
DJV_C_API djv_hud_item djv_hud_get_item(size_t);
DJV_C_API const char* djv_hud_get_item_name(djv_hud_item);

//! The corners of the picture an item is shown in, or none.
typedef enum
{
    DJV_HUD_POS_NONE,
    DJV_HUD_POS_TOP_LEFT,
    DJV_HUD_POS_TOP_RIGHT,
    DJV_HUD_POS_BOTTOM_LEFT,
    DJV_HUD_POS_BOTTOM_RIGHT
} djv_hud_pos;

//! Whether the HUD is shown, and the corner each item is in.
DJV_C_API int djv_hud_is_enabled(djv_models*);
DJV_C_API void djv_hud_set_enabled(djv_models*, int);
DJV_C_API djv_hud_pos djv_hud_get_pos(djv_models*, djv_hud_item);
DJV_C_API void djv_hud_set_pos(djv_models*, djv_hud_item, djv_hud_pos);

//! An item's line about the player now, as the desktop shows it; empty
//! when it has nothing to say. The speed played, the frames dropped and
//! the view's zoom are the caller's. Valid until the next call.
DJV_C_API const char* djv_hud_get_text(
    djv_models*,
    djv_hud_item,
    double fps,
    size_t droppedFrames,
    double viewZoom);

///@}

//! \name Time Units
///@{

typedef enum
{
    DJV_TIME_UNITS_FRAMES,
    DJV_TIME_UNITS_SECONDS,
    DJV_TIME_UNITS_TIMECODE
} djv_time_units;

//! The units times are shown in, as DJV's settings keep them, and a
//! time of the player's in them: seconds from the start of its timeline,
//! shown as the timeline counts, from its own start time. Valid until
//! the next call.
DJV_C_API djv_time_units djv_time_get_units(djv_models*);
DJV_C_API void djv_time_set_units(djv_models*, djv_time_units);
DJV_C_API const char* djv_time_get_label(djv_models*, double seconds);

///@}

//! \name Frame Shuttle
///@{

//! Where the frame shuttle is, in seconds from the start of the player's
//! time, a drag of so many of the view's pixels sideways from where it
//! began at a time: as DJV's viewport steps through the frames with the
//! mouse, with its frame shuttle scale. The time it began at without a
//! player.
DJV_C_API double djv_shuttle_get_time(djv_models*, double start, float dx);

///@}

//! \name Comparing
//! Comparing the player's file, "A", with one other, "B", as DJV's
//! Compare menu does: the mode, the wipe, the overlay and the time are
//! the files model's, with its settings. A new player compares with
//! nothing.
///@{

//! The modes, as tlRender's compare options have them.
typedef enum
{
    DJV_COMPARE_NONE,
    DJV_COMPARE_B,
    DJV_COMPARE_WIPE,
    DJV_COMPARE_BUTTERFLY,
    DJV_COMPARE_OVERLAY,
    DJV_COMPARE_DIFFERENCE,
    DJV_COMPARE_HORIZONTAL,
    DJV_COMPARE_VERTICAL,
    DJV_COMPARE_TILE,
    DJV_COMPARE_COUNT
} djv_compare;

//! The modes' labels, as the desktop's Compare menu shows them.
DJV_C_API const char* djv_compare_get_label(djv_compare);

//! The file compared with, or NULL or an empty path for none, which
//! also turns the comparison off. Zero if the file cannot be opened,
//! with the reason in the engine's error.
DJV_C_API int djv_compare_set_file(djv_models*, const char* path);
DJV_C_API int djv_compare_has_file(djv_models*);

DJV_C_API djv_compare djv_compare_get_mode(djv_models*);
DJV_C_API void djv_compare_set_mode(djv_models*, djv_compare);

//! The wipe's center, as a fraction of the picture, and its rotation in
//! degrees.
DJV_C_API void djv_compare_get_wipe(djv_models*, float* x, float* y, float* rotation);
DJV_C_API void djv_compare_set_wipe_rotation(djv_models*, float);

//! Dragging the wipe with a finger, in the view's pixels, as the desktop
//! drags it with the mouse: its rotation handle turns it, and its center
//! moves it from where it was taken. The models draw its handles over
//! the picture while wiping. Zero from the beginning when not wiping, or
//! not on a handle, which leaves the drag to something else.
DJV_C_API int djv_compare_wipe_begin(djv_models*, float x, float y);
//! Whether a finger at a point is on one of the wipe's handles, which
//! take it before drawing does, as the desktop's handles take the mouse.
DJV_C_API int djv_compare_wipe_hit(djv_models*, float x, float y);
DJV_C_API void djv_compare_wipe_move(djv_models*, float x, float y);
DJV_C_API void djv_compare_wipe_end(djv_models*);

//! How much of B the overlay shows, from zero to one.
DJV_C_API float djv_compare_get_overlay(djv_models*);
DJV_C_API void djv_compare_set_overlay(djv_models*, float);

//! What the difference is multiplied by before it is shown.
DJV_C_API float djv_compare_get_difference_gain(djv_models*);
DJV_C_API void djv_compare_set_difference_gain(djv_models*, float);

//! How B's time follows A's: from each one's start (relative), or the
//! same time code (absolute); labeled as the desktop labels them.
DJV_C_API int djv_compare_is_absolute_time(djv_models*);
DJV_C_API void djv_compare_set_absolute_time(djv_models*, int);
DJV_C_API const char* djv_compare_get_time_label(int absolute);

///@}

//! \name Drawing
//! Drawing on the picture, as the desktop's Review tool does: the tool
//! and its color and size, kept in the settings; the strokes, kept in
//! DJV's annotations model and saved in a review (.djvr) beside the
//! media, which the desktop opens; and the input, in the view's pixels.
///@{

//! The tools, as DJV's draw model has them.
typedef enum
{
    DJV_DRAW_PEN,
    DJV_DRAW_ERASER,
    DJV_DRAW_LINE,
    DJV_DRAW_ARROW,
    DJV_DRAW_RECTANGLE,
    DJV_DRAW_ELLIPSE,
    DJV_DRAW_TEXT,
    DJV_DRAW_SELECT
} djv_draw_tool;

DJV_C_API djv_draw_tool djv_draw_get_tool(djv_models*);
DJV_C_API void djv_draw_set_tool(djv_models*, djv_draw_tool);
//! RGBA, zero to one.
DJV_C_API void djv_draw_get_color(djv_models*, float color[4]);
DJV_C_API void djv_draw_set_color(djv_models*, const float color[4]);
//! The width, in the pixels of the image.
DJV_C_API float djv_draw_get_size(djv_models*);
DJV_C_API void djv_draw_set_size(djv_models*, float);
//! Shapes held square or round, lines to forty-five degrees: a touch
//! screen has no Shift key.
DJV_C_API int djv_draw_is_constrain(djv_models*);
DJV_C_API void djv_draw_set_constrain(djv_models*, int);
//! The height of text, in the pixels of the image.
DJV_C_API float djv_draw_get_text_size(djv_models*);
DJV_C_API void djv_draw_set_text_size(djv_models*, float);

//! Open the review of a file: the review file, which need not exist yet,
//! and the media it is about. Its drawings are read into the model; a
//! review the desktop wrote keeps what this does not use. Zero, with the
//! reason in djv_review_get_error(), when it cannot be read.
DJV_C_API int djv_review_open(djv_models*, const char* reviewPath, const char* mediaPath);
//! Write it: not at all while it has no drawings and no file yet. Zero
//! on failure, with the reason.
DJV_C_API int djv_review_save(djv_models*);
DJV_C_API void djv_review_close(djv_models*);
DJV_C_API const char* djv_review_get_error(djv_models*);

//! Drawing with a finger or a pen, at a point of the view in its pixels:
//! begun on the picture of the player's frame (one when it began), and
//! kept when it ends. The eraser removes the strokes it touches. A pen
//! gives its pressure, zero to one. A cancelled stroke is not kept, and
//! what a cancelled eraser took is put back: a second finger landing
//! makes the first a pinch.
//!
//! The text tool begins text where it touched, which stays open for
//! typing after the touch ends (djv_text_*). The select tool takes the
//! stroke under the touch, or a handle of the shape selected, and moves
//! it; the move is kept when the touch ends and put back when it is
//! cancelled. A touch away from the selection takes the selection away,
//! and nothing more. Zero when the touch is not the tool's.
DJV_C_API int djv_draw_begin(djv_models*, float x, float y, int pen, float pressure);
DJV_C_API void djv_draw_move(djv_models*, float x, float y, float pressure);
DJV_C_API void djv_draw_end(djv_models*);
DJV_C_API void djv_draw_cancel(djv_models*);

//! The text being typed: whether there is any, what it says so far, as
//! it is drawn on the picture with a caret, and where it is in the
//! view's pixels (its top left, and the height of its letters), for a
//! field beside it. Finished, it is kept (unless it is empty) or
//! dropped; the selected text written again is kept as it is now, or
//! removed when it was emptied. The string stays valid until the text
//! changes.
DJV_C_API int djv_text_is_open(djv_models*);
DJV_C_API const char* djv_text_get(djv_models*);
DJV_C_API void djv_text_set(djv_models*, const char*);
DJV_C_API void djv_text_get_area(djv_models*, float* x, float* y, float* height);
DJV_C_API void djv_text_end(djv_models*, int commit);

//! The selected stroke, on the frame shown: whether there is one, its
//! place among the frame's strokes (below zero for none), and whether it
//! is text. The color and the sizes set apply to it too. Removed, or its
//! text opened for typing again (zero when it is not text), or let go.
DJV_C_API int djv_select_has(djv_models*);
DJV_C_API int djv_select_get_index(djv_models*);
DJV_C_API int djv_select_is_text(djv_models*);
DJV_C_API void djv_select_remove(djv_models*);
DJV_C_API int djv_select_edit_text(djv_models*);
DJV_C_API void djv_select_clear(djv_models*);

//! Undo and redo the drawing, and clear the frame shown.
DJV_C_API int djv_draw_has_undo(djv_models*);
DJV_C_API int djv_draw_has_redo(djv_models*);
DJV_C_API void djv_draw_undo(djv_models*);
DJV_C_API void djv_draw_redo(djv_models*);
DJV_C_API void djv_draw_clear_frame(djv_models*);

//! Clear the drawings on the player's file, every frame, as one undo
//! step.
DJV_C_API void djv_draw_clear_all(djv_models*);

//! The frames with drawings on them, in seconds from the start of the
//! player's timeline, in order; and whether the frame shown has any.
DJV_C_API size_t djv_draw_get_frame_count(djv_models*);
DJV_C_API double djv_draw_get_frame(djv_models*, size_t);

//! The frame to go to from the player's, in seconds from the start of
//! its timeline, as DJV's Previous and Next Marker do: the next or the
//! previous with a drawing, a note, or one of the markers given (in
//! seconds from the start), around from one end to the other. Below zero
//! when there is none.
DJV_C_API double djv_review_get_jump(djv_models*, const double* markers, size_t count, int next);

//! The review's notes, as DJV's review markers: in time order, those
//! about no frame in particular first. Saved in the review with the
//! drawings, and the desktop's markers. The strings stay valid until the
//! notes change.
DJV_C_API size_t djv_note_get_count(djv_models*);
DJV_C_API const char* djv_note_get_id(djv_models*, size_t);
DJV_C_API const char* djv_note_get_name(djv_models*, size_t);
DJV_C_API const char* djv_note_get_text(djv_models*, size_t);
DJV_C_API const char* djv_note_get_author(djv_models*, size_t);
//! When it was written, ISO 8601.
DJV_C_API const char* djv_note_get_created(djv_models*, size_t);
//! The frames it is about, in seconds from the start of the player's
//! timeline: a start below zero for none in particular.
DJV_C_API void djv_note_get_range(djv_models*, size_t, double* start, double* duration);
DJV_C_API void djv_note_get_color(djv_models*, size_t, float rgba[4]);

//! Add a note about the player's frame, signed with the author: its id,
//! valid until the next one is added; NULL without a review or a
//! player.
DJV_C_API const char* djv_note_add(djv_models*, const char* text);
//! Add a note about a span of frames, in seconds from the start of the
//! timeline, as the desktop's Add a Range adds one for the in and out
//! points; at least a frame. Its identifier, or NULL.
DJV_C_API const char* djv_note_add_range(
    djv_models*,
    double start,
    double duration,
    const char* text);
//! A note's text changed; who wrote it and when stay.
DJV_C_API void djv_note_set_text(djv_models*, const char* id, const char* text);
//! A note's color, as the desktop's swatch sets it.
DJV_C_API void djv_note_set_color(djv_models*, const char* id, const float rgba[4]);
DJV_C_API void djv_note_remove(djv_models*, const char* id);
//! Remove all the notes.
DJV_C_API void djv_note_clear_all(djv_models*);

//! The name the notes and drawings are signed with, kept in the
//! settings. Empty signs them with nothing on the iPad.
DJV_C_API const char* djv_review_get_author(djv_models*);

//! The file the review compares with, as "B", where it is found from the
//! review; empty for none, or none found. Compared with, the review's
//! mode, options and time are put back with djv_review_restore_compare().
//! A comparison changed is written into the review when it is saved, as
//! the desktop writes it; one not changed stays as the review has it.
DJV_C_API const char* djv_review_get_compare_file(djv_models*);

//! The names of the files a review file is about, without opening it: the
//! one it is about ("A") first, then those it compares with. Zero, with
//! the reason in djv_review_get_error(), when it cannot be read. The names
//! stay valid until the next read.
DJV_C_API size_t djv_review_read_file_names(djv_models*, const char* path);
DJV_C_API const char* djv_review_get_file_name(djv_models*, size_t);
DJV_C_API void djv_review_restore_compare(djv_models*);
DJV_C_API void djv_review_set_author(djv_models*, const char*);
DJV_C_API int djv_draw_has_frame(djv_models*);

///@}

//! \name Cache
///@{

//! The video cache of the player, in gigabytes, as DJV's settings keep
//! it; set, it is held to the most the machine can take. What a machine
//! of this size starts with, and that most.
DJV_C_API float djv_cache_get_video_gb(djv_models*);
DJV_C_API void djv_cache_set_video_gb(djv_models*, float);
DJV_C_API float djv_cache_get_default_video_gb(void);
DJV_C_API float djv_cache_get_max_gb(void);

///@}

//! \name Information
///@{

//! The information about the player's file, as DJV's Info tool shows
//! it: rows of a section (File, Video, Audio, Metadata), a name and a
//! value, those matching the search (NULL or empty for all). For a timeline,
//! the media of its first clip. Read when counted; the strings stay
//! valid until the next count.
DJV_C_API size_t djv_info_get_count(djv_models*, const char* search);
DJV_C_API const char* djv_info_get_section(djv_models*, size_t);
DJV_C_API const char* djv_info_get_name(djv_models*, size_t);
DJV_C_API const char* djv_info_get_value(djv_models*, size_t);

///@}

//! \name Stills
///@{

//! The name a still of the frame shown is given, with an extension
//! (".png"): the file's name and the time as the time units label it,
//! as DJV's Export Still names it. Valid until the next call.
DJV_C_API const char* djv_still_get_name(djv_models*, const char* ext);

//! Write a still of the frame shown, as the view shows it with its
//! drawings, into a file of the type its extension says. Zero when it
//! cannot, and djv_still_get_error() says why.
DJV_C_API int djv_still_write(djv_models*, const char* path);
DJV_C_API const char* djv_still_get_error(djv_models*);

///@}

//! \name Shortcuts
///@{

//! The keyboard's shortcuts: the desktop's, from the models' table of
//! default keys (models::getDefaultShortcuts()), with any saved ones over
//! them. An action's name, "Playback/Toggle", and its keys: a key as ftk
//! names it ("Space", "Left", "K", "0"), and its modifiers as ftk's bits
//! (shift 1, control 2, alt 4, super 8, which is the command key on
//! Apple's systems). Read when counted; the strings stay valid until the
//! next count.
DJV_C_API size_t djv_shortcut_get_count(djv_models*);
DJV_C_API const char* djv_shortcut_get_name(djv_models*, size_t);

//! A shortcut's primary key (which is 0) or its secondary (1), and its
//! modifiers; an empty string when there is none.
DJV_C_API const char* djv_shortcut_get_key(djv_models*, size_t, int which, int* modifiers);

///@}

#ifdef __cplusplus
}
#endif
