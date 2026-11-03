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
    char buf[8192];
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

    /* The desktop's review of the file. */
    writeFile(DJV_C_TEST_DIR "/review.djvr", reviewText);
    CHECK(djv_review_open(models, DJV_C_TEST_DIR "/review.djvr", DJV_SAMPLE_DATA "/BART_2021-02-07.0000.jpg") == 1);
    djv_models_set_player(models, player);
    CHECK(djv_draw_get_frame_count(models) == 1);
    CHECK_NEAR(djv_draw_get_frame(models, 0), 0.0, 0.0001);
    CHECK(djv_draw_has_frame(models));

    /* No view, so nowhere to draw. */
    CHECK(djv_draw_begin(models, 10.F, 10.F, 0, 0.F) == 0);

    /* Cleared, and back again. */
    CHECK(!djv_draw_has_undo(models));
    djv_draw_clear_frame(models);
    CHECK(!djv_draw_has_frame(models));
    CHECK(djv_draw_get_frame_count(models) == 0);
    CHECK(djv_draw_has_undo(models));
    djv_draw_undo(models);
    CHECK(djv_draw_has_frame(models));
    CHECK(djv_draw_has_redo(models));
    djv_draw_redo(models);
    CHECK(!djv_draw_has_frame(models));
    djv_draw_undo(models);
    CHECK(djv_draw_has_frame(models));

    /* Saved, keeping what the iPad does not read, and read again. */
    CHECK(djv_review_save(models) == 1);
    CHECK(fileContains(DJV_C_TEST_DIR "/review.djvr", "\"elsewhere\""));
    djv_review_close(models);
    CHECK(djv_draw_get_frame_count(models) == 0);
    CHECK(djv_review_open(models, DJV_C_TEST_DIR "/review.djvr", DJV_SAMPLE_DATA "/BART_2021-02-07.0000.jpg") == 1);
    djv_models_set_player(models, player);
    CHECK(djv_draw_get_frame_count(models) == 1);

    /* A new review is not written while it has nothing in it. */
    CHECK(djv_review_open(models, DJV_C_TEST_DIR "/empty.djvr", DJV_SAMPLE_DATA "/BART_2021-02-07.0000.jpg") == 1);
    CHECK(djv_draw_get_frame_count(models) == 0);
    CHECK(djv_review_save(models) == 1);
    CHECK(fopen(DJV_C_TEST_DIR "/empty.djvr", "r") == NULL);

    /* What cannot be read says why. */
    writeFile(DJV_C_TEST_DIR "/bad.djvr", "{ not a review");
    CHECK(djv_review_open(models, DJV_C_TEST_DIR "/bad.djvr", DJV_SAMPLE_DATA "/BART_2021-02-07.0000.jpg") == 0);
    CHECK(strlen(djv_review_get_error(models)) > 0);

    djv_models_set_player(models, NULL);
    djv_models_destroy(models);
}

/* What was set is there again when the models are made from the file. */
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
    djv_models_destroy(models);
}

int main(void)
{
    tl_engine* engine = tl_engine_create();
    CHECK(engine != NULL);
    if (!engine)
        return 1;

    djv_models* models = djv_models_create(engine, DJV_C_TEST_SETTINGS);
    CHECK(models != NULL);
    if (!models)
        return 1;

    /* Nothing open: nothing to say. */
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
    }
    viewport(models);
    settings(models);
    if (player)
    {
        drawing(engine, player);
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
