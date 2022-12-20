#pragma once

#include <stdint.h>
#include <esp_err.h>
#include "midi_message.h"
#include "callback.h"
#include "lpui_types.h"


#define LPUI_SYSEX_BUFFER_SIZE 256
#define LPUI_SYSEX_HEADER 0xF0, 0x00, 0x20, 0x29, 0x02, 0x10
#define LPUI_SYSEX_COMMAND_SET_LEDS 0x0B

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


CALLBACK_DECLARE(lpui_component_button_event, esp_err_t,
    const lpui_position_t pos, uint8_t velocity);

typedef struct {
    void *context;
    CALLBACK_TYPE(lpui_component_button_event) button_event;
} lpui_component_functions_t;

typedef struct {
    lpui_position_t pos;
    lpui_size_t size;
} lpui_component_config_t;

typedef struct lpui_t lpui_t;
typedef struct lpui_component_t lpui_component_t;
struct lpui_component_t {
    lpui_component_config_t config;
    lpui_component_functions_t functions;

    lpui_t *ui;
    lpui_component_t *next;
};


CALLBACK_DECLARE(lpui_sysex_ready, esp_err_t,
    lpui_t *ui, uint8_t *buffer, size_t length);

typedef struct {
    struct {
        void *context;
        CALLBACK_TYPE(lpui_sysex_ready) sysex_ready;
    } callbacks;
} lpui_config_t;

struct lpui_t {
    lpui_config_t config;

    lpui_component_t *components;

    uint8_t *buffer;
    uint8_t *buffer_ptr;
};


esp_err_t lpui_init(lpui_t *ui, const lpui_config_t *config);
esp_err_t lpui_free(lpui_t *ui);

esp_err_t lpui_component_init(lpui_component_t *cmp,
    const lpui_component_config_t *config,
    const lpui_component_functions_t *functions);
esp_err_t lpui_add_component(lpui_t *ui, lpui_component_t *cmp);
esp_err_t lpui_remove_component(lpui_t *ui, lpui_component_t *cmp);


esp_err_t lpui_sysex_reset(lpui_t *ui, uint8_t command);
esp_err_t lpui_sysex_add_color(lpui_t *ui, lpui_color_t color);
esp_err_t lpui_sysex_add_led(lpui_t *ui, lpui_position_t pos, lpui_color_t color);
esp_err_t lpui_sysex_commit(lpui_t *ui);


esp_err_t lpui_midi_recv(lpui_t *ui, const midi_message_t *message);
