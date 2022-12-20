#include "lpui.h"

#include <stdlib.h>
#include <string.h>
#include <esp_check.h>


static const char *TAG = "lpui";


// set leds rgb command
static const uint8_t lpui_sysex_header[] = { 0xF0, 0x00, 0x20, 0x29, 0x02, 0x10, 0x0B };
static const lpui_color_t lpui_color_patterns[] = { LPUI_COLOR_PATTERNS };

static const int8_t lpui_piano_editor_note_map[2][8] = {
    { 0, 2, 4, 5, 7, 9, 11, 12 },
    { -1, 1, 3, -1, 6, 8, 10, -1 }
};


esp_err_t lpui_init(lpui_t *ui, const lpui_config_t *config) {
    // store the config
    ui->config = *config;

    // allocate the sysex buffer
    ui->buffer = calloc(LPUI_SYSEX_BUFFER_SIZE, sizeof(uint8_t));
    if (ui->buffer == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // store sysex header
    memcpy(ui->buffer, lpui_sysex_header, sizeof(lpui_sysex_header));

    ui->buffer_ptr = ui->buffer;
    return ESP_OK;
}

esp_err_t lpui_free(lpui_t *ui) {
    free(ui->buffer);
    return ESP_OK;
}


static bool lpui_sysex_buffer_has_space(lpui_t *ui, size_t size) {
    return ui->buffer_ptr + size < ui->buffer + LPUI_SYSEX_BUFFER_SIZE;
}

static esp_err_t lpui_sysex_start(lpui_t *ui) {
    // reset the buffer pointer right after the header
    ui->buffer_ptr = ui->buffer + sizeof(lpui_sysex_header);

    return ESP_OK;
}

static esp_err_t lpui_sysex_add_led(lpui_t *ui, lpui_position_t pos, const lpui_color_t color) {
    // validate the buffer size
    ESP_RETURN_ON_FALSE(lpui_sysex_buffer_has_space(ui, 4), ESP_ERR_NO_MEM,
        TAG, "not enough space in sysex buffer");

    // write the position
    *ui->buffer_ptr++ = pos.x + pos.y * 10;

    // write rgb components
    *ui->buffer_ptr++ = color.red;
    *ui->buffer_ptr++ = color.green;
    *ui->buffer_ptr++ = color.blue;

    return ESP_OK;
}

static esp_err_t lpui_sysex_commit(lpui_t *ui) {
    // validate the buffer size
    ESP_RETURN_ON_FALSE(lpui_sysex_buffer_has_space(ui, 1), ESP_ERR_NO_MEM,
        TAG, "not enough space in sysex buffer");

    *ui->buffer_ptr++ = 0xF7;
    size_t length = ui->buffer_ptr - ui->buffer;

    // invoke the sysex ready callback to render out the buffer
    return CALLBACK_INVOKE(&ui->config.callbacks, sysex_ready, ui, ui->buffer, length);
}


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

void lpui_pattern_editor_draw(lpui_t *ui, lpui_pattern_editor_t *editor) {
    lpui_position_t *pos = &editor->cmp.pos;
    lpui_size_t *size = &editor->cmp.size;
    uint8_t page_steps = size->width * size->height;
    uint8_t step_offset = editor->page * page_steps;

    lpui_sysex_start(ui);

    lpui_position_t p;
    for (p.y = 0; p.y < size->height; p.y++) {
        for (p.x = 0; p.x < size->width; p.x++) {
            uint8_t step_index = p.y * size->width + p.x + step_offset;
            lpui_color_t color = lpui_pattern_editor_get_step_color(editor, step_index);

            lpui_sysex_add_led(ui, (lpui_position_t) {
                .x = pos->x + p.x,
                .y = pos->y + size->height - 1 - p.y
            }, color);
        }
    }

    lpui_sysex_commit(ui);
}

void lpui_pattern_editor_set_pattern(lpui_pattern_editor_t *editor, pattern_t *pattern) {
    // check if pattern is available
    if (pattern == NULL) {
        if (editor->pattern != NULL) {
            editor->pattern = NULL;
            // TODO: draw empty pattern
        }
        return;
    }

    // return if the pattern step has not changed
    if (editor->pattern == pattern && editor->step_position == pattern->step_position) {
        return;
    }

    // store the new pattern and step position
    editor->pattern = pattern;
    editor->step_position = pattern->step_position;

    // automatic scrolling
    uint8_t page_steps = editor->cmp.size.width * editor->cmp.size.height;
    editor->page = pattern->step_position / page_steps;

    // TODO: update required parts only
    lpui_pattern_editor_draw(editor->cmp.ui, editor);
}

void lpui_pattern_editor_set_track_id(lpui_pattern_editor_t *editor, int track_id) {
    // redraw if the id changed on an active pattern
    if (editor->pattern && editor->track_id != track_id) {
        // TODO: redraw entire pattern
    }

    editor->track_id = track_id;
}


static lpui_color_t lpui_piano_editor_get_key_color(lpui_piano_editor_t *editor, int8_t key) {
    // check if key is valid
    if (key == -1) {
        return LPUI_COLOR_BLACK;
    }

    // TODO: different pressed color
    return LPUI_COLOR_PIANO_RELEASED;
}

void lpui_piano_editor_draw(lpui_t *ui, lpui_piano_editor_t *editor) {
    lpui_position_t *pos = &editor->cmp.pos;
    lpui_size_t *size = &editor->cmp.size;

    lpui_sysex_start(ui);

    lpui_position_t p;
    for (p.y = 0; p.y < size->height; p.y++) {
        for (p.x = 0; p.x < size->width; p.x++) {
            uint8_t octave = p.y / 2;
            uint8_t key = lpui_piano_editor_note_map[p.y % 2][p.x % 8];
            lpui_color_t color = lpui_piano_editor_get_key_color(editor, key);

            lpui_sysex_add_led(ui, (lpui_position_t) {
                .x = pos->x + p.x,
                .y = pos->y + p.y
            }, color);
        }
    }

    lpui_sysex_commit(ui);
}
