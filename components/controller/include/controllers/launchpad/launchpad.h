#pragma once

#include "controller.h"
#include "midi_message.h"


#define LP_VENDOR_ID 0x1235
#define LP_PRODUCT_ID 0x51

#define LP_SYSEX_BUFFER_SIZE 256
#define LP_SYSEX_HEADER 0xF0, 0x00, 0x20, 0x29, 0x02, 0x10
#define LP_SYSEX_COMMAND_SET_LEDS 0x0A
#define LP_SYSEX_COMMAND_SET_LEDS_RGB 0x0B
#define LP_SYSEX_COMMAND_SET_LEDS_COLUMN 0x0C
#define LP_SYSEX_COMMAND_SET_LEDS_ROW 0x0D
#define LP_SYSEX_COMMAND_SET_LEDS_ALL 0x0E
#define LP_SYSEX_COMMAND_SET_LEDS_GRID_RGB 0x0F

#define LP_SYSEX_GRID_10X10 0x00
#define LP_SYSEX_GRID_8X8 0x01

#define LP_SYSEX_SIDE_LED 0x63
#define LP_SYSEX_COLOR_CLEAR 0x00

#define LP_COLOR(r, g, b) ((lp_color_t) { .red = r, .green = g, .blue = b })
#define LP_COLOR_SEQ_BG LP_COLOR(0x00, 0x04, 0x02)
#define LP_COLOR_SEQ_ENABLED LP_COLOR(0x00, 0x3f, 0x20)
#define LP_COLOR_SEQ_ACTIVE LP_COLOR(0x3f, 0x3f, 0x3f)


extern const controller_class_t controller_class_launchpad;


typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} lp_color_t;

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
} lp_bounds_t;

typedef struct {
    pattern_t *pattern;
    lp_bounds_t bounds;
    uint8_t page;
    uint16_t last_step_position;
} lp_pattern_editor_t;

typedef struct {
    controller_t super;
    lp_pattern_editor_t pattern_editor;
} controller_launchpad_t;


bool controller_launchpad_supported(void *context, const usb_device_desc_t *desc);

esp_err_t controller_launchpad_init(void *context);
esp_err_t controller_launchpad_free(void *context);

esp_err_t controller_launchpad_midi_recv(void *context, const midi_message_t *message);
esp_err_t controller_launchpad_sequencer_event(void *context, sequencer_event_t event, sequencer_t *sequencer, void *data);
