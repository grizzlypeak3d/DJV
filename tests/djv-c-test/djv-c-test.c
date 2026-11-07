/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright Contributors to the DJV project. */

/* The C interface to DJV's models (djvC), from C: what DJV Studio for the
 * iPad keeps its options and settings with. Written in C, so the header is
 * checked as C as well as what it does. The models are made on an engine
 * that may have no GPU renderer: they need none. */

#include <djv/C/Models.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(x) check((x), #x, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, e) checkNear((a), (b), (e), #a, __FILE__, __LINE__)

static void check(int value, const char* text, const char* file, int line)
{
    if (!value)
    {
        printf("FAILED: %s (%s:%d)\n", text, file, line);
        ++failures;
    }
}

static void checkNear(double a, double b, double e, const char* text, const char* file, int line)
{
    if (fabs(a - b) > e)
    {
        printf("FAILED: %s is %f, not %f (%s:%d)\n", text, a, b, file, line);
        ++failures;
    }
}

static int containsNoCase(const char* s, const char* word)
{
    const size_t n = strlen(word);
    for (; *s; ++s)
    {
        size_t i = 0;
        while (i < n && s[i] && tolower((unsigned char)s[i]) == tolower((unsigned char)word[i]))
            ++i;
        if (i == n)
            return 1;
    }
    return 0;
}

static void information(djv_models* models)
{
    const size_t count = djv_info_get_count(models, NULL);
    CHECK(count > 0);
    if (!count)
        return;
    CHECK(strcmp(djv_info_get_section(models, 0), "File") == 0);
    int name = 0;
    int resolution = 0;
    for (size_t i = 0; i < count; ++i)
    {
        const char* key = djv_info_get_name(models, i);
        const char* value = djv_info_get_value(models, i);
        name |= strcmp(key, "Name") == 0 && strstr(value, "BART_2021-02-07") != NULL;
        resolution |= strcmp(key, "Resolution") == 0 && strlen(value) > 0;
    }
    CHECK(name);
    CHECK(resolution);
    CHECK(strcmp(djv_info_get_name(models, count), "") == 0);

    /* The search: every row kept has the word in its name or value. */
    const size_t found = djv_info_get_count(models, "RESOLUTION");
    CHECK(found > 0 && found < count);
    for (size_t i = 0; i < found; ++i)
    {
        CHECK(
            containsNoCase(djv_info_get_name(models, i), "resolution") ||
            containsNoCase(djv_info_get_value(models, i), "resolution"));
    }
    CHECK(djv_info_get_count(models, "nothing-has-this") == 0);
}

static void hud(djv_models* models)
{
    CHECK(djv_hud_get_item_count() == DJV_HUD_COUNT);
    CHECK(strcmp(djv_hud_get_item_name(DJV_HUD_TIME), "Time") == 0);
    CHECK(djv_hud_get_item(0) == DJV_HUD_FILE_NAME);

    /* The desktop's corners to begin with. */
    CHECK(djv_hud_get_pos(models, DJV_HUD_FILE_NAME) == DJV_HUD_POS_TOP_LEFT);
    CHECK(djv_hud_get_pos(models, DJV_HUD_TIME) == DJV_HUD_POS_TOP_RIGHT);
    djv_hud_set_pos(models, DJV_HUD_RENDER, DJV_HUD_POS_BOTTOM_LEFT);
    CHECK(djv_hud_get_pos(models, DJV_HUD_RENDER) == DJV_HUD_POS_BOTTOM_LEFT);
    djv_hud_set_enabled(models, 1);
    CHECK(djv_hud_is_enabled(models));

    CHECK(strstr(djv_hud_get_text(models, DJV_HUD_FILE_NAME, 0.0, 0, 1.0), "BART_2021-02-07") != NULL);
    const char* time = djv_hud_get_text(models, DJV_HUD_TIME, 24.0, 3, 1.0);
    CHECK(strncmp(time, "Time: ", 6) == 0);
    CHECK(strstr(time, "3 dropped") != NULL);
    CHECK(strstr(djv_hud_get_text(models, DJV_HUD_INFO, 0.0, 0, 1.0), "V: ") != NULL);
    CHECK(strncmp(djv_hud_get_text(models, DJV_HUD_RENDER, 0.0, 0, 1.0), "Render: ", 8) == 0);
    CHECK(strlen(djv_hud_get_text(models, DJV_HUD_COLOR_PICKER, 0.0, 0, 1.0)) == 0);
}

static void timeUnits(djv_models* models)
{
    CHECK(djv_time_get_units(models) == DJV_TIME_UNITS_TIMECODE);
    CHECK(strchr(djv_time_get_label(models, 1.0), ':') != NULL);
    djv_time_set_units(models, DJV_TIME_UNITS_FRAMES);
    CHECK(djv_time_get_units(models) == DJV_TIME_UNITS_FRAMES);
    const char* frames = djv_time_get_label(models, 1.0);
    CHECK(strlen(frames) > 0 && strchr(frames, ':') == NULL);
    djv_time_set_units(models, DJV_TIME_UNITS_SECONDS);
    CHECK(djv_time_get_units(models) == DJV_TIME_UNITS_SECONDS);
}

static void viewport(djv_models* models)
{
    /* The adjustments, and the reset that leaves the view options. */
    CHECK(!djv_viewport_has_color_adjustments(models));
    djv_display_options display = djv_viewport_get_display(models);
    display.exposureEnabled = 1;
    display.exposure = 2.F;
    display.colorEnabled = 1;
    display.hue = .25F;
    display.negative = 1;
    display.channels = 1;
    djv_viewport_set_display(models, display);
    CHECK(djv_viewport_has_color_adjustments(models));
    display = djv_viewport_get_display(models);
    CHECK_NEAR(display.exposure, 2.0, 0.0001);
    CHECK_NEAR(display.hue, .25, 0.0001);
    djv_viewport_reset_color_adjustments(models, 1, 1);
    CHECK(!djv_viewport_has_color_adjustments(models));
    display = djv_viewport_get_display(models);
    CHECK(display.negative);
    CHECK(display.channels == 1);
    CHECK_NEAR(display.exposure, 0.0, 0.0001);

    /* The guides. */
    djv_foreground_options foreground = djv_viewport_get_foreground(models);
    CHECK(!foreground.frameGuideEnabled);
    CHECK(!foreground.safeAreasEnabled);
    foreground.frameGuideEnabled = 1;
    foreground.frameGuideAspect = 1.85F;
    foreground.safeAreasEnabled = 1;
    foreground.gridCellSize = 1;
    djv_viewport_set_foreground(models, foreground);
    foreground = djv_viewport_get_foreground(models);
    CHECK(foreground.frameGuideEnabled);
    CHECK_NEAR(foreground.frameGuideAspect, 1.85, 0.0001);
    CHECK(foreground.safeAreasEnabled);
    CHECK(foreground.gridCellSize == 1);

    djv_background_options background = djv_viewport_get_background(models);
    background.outlineEnabled = 1;
    background.outlineWidth = 7;
    djv_viewport_set_background(models, background);
    CHECK(djv_viewport_get_background(models).outlineWidth == 7);

    CHECK(djv_viewport_get_aspect_ratio_count(models) > 1);
}

static void settings(djv_models* models)
{
    /* The cache, held to what the machine can take. */
    CHECK(djv_cache_get_default_video_gb() > 0.F);
    CHECK(djv_cache_get_max_gb() >= djv_cache_get_default_video_gb());
    djv_cache_set_video_gb(models, 1.F);
    CHECK_NEAR(djv_cache_get_video_gb(models), 1.0, 0.0001);
    djv_cache_set_video_gb(models, 1000000.F);
    CHECK_NEAR(djv_cache_get_video_gb(models), djv_cache_get_max_gb(), 0.0001);
    djv_cache_set_video_gb(models, 1.F);

    djv_audio_set_volume(models, .5F);
    CHECK_NEAR(djv_audio_get_volume(models), .5, 0.0001);
    djv_audio_set_muted(models, 1);
    CHECK(djv_audio_is_muted(models));
    djv_audio_set_sync_offset(models, .25);
    CHECK_NEAR(djv_audio_get_sync_offset(models), .25, 0.0001);

    /* The built-in OpenColorIO configuration, where it is built in. */
    if (djv_ocio_is_available())
    {
        djv_ocio_set_enabled(models, 1);
        CHECK(djv_ocio_is_enabled(models));
        CHECK(djv_ocio_get_display_count(models) > 0);
        CHECK(djv_ocio_get_view_count(models) > 0);
        CHECK(strlen(djv_ocio_get_name(models)) > 0);
    }
}

/* The keyboard's shortcuts, the desktop's defaults: Space plays and
   stops, and an action without a key has none. */
static void shortcuts(djv_models* models)
{
    /* Volume Up and Volume Down, as DJV steps them. */
    djv_audio_set_volume(models, .5F);
    djv_audio_volume_up(models);
    CHECK_NEAR(djv_audio_get_volume(models), .51, 0.0001);
    djv_audio_volume_down(models);
    djv_audio_volume_down(models);
    CHECK_NEAR(djv_audio_get_volume(models), .49, 0.0001);
    djv_audio_set_volume(models, 1.F);

    const size_t count = djv_shortcut_get_count(models);
    CHECK(count > 0);
    int toggle = 0;
    int redo = 0;
    for (size_t i = 0; i < count; ++i)
    {
        const char* name = djv_shortcut_get_name(models, i);
        int modifiers = -1;
        const char* key = djv_shortcut_get_key(models, i, 0, &modifiers);
        if (strcmp(name, "Playback/Toggle") == 0)
        {
            toggle = 1;
            CHECK(strcmp(key, "Space") == 0);
            CHECK(modifiers == 0);
            CHECK(strlen(djv_shortcut_get_key(models, i, 1, &modifiers)) == 0);
        }
        else if (strcmp(name, "Review/Redo") == 0)
        {
            redo = 1;
            CHECK(strcmp(key, "Z") == 0);
            /* Shift and the command key. */
            CHECK(modifiers & 1);
        }
    }
    CHECK(toggle);
    CHECK(redo);
    CHECK(strlen(djv_shortcut_get_name(models, count)) == 0);
}

/* A still of the frame shown: named after the file and the time, and
   written where there is a renderer to draw it with. */
static void stills(tl_engine* engine, djv_models* models)
{
    const char* name = djv_still_get_name(models, ".png");
    CHECK(strncmp(name, "BART_2021-02-07_", 16) == 0);
    CHECK(strlen(name) > 20 && strcmp(name + strlen(name) - 4, ".png") == 0);
    if (tl_engine_has_gpu(engine))
    {
        /* Written with the name given: one ending in a number, as the
         * time does, is not taken for a frame of a sequence. */
        const char* path = DJV_C_TEST_SETTINGS "-still_01-00-00-03.png";
        remove(path);
        CHECK(djv_still_write(models, path));
        CHECK(strlen(djv_still_get_error(models)) == 0);
        FILE* f = fopen(path, "rb");
        CHECK(f != NULL);
        if (f)
        {
            fseek(f, 0, SEEK_END);
            CHECK(ftell(f) > 1000);
            fclose(f);
        }
        remove(path);
    }
    /* A type nothing writes says why. */
    CHECK(!djv_still_write(models, DJV_C_TEST_SETTINGS "-still.unknown"));
    CHECK(strlen(djv_still_get_error(models)) > 0);
}

static void writeFile(const char* path, const char* text)
{
    FILE* f = fopen(path, "w");
    CHECK(f != NULL);
    if (f)
    {
        fputs(text, f);
        fclose(f);
    }
}

static int fileContains(const char* path, const char* text)
{
    static char buf[65536];
    FILE* f = fopen(path, "r");
    if (!f)
        return 0;
    const size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = 0;
    return strstr(buf, text) != NULL;
}

/* A review as the desktop writes one: the file, a drawing on its first
 * frame, and a section the iPad does not know, which a save keeps. */
static const char* reviewText =
    "{\"djvReview\": 1, \"app\": \"DJV\", \"created\": \"2026-11-02T00:00:00Z\","
    " \"files\": [{\"id\": \"f1\", \"path\": \"BART_2021-02-07.0000.jpg\"}],"
    " \"annotations\": [{\"id\": \"a1\", \"sourceId\": \"f1\", \"space\": \"image\","
    " \"time\": {\"value\": 0.0, \"rate\": 24.0}, \"author\": \"desk\", \"strokes\": ["
    " {\"color\": [1.0, 0.0, 0.0, 1.0], \"width\": 4.0, \"widthSpace\": \"image\","
    " \"points\": [10.0, 10.0, 20.0, 20.0]}]}],"
    " \"elsewhere\": {\"kept\": true}}";

/* Whether a space-separated list has a word. */
static int hasLUTWord(const char* list, const char* word)
{
    const size_t n = strlen(word);
    for (const char* p = list; (p = strstr(p, word)); p += n)
    {
        if ((p == list || ' ' == p[-1]) && (' ' == p[n] || 0 == p[n]))
            return 1;
    }
    return 0;
}

/* The LUT: its file, order and direction, kept in the settings; and the
 * extensions a LUT can have. */
static void luts(djv_models* models)
{
    djv_lut_set_file_name(models, "/tmp/look.cube");
    CHECK(strcmp(djv_lut_get_file_name(models), "/tmp/look.cube") == 0);
    djv_lut_set_enabled(models, 1);
    CHECK(djv_lut_is_enabled(models));
    djv_lut_set_order(models, DJV_LUT_ORDER_PRE_CONFIG);
    CHECK(djv_lut_get_order(models) == DJV_LUT_ORDER_PRE_CONFIG);
    djv_lut_set_direction(models, DJV_LUT_DIRECTION_INVERSE);
    CHECK(djv_lut_get_direction(models) == DJV_LUT_DIRECTION_INVERSE);
    djv_lut_set_enabled(models, 0);
    djv_lut_set_order(models, DJV_LUT_ORDER_POST_CONFIG);
    djv_lut_set_direction(models, DJV_LUT_DIRECTION_FORWARD);
    djv_lut_set_file_name(models, "");
    CHECK(!djv_lut_is_enabled(models));

    const char* exts = djv_lut_get_extensions();
    printf("LUT extensions: %s\n", exts);
    if (djv_ocio_is_available())
    {
        CHECK(hasLUTWord(exts, "cube"));
        CHECK(hasLUTWord(exts, "3dl"));
        CHECK(!strchr(exts, '.'));
    }
    CHECK(djv_lut_get_extensions() == exts);
}

static void drawing(tl_engine* engine, tl_player* player)
{
    djv_models* models = djv_models_create(engine, DJV_C_TEST_SETTINGS);
    CHECK(models != NULL);
    if (!models)
        return;

    /* The tool, kept in the settings. */
    djv_draw_set_tool(models, DJV_DRAW_ARROW);
    CHECK(djv_draw_get_tool(models) == DJV_DRAW_ARROW);
    const float red[4] = { 1.F, 0.F, 0.F, 1.F };
    djv_draw_set_color(models, red);
    float color[4] = { 0.F, 0.F, 0.F, 0.F };
    djv_draw_get_color(models, color);
    CHECK_NEAR(color[0], 1.0, 0.0001);
    CHECK_NEAR(color[1], 0.0, 0.0001);
    djv_draw_set_size(models, 9.F);
    CHECK_NEAR(djv_draw_get_size(models), 9.0, 0.0001);
    djv_draw_set_constrain(models, 1);
    CHECK(djv_draw_is_constrain(models));
    djv_draw_set_tool(models, DJV_DRAW_PEN);
    /* Written by a save while the models are still there: an iPad's
       application is stopped, never quit, so they never go. */
    djv_draw_set_text_size(models, 72.F);
    djv_models_save(models);
    CHECK(fileContains(DJV_C_TEST_SETTINGS, "\"Draw\""));
    CHECK(fileContains(DJV_C_TEST_SETTINGS, "\"TextSize\": 72"));

    /* The desktop's review of the file. */
    writeFile(DJV_C_TEST_DIR "/review.djvr", reviewText);
    CHECK(djv_review_open(models, DJV_C_TEST_DIR "/review.djvr", DJV_SAMPLE_DATA "/BART_2021-02-07.0000.jpg") == 1);
    djv_models_set_player(models, player);
    CHECK(djv_draw_get_frame_count(models) == 1);
    CHECK_NEAR(djv_draw_get_frame(models, 0), 0.0, 0.0001);
    CHECK(djv_draw_has_frame(models));

    /* The drawing's frame is the only one to go to, either way; with
       nothing else, nowhere. */
    CHECK_NEAR(djv_review_get_jump(models, NULL, 0, 1), 0.0, 0.0001);
    CHECK_NEAR(djv_review_get_jump(models, NULL, 0, 0), 0.0, 0.0001);
    const double markers[1] = { 0.0 };
    CHECK_NEAR(djv_review_get_jump(models, markers, 1, 1), 0.0, 0.0001);

    /* No view, so nowhere to draw, to type or to select. */
    CHECK(djv_draw_begin(models, 10.F, 10.F, 0, 0.F) == 0);
    djv_draw_set_tool(models, DJV_DRAW_TEXT);
    CHECK(djv_draw_begin(models, 10.F, 10.F, 0, 0.F) == 0);
    CHECK(!djv_text_is_open(models));
    CHECK(strcmp(djv_text_get(models), "") == 0);
    djv_text_set(models, "ignored");
    djv_text_end(models, 1);
    CHECK(djv_draw_get_frame_count(models) == 1);
    djv_draw_set_tool(models, DJV_DRAW_SELECT);
    CHECK(djv_draw_begin(models, 10.F, 10.F, 0, 0.F) == 0);
    CHECK(!djv_select_has(models));
    CHECK(djv_select_get_index(models) < 0);
    CHECK(!djv_select_is_text(models));
    CHECK(!djv_select_edit_text(models));
    djv_select_remove(models);
    CHECK(djv_draw_has_frame(models));
    djv_draw_set_tool(models, DJV_DRAW_PEN);

    /* The text's size, kept in the settings. */
    djv_draw_set_text_size(models, 64.F);
    CHECK_NEAR(djv_draw_get_text_size(models), 64.0, 0.0001);

    /* Cleared, and back again. */
    CHECK(!djv_draw_has_undo(models));
    djv_draw_clear_frame(models);
    CHECK(!djv_draw_has_frame(models));
    CHECK(djv_draw_get_frame_count(models) == 0);
    CHECK(djv_review_get_jump(models, NULL, 0, 1) < 0.0);
    CHECK(djv_draw_has_undo(models));
    djv_draw_undo(models);
    CHECK(djv_draw_has_frame(models));
    CHECK(djv_draw_has_redo(models));
    djv_draw_redo(models);
    CHECK(!djv_draw_has_frame(models));
    djv_draw_undo(models);
    CHECK(djv_draw_has_frame(models));

    /* A note on the frame, signed with the author; its text changed, who
       wrote it kept; another added and taken away. */
    djv_review_set_author(models, "Pad");
    CHECK(strcmp(djv_review_get_author(models), "Pad") == 0);
    CHECK(djv_note_get_count(models) == 0);
    const char* id = djv_note_add(models, "Too dark");
    CHECK(id != NULL);
    char noteId[64] = { 0 };
    if (id)
        strncpy(noteId, id, sizeof(noteId) - 1);
    CHECK(djv_note_get_count(models) == 1);
    CHECK(strcmp(djv_note_get_text(models, 0), "Too dark") == 0);
    CHECK(strcmp(djv_note_get_author(models, 0), "Pad") == 0);
    CHECK(strlen(djv_note_get_created(models, 0)) > 0);
    double start = -1.0, duration = 0.0;
    djv_note_get_range(models, 0, &start, &duration);
    CHECK_NEAR(start, 0.0, 0.0001);
    CHECK(duration > 0.0);
    djv_note_set_text(models, noteId, "Much too dark");
    CHECK(strcmp(djv_note_get_text(models, 0), "Much too dark") == 0);
    CHECK(strcmp(djv_note_get_author(models, 0), "Pad") == 0);
    id = djv_note_add(models, "Gone");
    CHECK(djv_note_get_count(models) == 2);
    if (id)
    {
        char gone[64] = { 0 };
        strncpy(gone, id, sizeof(gone) - 1);
        djv_note_remove(models, gone);
    }
    CHECK(djv_note_get_count(models) == 1);

    /* A note about a span of frames, in a color of its own: after the one
       about the first frame, in time order. */
    id = djv_note_add_range(models, 1.0, 0.5, "This runs long");
    CHECK(id != NULL);
    CHECK(djv_note_get_count(models) == 2);
    if (id)
    {
        char range[64] = { 0 };
        strncpy(range, id, sizeof(range) - 1);
        const float green[4] = { 0.2F, 0.85F, 0.3F, 1.F };
        djv_note_set_color(models, range, green);
        CHECK(strcmp(djv_note_get_text(models, 1), "This runs long") == 0);
        djv_note_get_range(models, 1, &start, &duration);
        CHECK_NEAR(start, 1.0, 0.05);
        CHECK_NEAR(duration, 0.5, 0.05);
        float rgba[4] = { 0.F, 0.F, 0.F, 0.F };
        djv_note_get_color(models, 1, rgba);
        CHECK_NEAR(rgba[1], 0.85, 0.0001);
        djv_note_remove(models, range);
    }
    /* At least a frame. */
    id = djv_note_add_range(models, 0.0, 0.0, "A frame");
    CHECK(id != NULL);
    djv_note_get_range(models, 1, &start, &duration);
    CHECK(duration > 0.0);
    if (id)
    {
        char frame[64] = { 0 };
        strncpy(frame, id, sizeof(frame) - 1);
        djv_note_remove(models, frame);
    }
    CHECK(djv_note_get_count(models) == 1);

    /* Saved, keeping what the iPad does not read, and read again. */
    CHECK(djv_review_save(models) == 1);
    CHECK(fileContains(DJV_C_TEST_DIR "/review.djvr", "\"elsewhere\""));
    CHECK(fileContains(DJV_C_TEST_DIR "/review.djvr", "Much too dark"));
    djv_review_close(models);
    CHECK(djv_draw_get_frame_count(models) == 0);
    CHECK(djv_note_get_count(models) == 0);
    CHECK(djv_review_open(models, DJV_C_TEST_DIR "/review.djvr", DJV_SAMPLE_DATA "/BART_2021-02-07.0000.jpg") == 1);
    djv_models_set_player(models, player);
    CHECK(djv_draw_get_frame_count(models) == 1);
    CHECK(djv_note_get_count(models) == 1);
    CHECK(strcmp(djv_note_get_text(models, 0), "Much too dark") == 0);
    CHECK(strcmp(djv_note_get_author(models, 0), "Pad") == 0);

    /* All the drawings cleared, the notes kept, and back with an undo;
       then all the notes. */
    djv_draw_clear_all(models);
    CHECK(djv_draw_get_frame_count(models) == 0);
    CHECK(djv_note_get_count(models) == 1);
    djv_draw_undo(models);
    CHECK(djv_draw_get_frame_count(models) == 1);
    djv_note_clear_all(models);
    CHECK(djv_note_get_count(models) == 0);

    /* A new review is not written while it has nothing in it. */
    CHECK(djv_review_open(models, DJV_C_TEST_DIR "/empty.djvr", DJV_SAMPLE_DATA "/BART_2021-02-07.0000.jpg") == 1);
    CHECK(djv_draw_get_frame_count(models) == 0);
    CHECK(djv_review_save(models) == 1);
    CHECK(fopen(DJV_C_TEST_DIR "/empty.djvr", "r") == NULL);

    /* One with only a note is. */
    remove(DJV_C_TEST_DIR "/note.djvr");
    CHECK(djv_review_open(models, DJV_C_TEST_DIR "/note.djvr", DJV_SAMPLE_DATA "/BART_2021-02-07.0000.jpg") == 1);
    CHECK(djv_note_add(models, "Only this") != NULL);
    CHECK(djv_review_save(models) == 1);
    CHECK(fileContains(DJV_C_TEST_DIR "/note.djvr", "Only this"));
    djv_review_set_author(models, "");

    /* What cannot be read says why. */
    writeFile(DJV_C_TEST_DIR "/bad.djvr", "{ not a review");
    CHECK(djv_review_open(models, DJV_C_TEST_DIR "/bad.djvr", DJV_SAMPLE_DATA "/BART_2021-02-07.0000.jpg") == 0);
    CHECK(strlen(djv_review_get_error(models)) > 0);

    djv_models_set_player(models, NULL);
    djv_models_destroy(models);
}

/* Comparing with another file, as the files model has it. */
static void comparing(djv_models* models, tl_player* player)
{
    CHECK(strcmp(djv_compare_get_label(DJV_COMPARE_WIPE), "Wipe") == 0);
    CHECK(strcmp(djv_compare_get_label(DJV_COMPARE_COUNT), "") == 0);
    CHECK(strcmp(djv_compare_get_time_label(1), "Timecode") == 0);

    /* Nothing to compare with: no comparison. */
    CHECK(!djv_compare_has_file(models));
    djv_compare_set_mode(models, DJV_COMPARE_WIPE);
    CHECK(djv_compare_get_mode(models) == DJV_COMPARE_NONE);

    CHECK(djv_compare_set_file(models, DJV_SAMPLE_DATA "/BART_2021-02-07.0001.jpg") == 1);
    CHECK(djv_compare_has_file(models));
    djv_compare_set_mode(models, DJV_COMPARE_WIPE);
    CHECK(djv_compare_get_mode(models) == DJV_COMPARE_WIPE);

    /* No view to drag it in. */
    CHECK(djv_compare_wipe_begin(models, 10.F, 10.F) == 0);
    CHECK(djv_compare_wipe_hit(models, 10.F, 10.F) == 0);

    /* One frame to shuttle through: always at it. */
    CHECK_NEAR(djv_shuttle_get_time(models, 0.0, 0.F), 0.0, 0.0001);
    CHECK_NEAR(djv_shuttle_get_time(models, 0.0, 200.F), 0.0, 0.0001);
    djv_compare_wipe_move(models, 20.F, 20.F);
    djv_compare_wipe_end(models);

    djv_compare_set_wipe_rotation(models, 45.F);
    float x = 0.F, y = 0.F, rotation = 0.F;
    djv_compare_get_wipe(models, &x, &y, &rotation);
    CHECK_NEAR(rotation, 45.0, 0.0001);
    djv_compare_set_overlay(models, 2.F);
    CHECK_NEAR(djv_compare_get_overlay(models), 1.0, 0.0001);
    djv_compare_set_difference_gain(models, 4.F);
    CHECK_NEAR(djv_compare_get_difference_gain(models), 4.0, 0.0001);
    djv_compare_set_absolute_time(models, 1);
    CHECK(djv_compare_is_absolute_time(models));

    /* A file that cannot be opened leaves the comparison as it was. */
    CHECK(djv_compare_set_file(models, DJV_SAMPLE_DATA "/nothing-here.mov") == 0);
    CHECK(djv_compare_has_file(models));
    CHECK(djv_compare_get_mode(models) == DJV_COMPARE_WIPE);

    /* None, and a new player, turn it off. */
    CHECK(djv_compare_set_file(models, NULL) == 1);
    CHECK(!djv_compare_has_file(models));
    CHECK(djv_compare_get_mode(models) == DJV_COMPARE_NONE);
    CHECK(djv_compare_set_file(models, DJV_SAMPLE_DATA "/BART_2021-02-07.0001.jpg") == 1);
    djv_compare_set_mode(models, DJV_COMPARE_B);
    djv_models_set_player(models, player);
    CHECK(!djv_compare_has_file(models));
    CHECK(djv_compare_get_mode(models) == DJV_COMPARE_NONE);
}

/* What was set is there again when the models are made from the file. */
/* A review whose comparison is with a file that is not there. */
static const char* reviewMissingB =
    "{\"djvReview\": 1, \"app\": \"DJV\", \"created\": \"2026-11-03T00:00:00Z\","
    " \"files\": [{\"id\": \"f1\", \"path\": \"BART_2021-02-07.0000.jpg\"},"
    " {\"id\": \"f2\", \"path\": \"NotThere.jpg\"}],"
    " \"compare\": {\"aId\": \"f1\", \"bIds\": [\"f2\"], \"options\": {\"Compare\": \"Wipe\","
    " \"DifferenceGain\": 1.0, \"Overlay\": 0.5, \"SameSize\": true,"
    " \"WipeCenter\": [0.5, 0.5], \"WipeRotation\": 0.0}, \"time\": \"Relative\"}}";

static void reviewComparing(tl_engine* engine, tl_player* player)
{
    djv_models* models = djv_models_create(engine, DJV_C_TEST_SETTINGS);
    CHECK(models != NULL);
    if (!models)
        return;
    djv_models_set_player(models, player);

    /* Compared, and saved as the desktop saves it: "B" one of the files. */
    remove(DJV_C_TEST_DIR "/compare.djvr");
    CHECK(djv_review_open(models, DJV_C_TEST_DIR "/compare.djvr", DJV_SAMPLE_DATA "/BART_2021-02-07.0000.jpg") == 1);
    CHECK(strlen(djv_review_get_compare_file(models)) == 0);
    CHECK(djv_compare_set_file(models, DJV_SAMPLE_DATA "/BART_2021-02-07.0001.jpg") == 1);
    djv_compare_set_mode(models, DJV_COMPARE_WIPE);
    djv_compare_set_wipe_rotation(models, 30.F);
    CHECK(djv_review_save(models) == 1);
    CHECK(fileContains(DJV_C_TEST_DIR "/compare.djvr", "BART_2021-02-07.0001.jpg"));
    CHECK(fileContains(DJV_C_TEST_DIR "/compare.djvr", "\"Wipe\""));

    /* Opened again: "B" found, and the comparison put back. */
    djv_models_set_player(models, player);
    CHECK(djv_compare_get_mode(models) == DJV_COMPARE_NONE);
    CHECK(djv_review_open(models, DJV_C_TEST_DIR "/compare.djvr", DJV_SAMPLE_DATA "/BART_2021-02-07.0000.jpg") == 1);
    const char* b = djv_review_get_compare_file(models);
    CHECK(strstr(b, "BART_2021-02-07.0001.jpg") != NULL);
    CHECK(djv_compare_set_file(models, b) == 1);
    djv_review_restore_compare(models);
    CHECK(djv_compare_get_mode(models) == DJV_COMPARE_WIPE);
    float x = 0.F, y = 0.F, rotation = 0.F;
    djv_compare_get_wipe(models, &x, &y, &rotation);
    CHECK_NEAR(rotation, 30.0, 0.0001);

    /* What the review is about, read without opening it: "A" first. */
    CHECK(djv_review_read_file_names(models, DJV_C_TEST_DIR "/compare.djvr") == 2);
    CHECK(strcmp(djv_review_get_file_name(models, 0), "BART_2021-02-07.0000.jpg") == 0);
    CHECK(strcmp(djv_review_get_file_name(models, 1), "BART_2021-02-07.0001.jpg") == 0);
    CHECK(djv_review_read_file_names(models, DJV_C_TEST_DIR "/NotThere.djvr") == 0);

    /* Turned off: "B" out of the files too. */
    CHECK(djv_compare_set_file(models, NULL) == 1);
    CHECK(djv_review_save(models) == 1);
    CHECK(!fileContains(DJV_C_TEST_DIR "/compare.djvr", "BART_2021-02-07.0001.jpg"));

    /* A comparison the iPad cannot show stays as the review has it. */
    writeFile(DJV_C_TEST_DIR "/missing.djvr", reviewMissingB);
    djv_models_set_player(models, player);
    CHECK(djv_review_open(models, DJV_C_TEST_DIR "/missing.djvr", DJV_SAMPLE_DATA "/BART_2021-02-07.0000.jpg") == 1);
    CHECK(strlen(djv_review_get_compare_file(models)) == 0);
    CHECK(djv_note_add(models, "Kept") != NULL);
    CHECK(djv_review_save(models) == 1);
    CHECK(fileContains(DJV_C_TEST_DIR "/missing.djvr", "NotThere.jpg"));
    CHECK(fileContains(DJV_C_TEST_DIR "/missing.djvr", "\"bIds\": ["));

    djv_review_close(models);
    djv_models_set_player(models, NULL);
    djv_models_destroy(models);
}

static void persistence(tl_engine* engine)
{
    djv_models* models = djv_models_create(engine, DJV_C_TEST_SETTINGS);
    CHECK(models != NULL);
    if (!models)
        return;
    CHECK(djv_hud_is_enabled(models));
    CHECK(djv_hud_get_pos(models, DJV_HUD_RENDER) == DJV_HUD_POS_BOTTOM_LEFT);
    CHECK(djv_time_get_units(models) == DJV_TIME_UNITS_SECONDS);
    CHECK_NEAR(djv_cache_get_video_gb(models), 1.0, 0.0001);
    CHECK(djv_audio_is_muted(models));
    CHECK(djv_viewport_get_foreground(models).frameGuideEnabled);
    CHECK(djv_viewport_get_background(models).outlineWidth == 7);
    CHECK(djv_viewport_get_display(models).negative);
    float rotation = 0.F;
    djv_compare_get_wipe(models, NULL, NULL, &rotation);
    CHECK_NEAR(rotation, 45.0, 0.0001);
    CHECK(djv_compare_is_absolute_time(models));
    djv_models_destroy(models);
}

int main(void)
{
    tl_engine* engine = tl_engine_create(NULL);
    CHECK(engine != NULL);
    if (!engine)
        return 1;

    djv_models* models = djv_models_create(engine, DJV_C_TEST_SETTINGS);
    CHECK(models != NULL);
    if (!models)
        return 1;

    /* Nothing open: nothing to say, nowhere to go, and the shuttle stays
       put. */
    CHECK(djv_review_get_jump(models, NULL, 0, 1) < 0.0);
    CHECK_NEAR(djv_shuttle_get_time(models, 1.5, 100.F), 1.5, 0.0001);
    CHECK(djv_info_get_count(models, NULL) == 0);
    CHECK(strcmp(djv_hud_get_text(models, DJV_HUD_FILE_NAME, 0.0, 0, 1.0), "(No file)") == 0);
    CHECK(strlen(djv_time_get_label(models, 0.0)) == 0);

    tl_player* player = tl_player_create(engine, DJV_SAMPLE_DATA "/BART_2021-02-07.0000.jpg");
    CHECK(player != NULL);
    if (player)
    {
        const uint64_t generation = djv_models_get_generation(models);
        djv_models_set_player(models, player);
        CHECK(djv_models_get_generation(models) != generation);
        information(models);
        hud(models);
        timeUnits(models);
        stills(engine, models);
    }
    /* No views here: binding none, or none for the second display,
       changes nothing. */
    djv_models_bind_view(models, NULL);
    djv_models_bind_output(models, NULL);
    viewport(models);
    luts(models);
    settings(models);
    shortcuts(models);
    if (player)
    {
        drawing(engine, player);
        reviewComparing(engine, player);
        comparing(models, player);
    }

    djv_models_set_player(models, NULL);
    CHECK(djv_info_get_count(models, NULL) == 0);
    djv_models_save(models);
    djv_models_destroy(models);
    if (player)
    {
        tl_player_destroy(player);
    }

    persistence(engine);

    tl_engine_destroy(engine);
    printf("%s: %d failed\n", failures ? "FAILED" : "Passed", failures);
    return failures ? 1 : 0;
}
