#pragma once

#include "controller.h"
#include "midi_message.h"
#include "launchpad_types.h"
#include "lpui.h"
#include "lpui_components/button.h"
#include "lpui_components/pattern_editor.h"
#include "lpui_components/piano_editor.h"


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
    pattern_editor_t pattern_editor;
    piano_editor_t piano_editor;
    button_t play_button;
    button_t record_button;

    int selected_track_id;
    pattern_step_t *selected_step;
} controller_launchpad_t;


bool controller_launchpad_supported(void *context, const usb_device_desc_t *desc);

esp_err_t controller_launchpad_init(void *context);
esp_err_t controller_launchpad_free(void *context);

esp_err_t controller_launchpad_midi_recv(void *context, const midi_message_t *message);
esp_err_t controller_launchpad_sequencer_event(void *context, sequencer_event_t event, sequencer_t *sequencer, void *data);

esp_err_t controller_launchpad_select_track(controller_launchpad_t *controller, int track_id);
esp_err_t controller_launchpad_select_step(controller_launchpad_t *controller, pattern_step_t *step);
