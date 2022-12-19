#pragma once

#include <stdint.h>
#include <esp_err.h>
#include "controllers/launchpad/launchpad_types.h"
#include "pattern.h"


#define LPUI_SYSEX_BUFFER_SIZE 256

#define LPUI_COLOR(r, g, b) ((lpui_color_t) { .red = r, .green = g, .blue = b })
#define LPUI_COLOR_SEQ_BG LPUI_COLOR(0x00, 0x04, 0x02)
#define LPUI_COLOR_SEQ_ENABLED LPUI_COLOR(0x00, 0x3f, 0x20)
#define LPUI_COLOR_SEQ_ACTIVE LPUI_COLOR(0x3f, 0x3f, 0x3f)


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


void lpui_pattern_editor_draw(lpui_t *ui, lpui_pattern_editor_t *editor);
void lpui_pattern_editor_set_pattern(lpui_pattern_editor_t *editor, pattern_t *pattern);
