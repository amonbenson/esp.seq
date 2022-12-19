#pragma once

#include "controller.h"
#include "midi_message.h"
#include "controllers/launchpad/launchpad_types.h"
#include "controllers/launchpad/lpui.h"


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


extern const controller_class_t controller_class_launchpad;


typedef struct {
    controller_t super;

    lpui_t ui;
    lpui_pattern_editor_t pattern_editor;
} controller_launchpad_t;


bool controller_launchpad_supported(void *context, const usb_device_desc_t *desc);

esp_err_t controller_launchpad_init(void *context);
esp_err_t controller_launchpad_free(void *context);

esp_err_t controller_launchpad_midi_recv(void *context, const midi_message_t *message);
esp_err_t controller_launchpad_sequencer_event(void *context, sequencer_event_t event, sequencer_t *sequencer, void *data);
