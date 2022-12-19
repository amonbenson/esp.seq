#include "controllers/launchpad/lpui.h"

#include <stdlib.h>
#include <string.h>

static const uint8_t lpui_sysex_header[] = { LP_SYSEX_HEADER };


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


static void lpui_sysex_start(lpui_t *ui, uint8_t command) {
    // reset the buffer pointer and write the command type
    ui->buffer_ptr = ui->buffer + sizeof(lpui_sysex_header);
    *ui->buffer_ptr++ = command;
}

static void lpui_sysex_color(lpui_t *ui, const lpui_color_t color) {
    // write rgb components
    *ui->buffer_ptr++ = color.red;
    *ui->buffer_ptr++ = color.green;
    *ui->buffer_ptr++ = color.blue;
}

static void lpui_sysex_led_color(lpui_t *ui, lpui_position_t pos, const lpui_color_t color) {
    // write the position and color
    *ui->buffer_ptr++ = pos.x + pos.y * 10;
    lpui_sysex_color(ui, color);
}

static void lpui_sysex_end(lpui_t *ui) {
    *ui->buffer_ptr++ = 0xF7;
    ui->buffer_size = ui->buffer_ptr - ui->buffer;
}


static lpui_color_t lpui_get_step_color(pattern_t *pattern, uint16_t step_index) {
    if (pattern == NULL) {
        return LPUI_COLOR_SEQ_BG;
    }

    pattern_step_t *step = &pattern->steps[step_index];
    if (step_index == pattern->step_position) {
        return LPUI_COLOR_SEQ_ACTIVE;
    } else if (step->atomic.velocity > 0) {
        return LPUI_COLOR_SEQ_ENABLED;
    } else {
        return LPUI_COLOR_SEQ_BG;
    }
}

void lpui_pattern_editor_draw(lpui_t *ui, lpui_pattern_editor_t *editor) {
    lpui_position_t *pos = &editor->cmp.pos;
    lpui_size_t *size = &editor->cmp.size;
    uint8_t page_steps = size->width * size->height;
    uint8_t step_offset = editor->page * page_steps;

    lpui_sysex_start(ui, LP_SET_LEDS_RGB);

    lpui_position_t p;
    for (p.y = 0; p.y < size->height; p.y++) {
        for (p.x = 0; p.x < size->width; p.x++) {
            uint8_t step_index = p.y * size->width + p.x + step_offset;
            lpui_color_t color = lpui_get_step_color(editor->pattern, step_index);

            lpui_sysex_led_color(ui, (lpui_position_t) {
                .x = pos->x + p.x,
                .y = 9 - pos->y - p.y
            }, color);
        }
    }

    lpui_sysex_end(ui);
}

void lpui_pattern_editor_set_pattern(lpui_pattern_editor_t *editor, pattern_t *pattern) {
    editor->cmp.redraw_required = false;

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
