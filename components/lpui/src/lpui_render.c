#include "lpui_render.h"

#include <string.h>


static const uint8_t lpui_sysex_header[] = { LPUI_SYSEX_HEADER };


static inline unsigned int lpui_sysex_partial_len(uint8_t n) {
    return sizeof(lpui_sysex_header) + 2 + n * 4;
}

static inline unsigned int lpui_sysex_full_len() {
    return sizeof(lpui_sysex_header) + 3 + 100 * 3;
}

static uint8_t *lpui_sysex_add_header(uint8_t *b) {
    memcpy(b, lpui_sysex_header, sizeof(lpui_sysex_header));
    b += sizeof(lpui_sysex_header);

    return b;
}

static uint8_t *lpui_sysex_add_color(uint8_t *b, lpui_color_t color) {
    *b++ = color.red;
    *b++ = color.green;
    *b++ = color.blue;

    return b;
}

static uint8_t *lpui_sysex_add_led_color(uint8_t *b, lpui_position_t pos, lpui_color_t color) {
    *b++ = pos.x + pos.y * 10;
    b = lpui_sysex_add_color(b, color);

    return b;
}


esp_err_t lpui_render(lpui_image_t *img, uint8_t *buf, uint16_t len) {
    esp_err_t ret;

    // check which bytes would need updating

    // store the previous buffer
    lpui_image_update_previous(img);

    return ESP_OK;
}
