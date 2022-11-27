#pragma once

#include "esp_err.h"
#include "midi_message.h"
#include "sequencer.h"
#include "output.h"
#include "usb.h"
#include "callback.h"


typedef struct controller_t controller_t;
// incoming events, handled by the controller
CALLBACK_DECLARE(controller_supported, bool,
    const usb_device_desc_t *desc);
CALLBACK_DECLARE(controller_init, esp_err_t);
CALLBACK_DECLARE(controller_free, esp_err_t);
CALLBACK_DECLARE(controller_midi_recv, esp_err_t,
    const midi_message_t *message)
CALLBACK_DECLARE(controller_sequencer_event, esp_err_t,
    sequencer_event_t event, sequencer_t *sequencer, void *data);

// outgoing events, invoked by the controller
CALLBACK_DECLARE(controller_midi_send, esp_err_t,
    controller_t *controller, const midi_message_t *message);


typedef struct {
    void *context;
    CALLBACK_TYPE(controller_supported) supported;
    CALLBACK_TYPE(controller_init) init;
    CALLBACK_TYPE(controller_free) free;
    CALLBACK_TYPE(controller_midi_recv) midi_recv;
    CALLBACK_TYPE(controller_sequencer_event) sequencer_event;
} controller_class_functions_t;

typedef struct {
    unsigned int size;
    controller_class_functions_t functions;
} controller_class_t;

typedef struct {
    struct {
        void *context;
        CALLBACK_TYPE(controller_midi_send) midi_send;
    } callbacks;
    sequencer_t *sequencer;
    output_t *output;
} controller_config_t;

struct controller_t {
    const controller_class_t *class;
    controller_class_functions_t functions;
    controller_config_t config;
};


bool controller_supported(const controller_class_t *class, const usb_device_desc_t *desc);

controller_t *controller_create_from_desc(const controller_class_t *classes[], const usb_device_desc_t *desc, const controller_config_t *config);
controller_t *controller_create(const controller_class_t *class, const controller_config_t *config);
esp_err_t controller_free(controller_t *controller);

esp_err_t controller_midi_send(controller_t *controller, const midi_message_t *message);
esp_err_t controller_midi_send_sysex(controller_t *controller, const uint8_t *data, size_t length);

esp_err_t controller_midi_recv(controller_t *controller, const midi_message_t *message);
esp_err_t controller_sequencer_event(controller_t *controller, sequencer_event_t event, sequencer_t *sequencer, void *data);
