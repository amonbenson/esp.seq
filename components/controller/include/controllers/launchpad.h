#pragma once

#include "controller.h"
#include "midi_message.h"


#define LAUNCHPAD_VENDOR_ID 0x1235
#define LAUNCHPAD_PRO_PRODUCT_ID 0x51


extern const controller_class_t controller_class_launchpad;


typedef struct {
    controller_t super;
} controller_launchpad_t;


bool controller_launchpad_supported(void *context, const usb_device_desc_t *desc);

esp_err_t controller_launchpad_init(void *context);
esp_err_t controller_launchpad_free(void *context);

esp_err_t controller_launchpad_midi_recv(void *context, const midi_message_t *message);
esp_err_t controller_launchpad_sequencer_event(void *context, sequencer_event_t event, sequencer_t *sequencer, void *data);
