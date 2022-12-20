#include "lpui_components/pattern_editor.h"

#include <esp_check.h>


static const char *TAG = "lpui_pattern_editor";

static const lpui_color_t lpui_color_patterns[] = { LPUI_COLOR_PATTERNS };


static lpui_color_t lpui_pattern_editor_get_step_color(lpui_pattern_editor_t *editor, uint16_t step_index) {
    // check if the pattern is valid
    if (editor->pattern == NULL) {
        return LPUI_COLOR_BLACK;
    }
    pattern_t *pattern = editor->pattern;

    // check if the step index is valid
    if (step_index >= pattern->config.step_length) {
        return LPUI_COLOR_BLACK;
    }

    // get the base color
    uint8_t color_id = editor->track_id % (sizeof(lpui_color_patterns) / sizeof(lpui_color_t));
    lpui_color_t base_color = lpui_color_patterns[color_id];

    pattern_step_t *step = &pattern->steps[step_index];
    if (step_index == pattern->step_position) {
        return LPUI_COLOR_PLAYHEAD;
    } else if (step->atomic.velocity > 0) {
        return base_color;
    } else {
        return lpui_color_darken(base_color);
    }
}

static esp_err_t lpui_pattern_editor_draw(lpui_t *ui, lpui_pattern_editor_t *editor) {
    lpui_position_t *pos = &editor->cmp.pos;
    lpui_size_t *size = &editor->cmp.size;
    uint8_t page_steps = size->width * size->height;
    uint8_t step_offset = editor->page * page_steps;

    ESP_RETURN_ON_ERROR(lpui_sysex_reset(ui, LPUI_SYSEX_COMMAND_SET_LEDS),
        TAG, "Failed to reset sysex buffer");

    lpui_position_t p;
    for (p.y = 0; p.y < size->height; p.y++) {
        for (p.x = 0; p.x < size->width; p.x++) {
            uint8_t step_index = p.y * size->width + p.x + step_offset;
            lpui_color_t color = lpui_pattern_editor_get_step_color(editor, step_index);

            ESP_RETURN_ON_ERROR(lpui_sysex_add_led(ui, (const lpui_position_t) {
                .x = pos->x + p.x,
                .y = pos->y + size->height - 1 - p.y
            }, color),
                TAG, "Failed to add led to sysex buffer");
        }
    }

    ESP_RETURN_ON_ERROR(lpui_sysex_commit(ui),
        TAG, "Failed to commit sysex buffer");

    return ESP_OK;
}

esp_err_t lpui_pattern_editor_set_pattern(lpui_pattern_editor_t *editor, pattern_t *pattern) {
    // check if pattern is available
    if (pattern == NULL) {
        if (editor->pattern != NULL) {
            editor->pattern = NULL;
            ESP_RETURN_ON_ERROR(lpui_pattern_editor_draw(editor->cmp.ui, editor),
                TAG, "Failed to redraw pattern editor");
        }

        return ESP_OK;
    }

    // return if the pattern step has not changed
    if (editor->pattern == pattern && editor->step_position == pattern->step_position) {
        return ESP_OK;
    }

    // store the new pattern and step position
    editor->pattern = pattern;
    editor->step_position = pattern->step_position;

    // automatic scrolling
    uint8_t page_steps = editor->cmp.size.width * editor->cmp.size.height;
    editor->page = pattern->step_position / page_steps;

    // TODO: update required parts only
    ESP_RETURN_ON_ERROR(lpui_pattern_editor_draw(editor->cmp.ui, editor),
        TAG, "Failed to redraw pattern editor");

    return ESP_OK;
}

esp_err_t lpui_pattern_editor_set_track_id(lpui_pattern_editor_t *editor, int track_id) {
    // redraw if the id changed on an active pattern
    if (editor->pattern && editor->track_id != track_id) {
        ESP_RETURN_ON_ERROR(lpui_pattern_editor_draw(editor->cmp.ui, editor),
            TAG, "Failed to redraw pattern editor");
    }

    // store the new track id
    editor->track_id = track_id;

    return ESP_OK;
}
