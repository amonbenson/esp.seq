#pragma once

#include "esp_err.h"
#include "midi_message.h"
#include "sequencer.h"
#include "output.h"


typedef struct controller_t controller_t;
typedef esp_err_t (*controller_init_function)(controller_t *controller);
typedef esp_err_t (*controller_free_function)(controller_t *controller);
typedef esp_err_t (*controller_midi_send_function)(controller_t *controller, const midi_message_t *message);
typedef esp_err_t (*controller_midi_recv_function)(controller_t *controller, const midi_message_t *message);
typedef esp_err_t (*controller_sequencer_event_function)(controller_t *controller, esp_event_base_t event_base, int32_t event_id, void *event_data);

typedef struct {
    unsigned int size;
    controller_init_function init;
    controller_free_function free;
    controller_midi_recv_function midi_recv;
    controller_sequencer_event_function sequencer_event;
} controller_class_t;

struct controller_t {
    const controller_class_t *class;
    sequencer_t *sequencer;
    output_t *output;
    controller_midi_send_function midi_send;
};


controller_t *controller_create(const controller_class_t *class,
    sequencer_t *sequencer,
    output_t *output,
    controller_midi_send_function midi_send);
esp_err_t controller_free(controller_t *controller);

esp_err_t controller_midi_recv(controller_t *controller, const midi_message_t *message);
esp_err_t controller_sequencer_event(controller_t *controller, esp_event_base_t event_base, int32_t event_id, void *event_data);
