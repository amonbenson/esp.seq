#include "lpui_components/pattern_editor.h"

#include <esp_check.h>


static const char *TAG = "pattern_editor";

static const lpui_color_t lpui_color_patterns[] = { LPUI_COLOR_PATTERNS };


esp_err_t pattern_editor_init(pattern_editor_t *editor, const pattern_editor_config_t *config) {
    editor->config = *config;
    lpui_component_init(&editor->cmp, &config->cmp_config);

    editor->page = 0;
    editor->step_offset = 0;
    editor->track_id = 0;

    editor->previous.pattern = 0;
    editor->previous.page = 0;
    editor->previous.step_position = 0;

    return ESP_OK;
}


static esp_err_t _pattern_editor_draw_step(pattern_editor_t *editor,
        pattern_t *pattern,
        uint16_t step_position) {
    lpui_t *ui = editor->cmp.ui;
    lpui_position_t *cmp_pos = &editor->cmp.config.pos;
    lpui_size_t *cmp_size = &editor->cmp.config.size;

    // calculate the drawing position
    uint16_t display_position = step_position - editor->step_offset;
    uint8_t x = display_position % cmp_size->width;
    uint8_t y = display_position / cmp_size->width;
    lpui_position_t pos = {
        .x = cmp_pos->x + x,
        .y = cmp_pos->y + cmp_size->height - 1 - y
    };

    // check if the pattern is valid
    if (pattern == NULL) {
        return lpui_sysex_add_led(ui, pos, LPUI_COLOR_BLACK);
    }

    // check if the step index is valid
    if (step_position >= pattern->config.step_length) {
        return lpui_sysex_add_led(ui, pos, LPUI_COLOR_BLACK);
    }

    // get the base color
    uint8_t color_id = editor->track_id % (sizeof(lpui_color_patterns) / sizeof(lpui_color_t));
    lpui_color_t base_color = lpui_color_patterns[color_id];

    pattern_step_t *step = &pattern->steps[step_position];
    if (step_position == pattern->step_position) {
        return lpui_sysex_add_led(ui, pos, LPUI_COLOR_PLAYHEAD);
    } else if (step->atomic.velocity > 0) {
        return lpui_sysex_add_led(ui, pos, base_color);
    } else {
        return lpui_sysex_add_led(ui, pos, lpui_color_darken(base_color));
    }
}

static esp_err_t pattern_editor_draw_steps(pattern_editor_t *editor, pattern_t *pattern, uint16_t step_positions[], size_t n) {
    lpui_t *ui = editor->cmp.ui;

    // restart the sysex data stream
    ESP_RETURN_ON_ERROR(lpui_sysex_reset(ui, LPUI_SYSEX_COMMAND_SET_LEDS),
        TAG, "Failed to reset sysex buffer");

    // draw all visible steps
    for (uint16_t i = 0; i < n; i++) {
        uint16_t step_position = step_positions[i];
        ESP_RETURN_ON_ERROR(_pattern_editor_draw_step(editor, pattern, step_position),
            TAG, "Failed to draw step");
    }

    // finish and send the sysex data
    ESP_RETURN_ON_ERROR(lpui_sysex_commit(ui),
        TAG, "Failed to commit sysex buffer");

    return ESP_OK;
}

static esp_err_t pattern_editor_draw_pattern(pattern_editor_t *editor, pattern_t *pattern) {
    lpui_t *ui = editor->cmp.ui;
    lpui_size_t *size = &editor->cmp.config.size;

    // restart the sysex data stream
    ESP_RETURN_ON_ERROR(lpui_sysex_reset(ui, LPUI_SYSEX_COMMAND_SET_LEDS),
        TAG, "Failed to reset sysex buffer");

    // draw all visible steps
    for (uint16_t i = 0; i < size->width * size->height; i++) {
        uint16_t step_position = editor->step_offset + i;
        ESP_RETURN_ON_ERROR(_pattern_editor_draw_step(editor, pattern, step_position),
            TAG, "Failed to draw step");
    }

    // finish and send the sysex data
    ESP_RETURN_ON_ERROR(lpui_sysex_commit(ui),
        TAG, "Failed to commit sysex buffer");

    return ESP_OK;
}

static pattern_t *pattern_editor_get_active_pattern(pattern_editor_t *editor) {
    sequencer_t *sequencer = editor->config.sequencer;
    track_t *track = &sequencer->tracks[editor->track_id];
    return track_get_active_pattern(track);
}

esp_err_t pattern_editor_update(pattern_editor_t *editor) {
    pattern_t *pattern = pattern_editor_get_active_pattern(editor);

    // draw empty pattern
    if (pattern == NULL) {
        if (editor->previous.pattern != NULL) {
            // TODO: special empty pattern function
            ESP_RETURN_ON_ERROR(pattern_editor_draw_pattern(editor, pattern),
                TAG, "Failed to redraw pattern editor");

            editor->previous.pattern = NULL;
        }

        return ESP_OK;
    }

    // update automatic scrolling
    uint8_t page_steps = editor->cmp.config.size.width * editor->cmp.config.size.height;
    editor->page = pattern->step_position / page_steps;
    editor->step_offset = editor->page * page_steps;

    // draw complete pattern
    if (pattern != editor->previous.pattern || editor->page != editor->previous.page) {
        ESP_RETURN_ON_ERROR(pattern_editor_draw_pattern(editor, pattern),
            TAG, "Failed to redraw pattern editor");

        // store the updated pattern, page and step position
        editor->previous.pattern = pattern;
        editor->previous.page = editor->page;
        editor->previous.step_position = pattern->step_position;

        return ESP_OK;
    }

    // draw previous and new steps only
    if (pattern->step_position != editor->previous.step_position) {
        ESP_RETURN_ON_ERROR(pattern_editor_draw_steps(editor, pattern, (uint16_t[]) {
            editor->previous.step_position,
            pattern->step_position
        }, 2),
            TAG, "Failed to redraw previous step");

        // store the updated step position
        editor->previous.step_position = pattern->step_position;

        return ESP_OK;
    }

    return ESP_OK;
}

esp_err_t pattern_editor_set_track_id(pattern_editor_t *editor, int track_id) {
    // store the new track id if it changed
    if (editor->track_id == track_id) {
        return ESP_OK;
    }
    editor->track_id = track_id;

    // update and redraw the pattern editor
    return pattern_editor_update(editor);
}
