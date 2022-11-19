#pragma once

#include "esp_err.h"
#include "midi_message.h"
#include "sequencer.h"
#include "output.h"
#include "usb.h"


typedef struct controller_t controller_t;
typedef bool (*controller_supported_function)(const usb_device_desc_t *desc);
typedef esp_err_t (*controller_init_function)(controller_t *controller);
typedef esp_err_t (*controller_free_function)(controller_t *controller);
typedef esp_err_t (*controller_midi_send_function)(controller_t *controller, const midi_message_t *message);
typedef esp_err_t (*controller_midi_recv_function)(controller_t *controller, const midi_message_t *message);
typedef esp_err_t (*controller_sequencer_event_function)(controller_t *controller, esp_event_base_t event_base, int32_t event_id, void *event_data);

typedef struct {
    unsigned int size;
    controller_supported_function supported;
    controller_init_function init;
    controller_free_function free;
    controller_midi_recv_function midi_recv;
    controller_sequencer_event_function sequencer_event;
} controller_class_t;

typedef struct {
    sequencer_t *sequencer;
    output_t *output;
    controller_midi_send_function midi_send;
} controller_config_t;

struct controller_t {
    const controller_class_t *class;
    controller_config_t config;
};


bool controller_supported(const controller_class_t *class, const usb_device_desc_t *desc);

controller_t *controller_create_from_desc(const controller_class_t *classes[], const usb_device_desc_t *desc, const controller_config_t *config);
controller_t *controller_create(const controller_class_t *class, const controller_config_t *config);
esp_err_t controller_free(controller_t *controller);

esp_err_t controller_midi_recv(controller_t *controller, const midi_message_t *message);
esp_err_t controller_sequencer_event(controller_t *controller, esp_event_base_t event_base, int32_t event_id, void *event_data);
