#include "lpui.h"

#include <stdlib.h>
#include <string.h>
#include <esp_check.h>
#include "utlist.h"


static const char *TAG = "lpui";

static const uint8_t lpui_sysex_header[] = { LPUI_SYSEX_HEADER };
//static const lpui_color_t lpui_color_patterns[] = { LPUI_COLOR_PATTERNS };

static const int8_t lpui_piano_editor_note_map[2][8] = {
    { 0, 2, 4, 5, 7, 9, 11, 12 },
    { -1, 1, 3, -1, 6, 8, 10, -1 }
};


esp_err_t lpui_init(lpui_t *ui, const lpui_config_t *config) {
    // store the config
    ui->config = *config;

    // reset the components list
    ui->components = NULL;

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

esp_err_t lpui_component_init(lpui_component_t *cmp, const lpui_component_config_t *config) {
    cmp->config = *config;
    cmp->next = NULL;

    return ESP_OK;
}

esp_err_t lpui_add_component(lpui_t *ui, lpui_component_t *cmp) {
    // link the ui and component
    cmp->ui = ui;
    LL_PREPEND(ui->components, cmp);
    return ESP_OK;
}

esp_err_t lpui_remove_component(lpui_t *ui, lpui_component_t *cmp) {
    // unlink the ui and component
    LL_DELETE(ui->components, cmp);
    cmp->ui = NULL;
    return ESP_OK;
}


static bool lpui_sysex_buffer_has_space(lpui_t *ui, size_t size) {
    return ui->buffer_ptr + size < ui->buffer + LPUI_SYSEX_BUFFER_SIZE;
}

esp_err_t lpui_sysex_reset(lpui_t *ui, uint8_t command) {
    // reset the buffer pointer right after the header
    ui->buffer_ptr = ui->buffer + sizeof(lpui_sysex_header);

    // write the command
    *ui->buffer_ptr++ = command;
    return ESP_OK;
}

esp_err_t lpui_sysex_add_color(lpui_t *ui, const lpui_color_t color) {
    // validate the buffer size
    ESP_RETURN_ON_FALSE(lpui_sysex_buffer_has_space(ui, 3), ESP_ERR_NO_MEM,
        TAG, "not enough space in sysex buffer");

    // write rgb components
    *ui->buffer_ptr++ = color.red;
    *ui->buffer_ptr++ = color.green;
    *ui->buffer_ptr++ = color.blue;

    return ESP_OK;
}

esp_err_t lpui_sysex_add_led(lpui_t *ui, const lpui_position_t pos, const lpui_color_t color) {
    // validate the buffer size
    ESP_RETURN_ON_FALSE(lpui_sysex_buffer_has_space(ui, 4), ESP_ERR_NO_MEM,
        TAG, "not enough space in sysex buffer");

    // write the position and rgb components
    *ui->buffer_ptr++ = pos.x + pos.y * 10;
    *ui->buffer_ptr++ = color.red;
    *ui->buffer_ptr++ = color.green;
    *ui->buffer_ptr++ = color.blue;

    return ESP_OK;
}

esp_err_t lpui_sysex_commit(lpui_t *ui) {
    // validate the buffer size
    ESP_RETURN_ON_FALSE(lpui_sysex_buffer_has_space(ui, 1), ESP_ERR_NO_MEM,
        TAG, "not enough space in sysex buffer");

    // add the terminator byze and calculate the length
    *ui->buffer_ptr++ = 0xF7;
    size_t length = ui->buffer_ptr - ui->buffer;

    // invoke the sysex ready callback to render out the buffer
    return CALLBACK_INVOKE(&ui->config.callbacks, sysex_ready, ui, ui->buffer, length);
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
    lpui_position_t *pos = &editor->cmp.config.pos;
    lpui_size_t *size = &editor->cmp.config.size;

    lpui_sysex_reset(ui, LPUI_SYSEX_COMMAND_SET_LEDS);

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
