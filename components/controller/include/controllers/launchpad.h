#pragma once

#include "controller.h"
#include "midi_message.h"


#define LP_VENDOR_ID 0x1235
#define LP_PRODUCT_ID 0x51

#define LP_SYSEX_HEADER 0xF0, 0x00, 0x20, 0x29, 0x02, 0x10


extern const controller_class_t controller_class_launchpad;


typedef struct {
    controller_t super;
} controller_launchpad_t;


bool controller_launchpad_supported(void *context, const usb_device_desc_t *desc);

esp_err_t controller_launchpad_init(void *context);
esp_err_t controller_launchpad_free(void *context);

esp_err_t controller_launchpad_midi_recv(void *context, const midi_message_t *message);
esp_err_t controller_launchpad_sequencer_event(void *context, sequencer_event_t event, sequencer_t *sequencer, void *data);
