#pragma once

#include <stdint.h>
#include <esp_err.h>
#include "callback.h"
#include "lpui_types.h"
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

#define LPUI_COLOR_PIANO_RELEASED LPUI_COLOR(0x20, 0x20, 0x20)
#define LPUI_COLOR_PIANO_PRESSED LPUI_COLOR(0x3f, 0x3f, 0x3f)


typedef struct lpui_t lpui_t;
CALLBACK_DECLARE(lpui_sysex_ready, esp_err_t,
    lpui_t *ui, uint8_t *buffer, size_t length);


typedef struct {
    lpui_t *ui;
    lpui_position_t pos;
    lpui_size_t size;
} lpui_component_t;

typedef struct {
    lpui_component_t cmp;
    uint8_t page;

    int track_id;
    pattern_t *pattern;
    uint16_t step_position;
} lpui_pattern_editor_t;

typedef struct {
    lpui_component_t cmp;
} lpui_piano_editor_t;


typedef struct {
    struct {
        void *context;
        CALLBACK_TYPE(lpui_sysex_ready) sysex_ready;
    } callbacks;
} lpui_config_t;

struct lpui_t {
    lpui_config_t config;

    uint8_t *buffer;
    uint8_t *buffer_ptr;
};


esp_err_t lpui_init(lpui_t *ui, const lpui_config_t *config);
esp_err_t lpui_free(lpui_t *ui);


void lpui_pattern_editor_draw(lpui_t *ui, lpui_pattern_editor_t *editor);

void lpui_pattern_editor_set_pattern(lpui_pattern_editor_t *editor, pattern_t *pattern);
void lpui_pattern_editor_set_track_id(lpui_pattern_editor_t *editor, int track_id);


void lpui_piano_editor_draw(lpui_t *ui, lpui_piano_editor_t *editor);
