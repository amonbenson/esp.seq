#pragma once

#include "controller.h"
#include "midi_message.h"


#define LAUNCHPAD_VENDOR_ID 0x1235
#define LAUNCHPAD_PRO_PRODUCT_ID 0x51


extern const controller_class_t controller_class_launchpad;


typedef struct {
    controller_t super;
} controller_launchpad_t;


bool controller_launchpad_supported(const usb_device_desc_t *desc);

esp_err_t controller_launchpad_init(controller_launchpad_t *controller);
esp_err_t controller_launchpad_free(controller_launchpad_t *controller);

esp_err_t controller_launchpad_midi_recv(controller_launchpad_t *controller, const midi_message_t *message);
esp_err_t controller_launchpad_sequencer_event(controller_launchpad_t *controller, esp_event_base_t event_base, int32_t event_id, void *event_data);
