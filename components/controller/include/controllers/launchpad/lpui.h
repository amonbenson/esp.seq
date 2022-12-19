#pragma once

#include <stdint.h>
#include <esp_err.h>
#include "controllers/launchpad/launchpad_types.h"
#include "pattern.h"


#define LPUI_SYSEX_BUFFER_SIZE 256

#define LPUI_COLOR(r, g, b) ((lpui_color_t) { .red = r, .green = g, .blue = b })
#define LPUI_COLOR_BLACK LPUI_COLOR(0x00, 0x00, 0x00)
#define LPUI_COLOR_PATTERNS \
    LPUI_COLOR(0x00, 0x30, 0x20), \
    LPUI_COLOR(0x30, 0x20, 0x00), \
    LPUI_COLOR(0x20, 0x00, 0x30), \
    LPUI_COLOR(0x20, 0x20, 0x20)
#define LPUI_COLOR_PLAYHEAD LPUI_COLOR(0x3f, 0x3f, 0x3f)


typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} lpui_color_t;

typedef struct {
    uint8_t x;
    uint8_t y;
} lpui_position_t;

typedef struct {
    uint8_t width;
    uint8_t height;
} lpui_size_t;

typedef struct {
    lpui_position_t pos;
    lpui_size_t size;
    bool redraw_required;
} lpui_component_t;

typedef struct {
    lpui_component_t cmp;
    uint8_t page;

    int track_id;
    pattern_t *pattern;
    uint16_t step_position;
} lpui_pattern_editor_t;

typedef struct {
    uint8_t *buffer;
    uint8_t *buffer_ptr;
    uint8_t buffer_size;
} lpui_t;


esp_err_t lpui_init(lpui_t *ui);
esp_err_t lpui_free(lpui_t *ui);


lpui_color_t lpui_color_darken(lpui_color_t color);
lpui_color_t lpui_color_lighten(lpui_color_t color);


void lpui_pattern_editor_draw(lpui_t *ui, lpui_pattern_editor_t *editor);

void lpui_pattern_editor_set_pattern(lpui_pattern_editor_t *editor, pattern_t *pattern);
void lpui_pattern_editor_set_track_id(lpui_pattern_editor_t *editor, int track_id);
