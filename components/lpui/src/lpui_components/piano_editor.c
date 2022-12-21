#include "lpui_components/piano_editor.h"
#include <esp_err.h>
#include <esp_log.h>


static const char *TAG = "piano_editor";

static const int8_t lpui_piano_editor_note_map[2][8] = {
    { 0, 2, 4, 5, 7, 9, 11, 12 },
    { -1, 1, 3, -1, 6, 8, 10, -1 }
};


esp_err_t piano_editor_init(piano_editor_t *editor, const piano_editor_config_t *config) {
    editor->config = *config;

    const lpui_component_functions_t functions = {
        .context = editor,
        .key_event = piano_editor_key_event
    };
    lpui_component_init(&editor->cmp, &config->cmp_config, &functions);

    return ESP_OK;
}

static lpui_color_t lpui_piano_editor_get_key_color(piano_editor_t *editor, int8_t key) {
    // check if key is valid
    if (key == -1) {
        return LPUI_COLOR_BLACK;
    }

    // TODO: different pressed color
    return LPUI_COLOR_PIANO_RELEASED;
}

static esp_err_t lpui_piano_editor_draw(lpui_t *ui, piano_editor_t *editor) {
    lpui_position_t *pos = &editor->cmp.config.pos;
    lpui_size_t *size = &editor->cmp.config.size;

    lpui_sysex_reset(ui, LPUI_SYSEX_COMMAND_SET_LEDS);

    lpui_position_t p;
    for (p.y = 0; p.y < size->height; p.y++) {
        for (p.x = 0; p.x < size->width; p.x++) {
            uint8_t octave = p.y / 2;
            uint8_t key = lpui_piano_editor_note_map[p.y % 2][p.x % 8];
            lpui_color_t color = lpui_piano_editor_get_key_color(editor, key);

            lpui_sysex_add_led_color(ui, (lpui_position_t) {
                .x = pos->x + p.x,
                .y = pos->y + p.y
            }, color);
        }
    }

    lpui_sysex_commit(ui);

    return ESP_OK;
}

esp_err_t piano_editor_update(piano_editor_t *editor) {
    return ESP_OK;
}

esp_err_t piano_editor_key_event(void *context, const lpui_position_t pos, uint8_t velocity) {
    ESP_LOGI(TAG, "piano editor button event at (%d, %d) with velocity %d", pos.x, pos.y, velocity);

    return ESP_OK;
}
