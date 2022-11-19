#pragma once

#include "controller.h"
#include "midi_message.h"


#define GENERIC_CC_BPM 1
#define GENERIC_BPM_MIN 40
#define GENERIC_BPM_MAX 240


extern const controller_class_t controller_class_generic;


typedef struct {
    controller_t super;
    uint8_t current_note;
} controller_generic_t;


bool controller_generic_supported(const usb_device_desc_t *desc);

esp_err_t controller_generic_init(controller_generic_t *controller);
esp_err_t controller_generic_free(controller_generic_t *controller);

esp_err_t controller_generic_midi_recv(controller_generic_t *controller, const midi_message_t *message);
esp_err_t controller_generic_sequencer_event(controller_generic_t *controller, esp_event_base_t event_base, int32_t event_id, void *event_data);
