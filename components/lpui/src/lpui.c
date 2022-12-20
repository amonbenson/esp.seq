#include "lpui.h"

#include <stdlib.h>
#include <string.h>


// set leds rgb command
static const uint8_t lpui_sysex_header[] = { 0xF0, 0x00, 0x20, 0x29, 0x02, 0x10, 0x0B };
static const lpui_color_t lpui_color_patterns[] = { LPUI_COLOR_PATTERNS };

static const int8_t lpui_piano_editor_note_map[2][8] = {
    { 0, 2, 4, 5, 7, 9, 11, 12 },
    { -1, 1, 3, -1, 6, 8, 10, -1 }
};


esp_err_t lpui_init(lpui_t *ui) {
    // allocate the sysex buffer
    ui->buffer = calloc(LPUI_SYSEX_BUFFER_SIZE, sizeof(uint8_t));
    if (ui->buffer == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // store sysex header
    memcpy(ui->buffer, lpui_sysex_header, sizeof(lpui_sysex_header));

    ui->buffer_ptr = ui->buffer;
    ui->buffer_size = 0;
    return ESP_OK;
}

esp_err_t lpui_free(lpui_t *ui) {
    free(ui->buffer);
    return ESP_OK;
}


inline lpui_color_t lpui_color_darken(lpui_color_t color) {
    return (lpui_color_t) {
        .red = color.red / 16,
        .green = color.green / 16,
        .blue = color.blue / 16,
    };
}

inline lpui_color_t lpui_color_lighten(lpui_color_t color) {
    return (lpui_color_t) {
        .red = color.red + (0xff - color.red) / 16,
        .green = color.green + (0xff - color.green) / 16,
        .blue = color.blue + (0xff - color.blue) / 16,
    };
}


static void lpui_sysex_start(lpui_t *ui) {
    // reset the buffer pointer and write the command type
    ui->buffer_ptr = ui->buffer + sizeof(lpui_sysex_header);
}

static void lpui_sysex_add_led(lpui_t *ui, lpui_position_t pos, const lpui_color_t color) {
    // write the position
    *ui->buffer_ptr++ = pos.x + pos.y * 10;

    // write rgb components
    *ui->buffer_ptr++ = color.red;
    *ui->buffer_ptr++ = color.green;
    *ui->buffer_ptr++ = color.blue;
}

static void lpui_sysex_end(lpui_t *ui) {
    *ui->buffer_ptr++ = 0xF7;
    ui->buffer_size = ui->buffer_ptr - ui->buffer;
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

    lpui_sysex_end(ui);
    editor->cmp.redraw_required = false;
}

void lpui_pattern_editor_set_pattern(lpui_pattern_editor_t *editor, pattern_t *pattern) {
    // check if pattern is available
    if (pattern == NULL) {
        if (editor->pattern != NULL) {
            editor->pattern = NULL;
            editor->cmp.redraw_required = true;
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

    // enable redraw flag
    editor->cmp.redraw_required = true;
}

void lpui_pattern_editor_set_track_id(lpui_pattern_editor_t *editor, int track_id) {
    // redraw if the id changed on an active pattern
    if (editor->pattern && editor->track_id != track_id) {
        editor->cmp.redraw_required = true;
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

    lpui_sysex_end(ui);
    editor->cmp.redraw_required = false;
}
